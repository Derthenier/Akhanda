module;

#include <cstdint>

export module Akhanda.Core.Threading;

import Akhanda.Core.Memory;
import std;

export namespace Akhanda::Threading {

    // ============================================================================
    // Threading Configuration
    // ============================================================================

    struct ThreadingConfig {
        uint32_t workerThreadCount = 0;        // 0 = auto-detect
        uint32_t jobQueueSize = 8192;          // Per-thread job queue size
        uint32_t taskPoolSize = 16384;         // Global task pool size
        bool enableCoroutines = true;          // Enable coroutine support
        bool enableProfiling = true;           // Enable profiling markers
        size_t jobAllocatorSize = 1024 * 1024; // 1MB per thread for job allocation
    };

    // ============================================================================
    // Hardware Detection
    // ============================================================================

    struct HardwareInfo {
        uint32_t logicalCores;
        uint32_t physicalCores;
        uint32_t numaNodes;
        bool hyperthreadingEnabled;
        std::vector<uint32_t> coreAffinityMasks;
    };

    class HardwareDetector {
    public:
        static HardwareInfo DetectHardware() noexcept;
        static uint32_t GetRecommendedWorkerThreadCount() noexcept;
        static uint64_t GetThreadAffinityMask(uint32_t threadIndex) noexcept;
    };

    // ============================================================================
    // Thread-Safe Primitives
    // ============================================================================

    // High-performance spin lock for short critical sections
    class SpinLock {
    public:
        SpinLock() noexcept = default;
        SpinLock(const SpinLock&) = delete;
        SpinLock& operator=(const SpinLock&) = delete;

        void lock() noexcept;
        bool try_lock() noexcept;
        void unlock() noexcept;

    private:
        std::atomic_flag flag_{};
    };

    // RAII lock guard for SpinLock
    using SpinLockGuard = std::lock_guard<SpinLock>;

    // Read-Write Lock for frequent reads, infrequent writes
    class ReadWriteLock {
    public:
        ReadWriteLock() noexcept = default;
        ReadWriteLock(const ReadWriteLock&) = delete;
        ReadWriteLock& operator=(const ReadWriteLock&) = delete;

        void lock_shared() noexcept;
        void unlock_shared() noexcept;
        void lock() noexcept;
        void unlock() noexcept;
        bool try_lock_shared() noexcept;
        bool try_lock() noexcept;

    private:
        mutable std::shared_mutex mutex_;
    };

    using ReadLockGuard = std::shared_lock<ReadWriteLock>;
    using WriteLockGuard = std::unique_lock<ReadWriteLock>;

    // Lock-free atomic counter with overflow protection
    template<typename T>
    class AtomicCounter {
        static_assert(std::is_integral_v<T>, "AtomicCounter requires integral type");

    public:
        explicit AtomicCounter(T initial = T{ 0 }) noexcept : value_(initial) {}

        T load(std::memory_order order = std::memory_order_seq_cst) const noexcept {
            return value_.load(order);
        }

        void store(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
            value_.store(desired, order);
        }

        T fetch_add(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
            return value_.fetch_add(arg, order);
        }

        T fetch_sub(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
            return value_.fetch_sub(arg, order);
        }

        T increment(std::memory_order order = std::memory_order_seq_cst) noexcept {
            return fetch_add(T{ 1 }, order) + T{ 1 };
        }

        T decrement(std::memory_order order = std::memory_order_seq_cst) noexcept {
            return fetch_sub(T{ 1 }, order) - T{ 1 };
        }

        bool compare_exchange_weak(T& expected, T desired,
            std::memory_order order = std::memory_order_seq_cst) noexcept {
            return value_.compare_exchange_weak(expected, desired, order);
        }

        bool compare_exchange_strong(T& expected, T desired,
            std::memory_order order = std::memory_order_seq_cst) noexcept {
            return value_.compare_exchange_strong(expected, desired, order);
        }

        operator T() const noexcept { return load(); }

    private:
        std::atomic<T> value_;
    };

    // ============================================================================
    // Thread Management
    // ============================================================================

    enum class ThreadPriority : int32_t {
        Idle = -2,
        Low = -1,
        Normal = 0,
        High = 1,
        Critical = 2
    };

    enum class ThreadType : uint32_t {
        Main = 0,
        Worker = 1,
        Render = 2,
        Resource = 3,
        Audio = 4,
        Network = 5,
        AI = 6,
        Custom = 7
    };

