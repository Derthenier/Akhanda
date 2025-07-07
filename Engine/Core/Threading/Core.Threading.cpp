module;

#ifdef _WIN32
#include <windows.h>
#include <processthreadsapi.h>
#include <sysinfoapi.h>
#include <intrin.h>
#pragma intrinsic(_mm_pause)
#endif

#undef max
#undef min
#undef Yield

module Akhanda.Core.Threading;

import Akhanda.Core.Memory;
import std;

namespace Akhanda::Threading {

    // ============================================================================
    // Global State
    // ============================================================================

    namespace {
        struct ThreadingState {
            std::atomic<bool> initialized{ false };
            ThreadingConfig config{};
            HardwareInfo hardwareInfo{};

            std::unique_ptr<Memory::IAllocator> threadAllocator;
            std::unique_ptr<Memory::IAllocator> jobAllocator;

            std::vector<std::unique_ptr<Thread>> managedThreads;
            mutable ReadWriteLock threadsLock;

            std::unordered_map<std::thread::id, Thread*> threadLookup;
            mutable SpinLock lookupLock;
        };

        static ThreadingState* g_state = nullptr;
        static std::once_flag g_initFlag;
    }

    // ============================================================================
    // Hardware Detection Implementation
    // ============================================================================

    HardwareInfo HardwareDetector::DetectHardware() noexcept {
        HardwareInfo info{};

#ifdef _WIN32
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        info.logicalCores = sysInfo.dwNumberOfProcessors;

        // Get physical cores using CPUID
        int cpuInfo[4];
        __cpuid(cpuInfo, 0x80000000);
        if (cpuInfo[0] >= 0x80000008) {
            __cpuid(cpuInfo, 0x80000008);
            info.physicalCores = (cpuInfo[2] & 0xFF) + 1;
        }
        else {
            // Fallback: assume no hyperthreading
            info.physicalCores = info.logicalCores;
        }

        info.hyperthreadingEnabled = (info.logicalCores > info.physicalCores);

        // Detect NUMA nodes
        ULONG highestNodeNumber = 0;
        if (GetNumaHighestNodeNumber(&highestNodeNumber)) {
            info.numaNodes = highestNodeNumber + 1;
        }
        else {
            info.numaNodes = 1;
        }

        // Generate affinity masks for worker threads
        info.coreAffinityMasks.reserve(info.logicalCores);
        for (uint32_t i = 0; i < info.logicalCores; ++i) {
            info.coreAffinityMasks.push_back(1ULL << i);
        }
#else
        // Fallback for non-Windows platforms
        info.logicalCores = std::thread::hardware_concurrency();
        info.physicalCores = info.logicalCores;
        info.numaNodes = 1;
        info.hyperthreadingEnabled = false;
#endif

        return info;
    }

    uint32_t HardwareDetector::GetRecommendedWorkerThreadCount() noexcept {
        const auto hardware = DetectHardware();
        // Reserve main thread, prefer physical cores
        return std::max(1u, hardware.physicalCores - 1);
    }

    uint64_t HardwareDetector::GetThreadAffinityMask(uint32_t threadIndex) noexcept {
        const auto hardware = DetectHardware();
        if (threadIndex < hardware.coreAffinityMasks.size()) {
            return hardware.coreAffinityMasks[threadIndex];
        }
        return 0; // No affinity
    }

    // ============================================================================
    // SpinLock Implementation
    // ============================================================================

    void SpinLock::lock() noexcept {
        constexpr uint32_t SPIN_COUNT = 1000;
        uint32_t spinCount = 0;

        while (flag_.test_and_set(std::memory_order_acquire)) {
            if (++spinCount < SPIN_COUNT) {
#ifdef _WIN32
                _mm_pause(); // CPU pause instruction
#else
                std::this_thread::yield();
#endif
            }
            else {
                std::this_thread::yield();
                spinCount = 0;
            }
        }
    }

    bool SpinLock::try_lock() noexcept {
        return !flag_.test_and_set(std::memory_order_acquire);
    }