    struct ThreadDesc {
        std::string name;
        ThreadType type = ThreadType::Worker;
        ThreadPriority priority = ThreadPriority::Normal;
        uint32_t stackSize = 0; // 0 = default
        uint64_t affinityMask = 0; // 0 = no affinity
        Memory::IAllocator* allocator = nullptr; // nullptr = use default
    };

    class Thread {
    public:
        Thread() noexcept = default;
        explicit Thread(const ThreadDesc& desc) noexcept;
        ~Thread() noexcept;

        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;
        Thread(Thread&& other) noexcept;
        Thread& operator=(Thread&& other) noexcept;

        template<typename F, typename... Args>
        bool Start(F&& func, Args&&... args) noexcept;

        bool Join(uint32_t timeoutMs = 0xFFFFFFFF) noexcept;
        bool Detach() noexcept;
        bool IsRunning() const noexcept;
        bool SetPriority(ThreadPriority priority) noexcept;
        bool SetAffinity(uint64_t affinityMask) noexcept;

        std::thread::id GetId() const noexcept;
        const std::string& GetName() const noexcept;
        ThreadType GetType() const noexcept;

        static Thread* GetCurrent() noexcept;
        static void Sleep(uint32_t milliseconds) noexcept;
        static void Yield() noexcept;

    private:
        std::unique_ptr<std::thread> thread_;
        ThreadDesc desc_;
        std::atomic<bool> running_{ false };
    };

    // ============================================================================
    // Thread-Local Storage
    // ============================================================================

    template<typename T>
    class ThreadLocal {
    public:
        ThreadLocal() noexcept = default;
        explicit ThreadLocal(const T& initial) noexcept : initial_(initial) {}

        T& Get() noexcept {
            thread_local T instance{ initial_ };
            return instance;
        }

        const T& Get() const noexcept {
            thread_local T instance{ initial_ };
            return instance;
        }

        void Set(const T& value) noexcept {
            Get() = value;
        }

        operator T& () noexcept { return Get(); }
        operator const T& () const noexcept { return Get(); }

        T& operator*() noexcept { return Get(); }
        const T& operator*() const noexcept { return Get(); }

        T* operator->() noexcept { return &Get(); }
        const T* operator->() const noexcept { return &Get(); }

    private:
        T initial_{};
    };

    // ============================================================================
    // Synchronization Primitives
    // ============================================================================

    // Event for thread synchronization
    class Event {
    public:
        Event(bool manualReset = false, bool initialState = false) noexcept;
        ~Event() noexcept = default;

        Event(const Event&) = delete;
        Event& operator=(const Event&) = delete;

        void Set() noexcept;
        void Reset() noexcept;
        bool Wait(uint32_t timeoutMs = 0xFFFFFFFF) noexcept;
        bool IsSet() const noexcept;

    private:
        mutable std::condition_variable cv_;
        mutable std::mutex mutex_;
        bool state_;
        bool manualReset_;
    };

    // Semaphore for resource counting
    class Semaphore {
    public:
        explicit Semaphore(uint32_t initialCount = 0, uint32_t maxCount = UINT32_MAX) noexcept;
        ~Semaphore() noexcept = default;

        Semaphore(const Semaphore&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;

        bool Acquire(uint32_t timeoutMs = 0xFFFFFFFF) noexcept;
        void Release(uint32_t count = 1) noexcept;
        uint32_t GetCount() const noexcept;

    private:
        std::counting_semaphore<UINT32_MAX> semaphore_;
        AtomicCounter<uint32_t> count_;
    };

    // Custom barrier for thread synchronization with timeout support
    class Barrier {
    public:
        explicit Barrier(uint32_t threadCount) noexcept;
        ~Barrier() noexcept = default;

        Barrier(const Barrier&) = delete;
        Barrier& operator=(const Barrier&) = delete;

        // Wait indefinitely for all threads to arrive
        void Wait() noexcept;

        // Try to wait with timeout (milliseconds)
        bool TryWait(uint32_t timeoutMs = 0) noexcept;

        // Reset barrier for reuse (all threads must have left previous wait)
        void Reset() noexcept;

        // Get current state
        uint32_t GetWaitingCount() const noexcept;
        uint32_t GetThreadCount() const noexcept;
        bool IsComplete() const noexcept;

    private:
        struct BarrierState {
            AtomicCounter<uint32_t> arrived{ 0 };
            AtomicCounter<uint32_t> waiting{ 0 };
            AtomicCounter<uint32_t> generation{ 0 }; // Prevents ABA problem
            AtomicCounter<bool> signaled{ false };
        };

        const uint32_t threadCount_;
        mutable std::mutex mutex_;
        mutable std::condition_variable cv_;
        BarrierState state_;

        void WakeAll() noexcept;
    };

    // ============================================================================
    // Lock-Free Data Structures
    // ============================================================================

    // Lock-free SPSC (Single Producer Single Consumer) queue
    template<typename T, size_t Capacity>
    class LockFreeSPSCQueue {
        static_assert((Capacity& (Capacity - 1)) == 0, "Capacity must be power of 2");

    public:
        LockFreeSPSCQueue() noexcept : head_(0), tail_(0) {}
        ~LockFreeSPSCQueue() noexcept = default;

        LockFreeSPSCQueue(const LockFreeSPSCQueue&) = delete;
        LockFreeSPSCQueue& operator=(const LockFreeSPSCQueue&) = delete;

        bool Push(const T& item) noexcept;
        bool Push(T&& item) noexcept;
        bool Pop(T& item) noexcept;
        bool IsEmpty() const noexcept;
        bool IsFull() const noexcept;
        size_t Size() const noexcept;

    private:
        static constexpr size_t MASK = Capacity - 1;

        alignas(64) std::atomic<size_t> head_;
        alignas(64) std::atomic<size_t> tail_;
        alignas(64) std::array<T, Capacity> buffer_;
    };

    // Lock-free MPMC (Multi Producer Multi Consumer) queue
    template<typename T, size_t Capacity>
    class LockFreeMPMCQueue {
        static_assert((Capacity& (Capacity - 1)) == 0, "Capacity must be power of 2");

    public:
        LockFreeMPMCQueue() noexcept;
        ~LockFreeMPMCQueue() noexcept = default;

        LockFreeMPMCQueue(const LockFreeMPMCQueue&) = delete;
        LockFreeMPMCQueue& operator=(const LockFreeMPMCQueue&) = delete;

        bool Push(const T& item) noexcept;
        bool Push(T&& item) noexcept;
        bool Pop(T& item) noexcept;
        bool IsEmpty() const noexcept;
        size_t Size() const noexcept;

    private:
        struct Cell {
            std::atomic<size_t> sequence;
            T data;
        };

        static constexpr size_t MASK = Capacity - 1;

        alignas(64) std::atomic<size_t> head_;
        alignas(64) std::atomic<size_t> tail_;
        alignas(64) std::array<Cell, Capacity> buffer_;
    };

    // ============================================================================
    // Profiling Integration
    // ============================================================================

    struct ThreadProfileData {
        std::string threadName;
        ThreadType threadType;
        uint64_t jobsExecuted = 0;
        uint64_t totalExecutionTime = 0; // microseconds
        uint64_t idleTime = 0; // microseconds
        double cpuUtilization = 0.0; // percentage
    };

    class ThreadProfiler {
    public:
        static void BeginProfile(const std::string& name) noexcept;
        static void EndProfile(const std::string& name) noexcept;
        static void RecordJobExecution(uint64_t executionTimeMicros) noexcept;
        static ThreadProfileData GetCurrentThreadProfile() noexcept;
        static std::vector<ThreadProfileData> GetAllThreadProfiles() noexcept;
        static void ResetProfiles() noexcept;

    private:
        static ThreadLocal<ThreadProfileData> profileData_;
        static std::vector<ThreadProfileData*> allProfiles_;
        static SpinLock profilesLock_;
    };

    // RAII profiling scope
    class ProfileScope {
    public:
        explicit ProfileScope(const char* name) noexcept;
        ~ProfileScope() noexcept;

        ProfileScope(const ProfileScope&) = delete;
        ProfileScope& operator=(const ProfileScope&) = delete;

    private:
        const char* name_;
        std::chrono::high_resolution_clock::time_point startTime_;
    };

    // ============================================================================
    // Thread Manager
    // ============================================================================

    class ThreadManager {
    public:
        static bool Initialize(const ThreadingConfig& config = {}) noexcept;
        static void Shutdown() noexcept;
        static bool IsInitialized() noexcept;

        static Thread* CreateThread(const ThreadDesc& desc) noexcept;
        static bool DestroyThread(Thread* thread) noexcept;
        static std::vector<Thread*> GetAllThreads() noexcept;
        static Thread* GetThread(const std::string& name) noexcept;
        static Thread* GetCurrentThread() noexcept;

        static const ThreadingConfig& GetConfig() noexcept;
        static const HardwareInfo& GetHardwareInfo() noexcept;

        // Memory integration
        static Memory::IAllocator* GetThreadAllocator() noexcept;
        static Memory::IAllocator* GetJobAllocator() noexcept;

    private:
        ThreadManager() = delete;
    };

} // namespace Akhanda::Threading