    void SpinLock::unlock() noexcept {
        flag_.clear(std::memory_order_release);
    }

    // ============================================================================
    // ReadWriteLock Implementation
    // ============================================================================

    void ReadWriteLock::lock_shared() noexcept {
        mutex_.lock_shared();
    }

    void ReadWriteLock::unlock_shared() noexcept {
        mutex_.unlock_shared();
    }

    void ReadWriteLock::lock() noexcept {
        mutex_.lock();
    }

    void ReadWriteLock::unlock() noexcept {
        mutex_.unlock();
    }

    bool ReadWriteLock::try_lock_shared() noexcept {
        return mutex_.try_lock_shared();
    }

    bool ReadWriteLock::try_lock() noexcept {
        return mutex_.try_lock();
    }

    // ============================================================================
    // Thread Implementation
    // ============================================================================

    Thread::Thread(const ThreadDesc& desc) noexcept : desc_(desc) {
    }

    Thread::~Thread() noexcept {
        if (running_.load(std::memory_order_acquire)) {
            Join();
        }
    }

    Thread::Thread(Thread&& other) noexcept
        : thread_(std::move(other.thread_))
        , desc_(std::move(other.desc_))
        , running_(other.running_.load()) {
        other.running_.store(false);
    }

    Thread& Thread::operator=(Thread&& other) noexcept {
        if (this != &other) {
            if (running_.load()) {
                Join();
            }

            thread_ = std::move(other.thread_);
            desc_ = std::move(other.desc_);
            running_.store(other.running_.load());
            other.running_.store(false);
        }
        return *this;
    }

    template<typename F, typename... Args>
    bool Thread::Start(F&& func, Args&&... args) noexcept {
        if (running_.load(std::memory_order_acquire)) {
            return false; // Already running
        }

        try {
            auto wrapper = [this, func = std::forward<F>(func), args...]() mutable {
                // Set thread name
#ifdef _WIN32
                if (!desc_.name.empty()) {
                    const std::wstring wideName(desc_.name.begin(), desc_.name.end());
                    SetThreadDescription(GetCurrentThread(), wideName.c_str());
                }
#endif

                // Set thread priority
                SetPriority(desc_.priority);

                // Set thread affinity
                if (desc_.affinityMask != 0) {
                    SetAffinity(desc_.affinityMask);
                }

                // Register with thread manager
                if (g_state && g_state->initialized.load()) {
                    SpinLockGuard lock(g_state->lookupLock);
                    g_state->threadLookup[std::this_thread::get_id()] = this;
                }

                // Execute the function
                running_.store(true, std::memory_order_release);

                try {
                    if constexpr (std::is_void_v<std::invoke_result_t<F, Args...>>) {
                        std::invoke(func, args...);
                    }
                    else {
                        auto result = std::invoke(func, args...);
                        // Could store result for future retrieval
                    }
                }
                catch (...) {
                    // Log exception but don't let it escape
                }

                running_.store(false, std::memory_order_release);

                // Unregister from thread manager
                if (g_state && g_state->initialized.load()) {
                    SpinLockGuard lock(g_state->lookupLock);
                    g_state->threadLookup.erase(std::this_thread::get_id());
                }
                };

            thread_ = std::make_unique<std::thread>(std::move(wrapper));
            return true;

        }
        catch (...) {
            return false;
        }
    }

    bool Thread::Join(uint32_t timeoutMs) noexcept {
        if (!thread_ || !thread_->joinable()) {
            return false;
        }

        try {
            if (timeoutMs == INFINITE) {
                thread_->join();
                return true;
            }
            else {
                // Timed join not directly supported, use polling
                auto start = std::chrono::steady_clock::now();
                auto timeout = std::chrono::milliseconds(timeoutMs);

                while (running_.load(std::memory_order_acquire)) {
                    if (std::chrono::steady_clock::now() - start >= timeout) {
                        return false; // Timeout
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

                thread_->join();
                return true;
            }
        }
        catch (...) {
            return false;
        }
    }

    bool Thread::Detach() noexcept {
        if (!thread_ || !thread_->joinable()) {
            return false;
        }

        try {
            thread_->detach();
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool Thread::IsRunning() const noexcept {
        return running_.load(std::memory_order_acquire);
    }

    bool Thread::SetPriority(ThreadPriority priority) noexcept {
#ifdef _WIN32
        if (!thread_) return false;

        int winPriority;
        switch (priority) {
        case ThreadPriority::Idle:      winPriority = THREAD_PRIORITY_IDLE; break;
        case ThreadPriority::Low:       winPriority = THREAD_PRIORITY_BELOW_NORMAL; break;
        case ThreadPriority::Normal:    winPriority = THREAD_PRIORITY_NORMAL; break;
        case ThreadPriority::High:      winPriority = THREAD_PRIORITY_ABOVE_NORMAL; break;
        case ThreadPriority::Critical:  winPriority = THREAD_PRIORITY_HIGHEST; break;
        default:                        winPriority = THREAD_PRIORITY_NORMAL; break;
        }

        HANDLE handle = thread_->native_handle();
        return SetThreadPriority(handle, winPriority) != 0;
#else
        return true; // Not implemented on other platforms
#endif
    }

    bool Thread::SetAffinity(uint64_t affinityMask) noexcept {
#ifdef _WIN32
        if (!thread_ || affinityMask == 0) return false;

        HANDLE handle = thread_->native_handle();
        DWORD_PTR result = SetThreadAffinityMask(handle, static_cast<DWORD_PTR>(affinityMask));
        return result != 0;
#else
        return true; // Not implemented on other platforms
#endif
    }

    std::thread::id Thread::GetId() const noexcept {
        return thread_ ? thread_->get_id() : std::thread::id{};
    }

    const std::string& Thread::GetName() const noexcept {
        return desc_.name;
    }

    ThreadType Thread::GetType() const noexcept {
        return desc_.type;
    }

    Thread* Thread::GetCurrent() noexcept {
        if (g_state && g_state->initialized.load()) {
            SpinLockGuard lock(g_state->lookupLock);
            auto it = g_state->threadLookup.find(std::this_thread::get_id());
            if (it != g_state->threadLookup.end()) {
                return it->second; // Return pointer directly
            }
        }

        // Current thread not managed by ThreadManager
        return nullptr;
    }

    void Thread::Sleep(uint32_t milliseconds) noexcept {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    void Thread::Yield() noexcept {
        std::this_thread::yield();
    }

    // ============================================================================
    // Event Implementation
    // ============================================================================

    Event::Event(bool manualReset, bool initialState) noexcept
        : state_(initialState), manualReset_(manualReset) {
    }

    void Event::Set() noexcept {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            state_ = true;
        }

        if (manualReset_) {
            cv_.notify_all();
        }
        else {
            cv_.notify_one();
        }
    }

    void Event::Reset() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = false;
    }

    bool Event::Wait(uint32_t timeoutMs) noexcept {
        std::unique_lock<std::mutex> lock(mutex_);

        if (timeoutMs == INFINITE) {
            cv_.wait(lock, [this] { return state_; });

            if (!manualReset_) {
                state_ = false; // Auto-reset
            }
            return true;
        }
        else {
            auto timeout = std::chrono::milliseconds(timeoutMs);
            bool result = cv_.wait_for(lock, timeout, [this] { return state_; });

            if (result && !manualReset_) {
                state_ = false; // Auto-reset
            }
            return result;
        }
    }

    bool Event::IsSet() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    // ============================================================================
    // Semaphore Implementation
    // ============================================================================

    Semaphore::Semaphore(uint32_t initialCount, [[maybe_unused]] uint32_t maxCount) noexcept
        : semaphore_(initialCount), count_(initialCount) {
    }

    bool Semaphore::Acquire(uint32_t timeoutMs) noexcept {
        try {
            if (timeoutMs == INFINITE) {
                semaphore_.acquire();
                count_.decrement();
                return true;
            }
            else {
                auto timeout = std::chrono::milliseconds(timeoutMs);
                bool result = semaphore_.try_acquire_for(timeout);
                if (result) {
                    count_.decrement();
                }
                return result;
            }
        }
        catch (...) {
            return false;
        }
    }

    void Semaphore::Release(uint32_t count) noexcept {
        try {
            semaphore_.release(count);
            count_.fetch_add(count);
        }
        catch (...) {
            // Ignore exceptions
        }
    }

    uint32_t Semaphore::GetCount() const noexcept {
        return count_.load();
    }

    // ============================================================================
    // Custom Barrier Implementation
    // ============================================================================

    Barrier::Barrier(uint32_t threadCount) noexcept
        : threadCount_(threadCount) {
        if (threadCount_ == 0) {
            // Invalid thread count, set to 1 to prevent deadlock
            const_cast<uint32_t&>(threadCount_) = 1;
        }
    }

    void Barrier::Wait() noexcept {
        if (threadCount_ == 1) {
            return; // Single thread barrier is a no-op
        }

        const uint32_t currentGen = state_.generation.load();

        // Increment arrived count
        const uint32_t arrivedCount = state_.arrived.increment();

        if (arrivedCount == threadCount_) {
            // Last thread to arrive - signal all and reset
            {
                std::lock_guard<std::mutex> lock(mutex_);
                state_.signaled.store(true);
            }
            WakeAll();

            // Reset for next use (only the last thread does this)
            state_.arrived.store(0);
            state_.waiting.store(0);
            state_.signaled.store(false);
            state_.generation.increment(); // New generation
        }
        else {
            // Not the last thread - wait for signal
            std::unique_lock<std::mutex> lock(mutex_);
            state_.waiting.increment();

            cv_.wait(lock, [this, currentGen]() {
                return state_.signaled.load() ||
                    state_.generation.load() != currentGen;
                });

            state_.waiting.decrement();
        }
    }

    bool Barrier::TryWait(uint32_t timeoutMs) noexcept {
        if (threadCount_ == 1) {
            return true; // Single thread barrier always succeeds immediately
        }

        const uint32_t currentGen = state_.generation.load();

        // Increment arrived count
        const uint32_t arrivedCount = state_.arrived.increment();

        if (arrivedCount == threadCount_) {
            // Last thread to arrive - signal all and reset
            {
                std::lock_guard<std::mutex> lock(mutex_);
                state_.signaled.store(true);
            }
            WakeAll();

            // Reset for next use
            state_.arrived.store(0);
            state_.waiting.store(0);
            state_.signaled.store(false);
            state_.generation.increment();
            return true;
        }
        else {
            // Not the last thread - wait with timeout
            std::unique_lock<std::mutex> lock(mutex_);
            state_.waiting.increment();

            bool result;
            if (timeoutMs == 0) {
                // Zero timeout = immediate check
                result = state_.signaled.load() ||
                    state_.generation.load() != currentGen;
            }
            else if (timeoutMs == INFINITE) {
                // Infinite timeout = regular wait
                cv_.wait(lock, [this, currentGen]() {
                    return state_.signaled.load() ||
                        state_.generation.load() != currentGen;
                    });
                result = true;
            }
            else {
                // Timed wait
                auto timeout = std::chrono::milliseconds(timeoutMs);
                result = cv_.wait_for(lock, timeout, [this, currentGen]() {
                    return state_.signaled.load() ||
                        state_.generation.load() != currentGen;
                    });
            }

            state_.waiting.decrement();

            // If we timed out, we need to decrement the arrived count
            if (!result) {
                state_.arrived.decrement();
            }

            return result;
        }
    }

    void Barrier::Reset() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);

        // Only reset if no threads are currently waiting
        if (state_.waiting.load() == 0) {
            state_.arrived.store(0);
            state_.signaled.store(false);
            state_.generation.increment();
        }
    }

    uint32_t Barrier::GetWaitingCount() const noexcept {
        return state_.waiting.load();
    }

    uint32_t Barrier::GetThreadCount() const noexcept {
        return threadCount_;
    }

    bool Barrier::IsComplete() const noexcept {
        return state_.arrived.load() >= threadCount_;
    }

    void Barrier::WakeAll() noexcept {
        cv_.notify_all();
    }

    // ============================================================================
    // Lock-Free Queue Implementations
    // ============================================================================

    template<typename T, size_t Capacity>
    bool LockFreeSPSCQueue<T, Capacity>::Push(const T& item) noexcept {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t nextHead = (head + 1) & MASK;

        if (nextHead == tail_.load(std::memory_order_acquire)) {
            return false; // Queue full
        }

        buffer_[head] = item;
        head_.store(nextHead, std::memory_order_release);
        return true;
    }

    template<typename T, size_t Capacity>
    bool LockFreeSPSCQueue<T, Capacity>::Push(T&& item) noexcept {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t nextHead = (head + 1) & MASK;

        if (nextHead == tail_.load(std::memory_order_acquire)) {
            return false; // Queue full
        }

        buffer_[head] = std::move(item);
        head_.store(nextHead, std::memory_order_release);
        return true;
    }

    template<typename T, size_t Capacity>
    bool LockFreeSPSCQueue<T, Capacity>::Pop(T& item) noexcept {
        const size_t tail = tail_.load(std::memory_order_relaxed);

        if (tail == head_.load(std::memory_order_acquire)) {
            return false; // Queue empty
        }

        item = std::move(buffer_[tail]);
        tail_.store((tail + 1) & MASK, std::memory_order_release);
        return true;
    }

    template<typename T, size_t Capacity>
    bool LockFreeSPSCQueue<T, Capacity>::IsEmpty() const noexcept {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    template<typename T, size_t Capacity>
    bool LockFreeSPSCQueue<T, Capacity>::IsFull() const noexcept {
        const size_t head = head_.load(std::memory_order_acquire);
        const size_t tail = tail_.load(std::memory_order_acquire);
        return ((head + 1) & MASK) == tail;
    }

    template<typename T, size_t Capacity>
    size_t LockFreeSPSCQueue<T, Capacity>::Size() const noexcept {
        const size_t head = head_.load(std::memory_order_acquire);
        const size_t tail = tail_.load(std::memory_order_acquire);
        return (head - tail) & MASK;
    }

    // MPMC Queue Implementation
    template<typename T, size_t Capacity>
    LockFreeMPMCQueue<T, Capacity>::LockFreeMPMCQueue() noexcept : head_(0), tail_(0) {
        for (size_t i = 0; i < Capacity; ++i) {
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    template<typename T, size_t Capacity>
    bool LockFreeMPMCQueue<T, Capacity>::Push(const T& item) noexcept {
        size_t head = head_.load(std::memory_order_relaxed);

        while (true) {
            Cell& cell = buffer_[head & MASK];
            size_t sequence = cell.sequence.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(sequence) - static_cast<intptr_t>(head);

            if (diff == 0) {
                if (head_.compare_exchange_weak(head, head + 1, std::memory_order_relaxed)) {
                    cell.data = item;
                    cell.sequence.store(head + 1, std::memory_order_release);
                    return true;
                }
            }
            else if (diff < 0) {
                return false; // Queue full
            }
            else {
                head = head_.load(std::memory_order_relaxed);
            }
        }
    }

    template<typename T, size_t Capacity>
    bool LockFreeMPMCQueue<T, Capacity>::Push(T&& item) noexcept {
        size_t head = head_.load(std::memory_order_relaxed);

        while (true) {
            Cell& cell = buffer_[head & MASK];
            size_t sequence = cell.sequence.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(sequence) - static_cast<intptr_t>(head);

            if (diff == 0) {
                if (head_.compare_exchange_weak(head, head + 1, std::memory_order_relaxed)) {
                    cell.data = std::move(item);
                    cell.sequence.store(head + 1, std::memory_order_release);
                    return true;
                }
            }
            else if (diff < 0) {
                return false; // Queue full
            }
            else {
                head = head_.load(std::memory_order_relaxed);
            }
        }
    }

    template<typename T, size_t Capacity>
    bool LockFreeMPMCQueue<T, Capacity>::Pop(T& item) noexcept {
        size_t tail = tail_.load(std::memory_order_relaxed);

        while (true) {
            Cell& cell = buffer_[tail & MASK];
            size_t sequence = cell.sequence.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(sequence) - static_cast<intptr_t>(tail + 1);

            if (diff == 0) {
                if (tail_.compare_exchange_weak(tail, tail + 1, std::memory_order_relaxed)) {
                    item = std::move(cell.data);
                    cell.sequence.store(tail + Capacity, std::memory_order_release);
                    return true;
                }
            }
            else if (diff < 0) {
                return false; // Queue empty
            }
            else {
                tail = tail_.load(std::memory_order_relaxed);
            }
        }
    }

    template<typename T, size_t Capacity>
    bool LockFreeMPMCQueue<T, Capacity>::IsEmpty() const noexcept {
        size_t tail = tail_.load(std::memory_order_acquire);
        Cell& cell = buffer_[tail & MASK];
        size_t sequence = cell.sequence.load(std::memory_order_acquire);
        return sequence <= tail;
    }

    template<typename T, size_t Capacity>
    size_t LockFreeMPMCQueue<T, Capacity>::Size() const noexcept {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        return head - tail;
    }

    // ============================================================================
    // Profiling Implementation
    // ============================================================================

    ThreadLocal<ThreadProfileData> ThreadProfiler::profileData_{};
    std::vector<ThreadProfileData*> ThreadProfiler::allProfiles_{};
    SpinLock ThreadProfiler::profilesLock_{};

    void ThreadProfiler::BeginProfile([[maybe_unused]] const std::string& name) noexcept {
#ifdef AKH_PROFILE
        // Implementation would track timing start
#endif
    }

    void ThreadProfiler::EndProfile([[maybe_unused]] const std::string& name) noexcept {
#ifdef AKH_PROFILE
        // Implementation would calculate duration and update stats
#endif
    }

    void ThreadProfiler::RecordJobExecution([[maybe_unused]] uint64_t executionTimeMicros) noexcept {
#ifdef AKH_PROFILE
        auto& data = profileData_.Get();
        data.jobsExecuted++;
        data.totalExecutionTime += executionTimeMicros;
#endif
    }

    ThreadProfileData ThreadProfiler::GetCurrentThreadProfile() noexcept {
        return profileData_.Get();
    }

    std::vector<ThreadProfileData> ThreadProfiler::GetAllThreadProfiles() noexcept {
        std::vector<ThreadProfileData> results;
        SpinLockGuard lock(profilesLock_);

        for (auto* profile : allProfiles_) {
            if (profile) {
                results.push_back(*profile);
            }
        }

        return results;
    }

    void ThreadProfiler::ResetProfiles() noexcept {
        SpinLockGuard lock(profilesLock_);

        for (auto* profile : allProfiles_) {
            if (profile) {
                *profile = ThreadProfileData{};
            }
        }
    }

    ProfileScope::ProfileScope(const char* name) noexcept
        : name_(name), startTime_(std::chrono::high_resolution_clock::now()) {
    }

    ProfileScope::~ProfileScope() noexcept {
#ifdef AKH_PROFILE
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime_);
        ThreadProfiler::RecordJobExecution(duration.count());
#endif
    }

    // ============================================================================
    // ThreadManager Implementation
    // ============================================================================

    bool ThreadManager::Initialize(const ThreadingConfig& config) noexcept {
        std::call_once(g_initFlag, [&config]() {
            g_state = new ThreadingState{};
            g_state->config = config;
            g_state->hardwareInfo = HardwareDetector::DetectHardware();

            // Auto-detect worker thread count if not specified
            if (g_state->config.workerThreadCount == 0) {
                g_state->config.workerThreadCount = HardwareDetector::GetRecommendedWorkerThreadCount();
            }

            // Initialize allocators
            try {
                g_state->threadAllocator = std::make_unique<Memory::LinearAllocator>(config.jobAllocatorSize * config.workerThreadCount);
                g_state->jobAllocator = std::make_unique<Memory::LinearAllocator>(sizeof(void*) * config.taskPoolSize);

                g_state->initialized.store(true, std::memory_order_release);
            }
            catch (...) {
                delete g_state;
                g_state = nullptr;
            }
            });

        return g_state && g_state->initialized.load(std::memory_order_acquire);
    }

    void ThreadManager::Shutdown() noexcept {
        if (!g_state) return;

        g_state->initialized.store(false, std::memory_order_release);

        // Join all managed threads
        {
            WriteLockGuard lock(g_state->threadsLock);
            for (auto& thread : g_state->managedThreads) {
                if (thread && thread->IsRunning()) {
                    thread->Join(5000); // 5 second timeout
                }
            }
            g_state->managedThreads.clear();
        }

        // Clear lookup table
        {
            SpinLockGuard lock(g_state->lookupLock);
            g_state->threadLookup.clear();
        }

        delete g_state;
        g_state = nullptr;
    }

    bool ThreadManager::IsInitialized() noexcept {
        return g_state && g_state->initialized.load(std::memory_order_acquire);
    }

    Thread* ThreadManager::CreateThread(const ThreadDesc& desc) noexcept {
        if (!IsInitialized()) return nullptr;

        try {
            WriteLockGuard lock(g_state->threadsLock);
            auto thread = std::make_unique<Thread>(desc);
            Thread* result = thread.get();
            g_state->managedThreads.push_back(std::move(thread));
            return result;
        }
        catch (...) {
            return nullptr;
        }
    }

    bool ThreadManager::DestroyThread(Thread* thread) noexcept {
        if (!IsInitialized() || !thread) return false;

        WriteLockGuard lock(g_state->threadsLock);
        auto it = std::find_if(g_state->managedThreads.begin(), g_state->managedThreads.end(),
            [thread](const auto& ptr) { return ptr.get() == thread; });

        if (it != g_state->managedThreads.end()) {
            if ((*it)->IsRunning()) {
                (*it)->Join(1000); // 1 second timeout
            }
            g_state->managedThreads.erase(it);
            return true;
        }

        return false;
    }

    std::vector<Thread*> ThreadManager::GetAllThreads() noexcept {
        if (!IsInitialized()) return {};

        std::vector<Thread*> result;
        ReadLockGuard lock(g_state->threadsLock);

        result.reserve(g_state->managedThreads.size());
        for (const auto& thread : g_state->managedThreads) {
            result.push_back(thread.get());
        }

        return result;
    }

    Thread* ThreadManager::GetThread(const std::string& name) noexcept {
        if (!IsInitialized()) return nullptr;

        ReadLockGuard lock(g_state->threadsLock);
        for (const auto& thread : g_state->managedThreads) {
            if (thread->GetName() == name) {
                return thread.get();
            }
        }

        return nullptr;
    }

    Thread* ThreadManager::GetCurrentThread() noexcept {
        if (!IsInitialized()) return nullptr;

        SpinLockGuard lock(g_state->lookupLock);
        auto it = g_state->threadLookup.find(std::this_thread::get_id());
        return (it != g_state->threadLookup.end()) ? it->second : nullptr;
    }

    const ThreadingConfig& ThreadManager::GetConfig() noexcept {
        static ThreadingConfig defaultConfig{};
        return g_state ? g_state->config : defaultConfig;
    }

    const HardwareInfo& ThreadManager::GetHardwareInfo() noexcept {
        static HardwareInfo defaultInfo{};
        return g_state ? g_state->hardwareInfo : defaultInfo;
    }

    Memory::IAllocator* ThreadManager::GetThreadAllocator() noexcept {
        return g_state ? g_state->threadAllocator.get() : nullptr;
    }

    Memory::IAllocator* ThreadManager::GetJobAllocator() noexcept {
        return g_state ? g_state->jobAllocator.get() : nullptr;
    }

} // namespace Akhanda::Threading