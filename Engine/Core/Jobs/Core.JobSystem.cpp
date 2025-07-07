// Engine/Core/JobSystem/Akhanda.Core.JobSystem.cpp
module;

#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#undef min
#undef max
#undef Yield
#endif

#include <chrono>
#include "Core/Logging/Core.Logging.hpp"

module Akhanda.Core.JobSystem;

import Akhanda.Core.Threading;
import Akhanda.Core.Memory;
import std;


namespace Akhanda::JobSystem {

    // ============================================================================
    // Internal Job Structure
    // ============================================================================

    struct JobData {
        std::unique_ptr<IJob> job;
        JobHandle handle;
        JobDependency dependencies;
        JobPriority priority = JobPriority::Normal;
        JobCategory category = JobCategory::General;

        // Timing and profiling
        std::chrono::high_resolution_clock::time_point submissionTime;
        std::chrono::high_resolution_clock::time_point executionStartTime;
        std::chrono::high_resolution_clock::time_point completionTime;

        // State management
        std::atomic<bool> isComplete{ false };
        std::atomic<bool> isRunning{ false };
        std::atomic<bool> hasFailed{ false };
        std::atomic<bool> isCancelled{ false };

        // Continuation support for coroutines
        std::vector<std::coroutine_handle<>> continuations;
        Threading::SpinLock continuationsLock;

        JobData(std::unique_ptr<IJob> j, uint64_t id)
            : job(std::move(j))
            , handle(id)
            , submissionTime(std::chrono::high_resolution_clock::now()) {
        }
    };

    // ============================================================================
    // Work-Stealing Queue Implementation
    // ============================================================================

    template<typename T, size_t Capacity>
    class WorkStealingQueue {
        static_assert((Capacity& (Capacity - 1)) == 0, "Capacity must be power of 2");

    public:
        WorkStealingQueue() : top_(0), bottom_(0) {}

        // Push job (only called by owner thread)
        bool Push(T&& item) noexcept {
            const int64_t bottom = bottom_.load(std::memory_order_relaxed);
            const int64_t top = top_.load(std::memory_order_acquire);

            if (bottom - top >= Capacity) {
                return false; // Queue full
            }

            buffer_[bottom & MASK] = std::move(item);
            bottom_.store(bottom + 1, std::memory_order_release);
            return true;
        }

        // Pop job (only called by owner thread)
        bool Pop(T& item) noexcept {
            int64_t bottom = bottom_.load(std::memory_order_relaxed) - 1;
            bottom_.store(bottom, std::memory_order_relaxed);

            int64_t top = top_.load(std::memory_order_relaxed);

            if (top <= bottom) {
                // Queue not empty
                item = std::move(buffer_[bottom & MASK]);

                if (top == bottom) {
                    // Last element - need to compete with stealers
                    if (!top_.compare_exchange_strong(top, top + 1,
                        std::memory_order_seq_cst, std::memory_order_relaxed)) {
                        // Lost race with stealer
                        bottom_.store(bottom + 1, std::memory_order_relaxed);
                        return false;
                    }
                    bottom_.store(bottom + 1, std::memory_order_relaxed);
                }
                return true;
            }
            else {
                // Queue empty
                bottom_.store(bottom + 1, std::memory_order_relaxed);
                return false;
            }
        }

        // Steal job (called by other threads)
        bool Steal(T& item) noexcept {
            int64_t top = top_.load(std::memory_order_acquire);
            int64_t bottom = bottom_.load(std::memory_order_acquire);

            if (top < bottom) {
                // Queue not empty
                item = std::move(buffer_[top & MASK]);

                if (top_.compare_exchange_strong(top, top + 1,
                    std::memory_order_seq_cst, std::memory_order_relaxed)) {
                    return true; // Successfully stolen
                }
            }
            return false; // Nothing to steal or lost race
        }

        bool IsEmpty() const noexcept {
            int64_t bottom = bottom_.load(std::memory_order_relaxed);
            int64_t top = top_.load(std::memory_order_relaxed);
            return top >= bottom;
        }

        size_t Size() const noexcept {
            int64_t bottom = bottom_.load(std::memory_order_relaxed);
            int64_t top = top_.load(std::memory_order_relaxed);
            return std::max(int64_t(0), bottom - top);
        }

    private:
        static constexpr size_t MASK = Capacity - 1;

        alignas(64) std::atomic<int64_t> top_;
        alignas(64) std::atomic<int64_t> bottom_;
        alignas(64) std::array<T, Capacity> buffer_;
    };

    // ============================================================================
    // Worker Thread Implementation
    // ============================================================================

    class WorkerThread {
    public:
        WorkerThread(uint32_t id, JobScheduler::Impl* scheduler)
            : id_(id), scheduler_(scheduler), queue_(), stats_{} {

            Threading::ThreadDesc desc{};
            desc.name = "JobWorker_" + std::to_string(id);
            desc.type = Threading::ThreadType::Worker;
            desc.priority = Threading::ThreadPriority::Normal;

            thread_ = Threading::ThreadManager::CreateThread(desc);
        }

        ~WorkerThread() {
            Stop();
            if (thread_) {
                Threading::ThreadManager::DestroyThread(thread_);
            }
        }

        bool Start() noexcept {
            if (!thread_) return false;

            return thread_->Start([this]() { WorkerLoop(); });
        }

        void Stop() noexcept {
            running_.store(false, std::memory_order_relaxed);
            if (thread_ && thread_->IsRunning()) {
                thread_->Join(5000); // 5 second timeout
            }
        }

        bool IsRunning() const noexcept {
            return running_.load(std::memory_order_acquire);
        }

        bool PushJob(std::shared_ptr<JobData> job) noexcept {
            return queue_.Push(std::move(job));
        }

        bool StealJob(std::shared_ptr<JobData>& job) noexcept {
            return queue_.Steal(job);
        }

        const JobScheduler::WorkerStats& GetStats() const noexcept {
            return stats_;
        }

        void ResetStats() noexcept {
            stats_ = {};
            stats_.threadId = id_;
            stats_.threadName = thread_ ? thread_->GetName() : "Unknown";
        }

    private:
        void WorkerLoop() noexcept;
        bool ExecuteJob(std::shared_ptr<JobData> job) noexcept;
        bool TryStealWork() noexcept;

        uint32_t id_;
        JobScheduler::Impl* scheduler_;
        Threading::Thread* thread_;
        std::atomic<bool> running_{ true };

        WorkStealingQueue<std::shared_ptr<JobData>, 1024> queue_;
        JobScheduler::WorkerStats stats_;

        // Profiling
        std::chrono::high_resolution_clock::time_point lastIdleTime_;
        uint64_t totalIdleTime_ = 0;
    };

    // ============================================================================
    // JobScheduler::Impl - Main implementation
    // ============================================================================

    class JobScheduler::Impl {
    public:
        explicit Impl(const JobSystemConfig& config)
            : config_(config)
            , jobCounter_(1) // Start from 1, 0 is invalid
            , running_(false) {

            // Initialize allocators
            auto* threadAllocator = Threading::ThreadManager::GetThreadAllocator();
            auto* jobAllocator = Threading::ThreadManager::GetJobAllocator();

            if (!threadAllocator || !jobAllocator) {
                throw std::runtime_error("Threading system not initialized");
            }

            // Initialize job storage
            jobs_.reserve(config_.maxJobs);
            completedJobs_.reserve(config_.maxJobs / 4);

            // Initialize worker threads
            const uint32_t workerCount = Threading::ThreadManager::GetConfig().workerThreadCount;
            workers_.reserve(workerCount);

            for (uint32_t i = 0; i < workerCount; ++i) {
                workers_.emplace_back(std::make_unique<WorkerThread>(i, this));
            }

            {
                std::string msg = std::format(
                    "JobScheduler initialized with {} worker threads, {} max jobs",
                    workerCount, config_.maxJobs);
                Logging::Channels::Engine().Info(msg);
            }
        }

        ~Impl() {
            Shutdown();
        }

        bool StartWorkers() noexcept {
            if (running_.load(std::memory_order_acquire)) {
                return true; // Already running
            }

            // Start all worker threads
            for (auto& worker : workers_) {
                if (!worker->Start()) {
                    Logging::Channels::Engine().ErrorFormat("Failed to start worker thread {}", worker->GetStats().threadId);
                    return false;
                }
            }

            running_.store(true, std::memory_order_release);

            Logging::Channels::Engine().InfoFormat("JobScheduler started with {} workers",
                workers_.size());
            return true;
        }

        void Shutdown() noexcept {
            if (!running_.load(std::memory_order_acquire)) {
                return; // Already shut down
            }

            running_.store(false, std::memory_order_release);

            // Wait for all jobs to complete
            WaitForAll();

            // Stop all worker threads
            for (auto& worker : workers_) {
                worker->Stop();
            }

            // Clear job storage
            {
                Threading::WriteLockGuard lock(jobsLock_);
                jobs_.clear();
                completedJobs_.clear();
            }

            Logging::Channels::Engine().Info("JobScheduler shut down");
        }

        JobHandle SubmitJob(std::unique_ptr<IJob> job,
            const JobDependency& dependencies,
            JobPriority priority) noexcept {

            if (!running_.load(std::memory_order_acquire)) {
                return JobHandle{}; // Invalid handle
            }

            try {
                // Create job data
                const uint64_t jobId = jobCounter_.increment();
                auto jobData = std::make_shared<JobData>(std::move(job), jobId);
                jobData->dependencies = dependencies;
                jobData->priority = priority;
                jobData->category = jobData->job->GetCategory();

                // Store job
                {
                    Threading::WriteLockGuard lock(jobsLock_);
                    jobs_[jobId] = jobData;
                }

                // Try to schedule immediately if no dependencies
                if (dependencies.IsEmpty() || dependencies.AreAllComplete()) {
                    ScheduleJob(jobData);
                }
                else {
                    // Add to pending jobs for dependency resolution
                    Threading::SpinLockGuard lock(pendingJobsLock_);
                    pendingJobs_.push_back(jobData);
                }

                // Update stats
                stats_.totalJobsSubmitted++;

                return JobHandle(jobId);

            }
            catch (...) {
                Logging::Channels::Engine().ErrorFormat("Failed to submit job: {}",
                    job ? job->GetName() : "Unknown");
                return JobHandle{};
            }
        }

        void WaitForJob(const JobHandle& handle) noexcept {
            auto jobData = GetJobData(handle);
            if (!jobData) return;

            // Busy wait with yielding
            while (!jobData->isComplete.load(std::memory_order_acquire)) {
                // Try to do work while waiting
                TryExecutePendingWork();
                Threading::Thread::Yield();
            }
        }

        void WaitForAll() noexcept {
            // Wait until all jobs are complete
            while (GetActiveJobCount() > 0) {
                TryExecutePendingWork();
                Threading::Thread::Sleep(1);
            }
        }

        bool TryWaitForJob(const JobHandle& handle, uint32_t timeoutMs) noexcept {
            auto jobData = GetJobData(handle);
            if (!jobData) return true; // Invalid job is "complete"

            const auto startTime = std::chrono::steady_clock::now();
            const auto timeout = std::chrono::milliseconds(timeoutMs);

            while (!jobData->isComplete.load(std::memory_order_acquire)) {
                if (timeoutMs > 0 && (std::chrono::steady_clock::now() - startTime) >= timeout) {
                    return false; // Timeout
                }

                TryExecutePendingWork();
                Threading::Thread::Yield();
            }

            return true;
        }

        std::shared_ptr<JobData> GetJobData(const JobHandle& handle) noexcept {
            if (!handle.IsValid()) return nullptr;

            Threading::ReadLockGuard lock(jobsLock_);
            auto it = jobs_.find(handle.GetId());
            return (it != jobs_.end()) ? it->second : nullptr;
        }

        void ScheduleJob(std::shared_ptr<JobData> jobData) noexcept {
            // Find least loaded worker thread
            size_t bestWorker = 0;
            size_t minLoad = SIZE_MAX;

            for (size_t i = 0; i < workers_.size(); ++i) {
                // Simple load balancing based on queue size
                // In a real implementation, you might want more sophisticated metrics
                if (i < minLoad) {
                    bestWorker = i;
                    minLoad = i;
                }
            }

            // Try to push to selected worker
            if (!workers_[bestWorker]->PushJob(jobData)) {
                // Worker queue full, try others
                for (size_t i = 0; i < workers_.size(); ++i) {
                    if (workers_[i]->PushJob(jobData)) {
                        return;
                    }
                }

                // All queues full, add to overflow queue
                Threading::SpinLockGuard lock(overflowQueueLock_);
                overflowQueue_.push_back(jobData);
            }
        }

        void ProcessDependencies() noexcept {
            Threading::SpinLockGuard lock(pendingJobsLock_);

            auto it = pendingJobs_.begin();
            while (it != pendingJobs_.end()) {
                if ((*it)->dependencies.AreAllComplete()) {
                    ScheduleJob(*it);
                    it = pendingJobs_.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        void TryExecutePendingWork() noexcept {
            // Process dependencies
            ProcessDependencies();

            // Process overflow queue
            {
                Threading::SpinLockGuard lock(overflowQueueLock_);
                if (!overflowQueue_.empty()) {
                    auto job = overflowQueue_.front();
                    overflowQueue_.pop_front();
                    lock.~lock_guard();

                    ScheduleJob(job);
                }
            }
        }

        bool TryStealWork(WorkerThread* thief) noexcept {
            for (auto& worker : workers_) {
                if (worker.get() == thief) continue; // Don't steal from yourself

                std::shared_ptr<JobData> job;
                if (worker->StealJob(job)) {
                    // Execute stolen job
                    if (ExecuteJobInternal(job)) {
                        // Update stealer stats
                        auto& stats = const_cast<JobScheduler::WorkerStats&>(thief->GetStats());
                        stats.jobsStolen++;
                        return true;
                    }
                }
            }
            return false;
        }

        bool ExecuteJobInternal(std::shared_ptr<JobData> jobData) noexcept {
            if (!jobData || jobData->isCancelled.load(std::memory_order_acquire)) {
                return false;
            }

            Threading::ProfileScope _prof_scope(jobData->job->GetName());

            // Mark as running
            jobData->isRunning.store(true, std::memory_order_release);
            jobData->executionStartTime = std::chrono::high_resolution_clock::now();

            try {
                // Execute the job
                jobData->job->Execute();

                // Mark as complete
                jobData->completionTime = std::chrono::high_resolution_clock::now();
                jobData->isComplete.store(true, std::memory_order_release);
                jobData->isRunning.store(false, std::memory_order_release);

                // Notify any waiting coroutines
                NotifyContinuations(jobData);

                // Update stats
                const auto executionTime = std::chrono::duration_cast<std::chrono::microseconds>(
                    jobData->completionTime - jobData->executionStartTime).count();

                stats_.totalJobsCompleted++;
                stats_.totalExecutionTime += executionTime;
                stats_.averageJobTime = stats_.totalExecutionTime / stats_.totalJobsCompleted;

                // Move to completed jobs
                {
                    Threading::SpinLockGuard lock(completedJobsLock_);
                    completedJobs_.push_back(jobData);
                }

                return true;

            }
            catch (const std::exception& e) {
                Logging::Channels::Engine().ErrorFormat("Job '{}' failed with exception: {}",
                    jobData->job->GetName(), e.what());

                jobData->hasFailed.store(true, std::memory_order_release);
                jobData->isComplete.store(true, std::memory_order_release);
                jobData->isRunning.store(false, std::memory_order_release);

                stats_.totalJobsFailed++;

                return false;
            }
            catch (...) {
                Logging::Channels::Engine().ErrorFormat("Job '{}' failed with unknown exception",
                    jobData->job->GetName());

                jobData->hasFailed.store(true, std::memory_order_release);
                jobData->isComplete.store(true, std::memory_order_release);
                jobData->isRunning.store(false, std::memory_order_release);

                stats_.totalJobsFailed++;

                return false;
            }
        }

        void NotifyContinuations(std::shared_ptr<JobData> jobData) noexcept {
            Threading::SpinLockGuard lock(jobData->continuationsLock);

            for (auto& continuation : jobData->continuations) {
                if (continuation) {
                    continuation.resume();
                }
            }

            jobData->continuations.clear();
        }

        void RegisterContinuation(const JobHandle& handle, std::coroutine_handle<> continuation) noexcept {
            auto jobData = GetJobData(handle);
            if (!jobData) {
                // Job doesn't exist or is already complete
                continuation.resume();
                return;
            }

            Threading::SpinLockGuard lock(jobData->continuationsLock);

            if (jobData->isComplete.load(std::memory_order_acquire)) {
                // Job completed while we were acquiring the lock
                lock.~lock_guard();
                continuation.resume();
            }
            else {
                jobData->continuations.push_back(continuation);
            }
        }

        // Getters
        size_t GetActiveJobCount() const noexcept {
            Threading::ReadLockGuard lock(jobsLock_);
            return jobs_.size() - completedJobs_.size();
        }

        size_t GetPendingJobCount() const noexcept {
            Threading::SpinLockGuard lock(pendingJobsLock_);
            return pendingJobs_.size();
        }

        size_t GetCompletedJobCount() const noexcept {
            Threading::SpinLockGuard lock(completedJobsLock_);
            return completedJobs_.size();
        }

        const JobSystemConfig& GetConfig() const noexcept { return config_; }

        const PerformanceStats& GetStats() const noexcept { return stats_; }

        std::vector<WorkerStats> GetWorkerStats() const noexcept {
            std::vector<WorkerStats> result;
            result.reserve(workers_.size());

            for (const auto& worker : workers_) {
                result.push_back(worker->GetStats());
            }

            return result;
        }

        // Friend access for WorkerThread
        friend class WorkerThread;

    private:
        JobSystemConfig config_;
        Threading::AtomicCounter<uint64_t> jobCounter_;
        std::atomic<bool> running_;

        // Job storage
        std::unordered_map<uint64_t, std::shared_ptr<JobData>> jobs_;
        mutable Threading::ReadWriteLock jobsLock_;

        std::vector<std::shared_ptr<JobData>> pendingJobs_;
        mutable Threading::SpinLock pendingJobsLock_;

        std::vector<std::shared_ptr<JobData>> completedJobs_;
        mutable Threading::SpinLock completedJobsLock_;

        std::deque<std::shared_ptr<JobData>> overflowQueue_;
        mutable Threading::SpinLock overflowQueueLock_;

        // Worker threads
        std::vector<std::unique_ptr<WorkerThread>> workers_;

        // Statistics
        mutable PerformanceStats stats_{};

        friend class JobScheduler;
        friend class JobHandle;
    };

    // ============================================================================
    // WorkerThread::WorkerLoop Implementation
    // ============================================================================

    void WorkerThread::WorkerLoop() noexcept {
        Threading::ProfileScope _prof_scope("WorkerThread::Loop");

        stats_.threadId = id_;
        stats_.threadName = thread_->GetName();

        auto idleStart = std::chrono::high_resolution_clock::now();

        while (running_.load(std::memory_order_acquire)) {
            std::shared_ptr<JobData> job;
            bool foundWork = false;

            // Try to get work from our own queue first
            if (queue_.Pop(job)) {
                foundWork = true;
            }
            // Try to steal work from other threads
            else if (scheduler_->config_.enableWorkStealing && TryStealWork()) {
                foundWork = true;
            }
            // Try to get work from overflow queue
            else {
                scheduler_->TryExecutePendingWork();
            }

            if (foundWork && job) {
                // Track idle time
                auto now = std::chrono::high_resolution_clock::now();
                totalIdleTime_ += std::chrono::duration_cast<std::chrono::microseconds>(
                    now - idleStart).count();

                // Execute job
                if (ExecuteJob(job)) {
                    stats_.jobsExecuted++;
                }

                idleStart = std::chrono::high_resolution_clock::now();
            }
            else {
                // No work found, yield briefly
                Threading::Thread::Yield();
            }
        }

        // Final idle time calculation
        auto now = std::chrono::high_resolution_clock::now();
        totalIdleTime_ += std::chrono::duration_cast<std::chrono::microseconds>(
            now - idleStart).count();

        stats_.idleTime = totalIdleTime_;
    }

    bool WorkerThread::ExecuteJob(std::shared_ptr<JobData> job) noexcept {
        return scheduler_->ExecuteJobInternal(job);
    }

    bool WorkerThread::TryStealWork() noexcept {
        stats_.stealAttempts++;
        return scheduler_->TryStealWork(this);
    }

    // ============================================================================
    // JobHandle Implementation
    // ============================================================================

    JobHandle::JobHandle(uint64_t id) noexcept : id_(id) {}

    bool JobHandle::IsValid() const noexcept {
        return id_ != 0;
    }

    bool JobHandle::IsComplete() const noexcept {
        if (!IsValid()) return true;

        auto& scheduler = JobScheduler::Instance();
        auto jobData = scheduler.pImpl_->GetJobData(*this);
        return jobData ? jobData->isComplete.load(std::memory_order_acquire) : true;
    }

    bool JobHandle::IsRunning() const noexcept {
        if (!IsValid()) return false;

        auto& scheduler = JobScheduler::Instance();
        auto jobData = scheduler.pImpl_->GetJobData(*this);
        return jobData ? jobData->isRunning.load(std::memory_order_acquire) : false;
    }

    bool JobHandle::HasFailed() const noexcept {
        if (!IsValid()) return false;

        auto& scheduler = JobScheduler::Instance();
        auto jobData = scheduler.pImpl_->GetJobData(*this);
        return jobData ? jobData->hasFailed.load(std::memory_order_acquire) : false;
    }

    void JobHandle::Wait() const noexcept {
        JobScheduler::Instance().WaitForJob(*this);
    }

    bool JobHandle::TryWait(uint32_t timeoutMs) const noexcept {
        return JobScheduler::Instance().TryWaitForJob(*this, timeoutMs);
    }

    uint64_t JobHandle::GetId() const noexcept {
        return id_;
    }

    const char* JobHandle::GetName() const noexcept {
        if (!IsValid()) return "Invalid";

        auto& scheduler = JobScheduler::Instance();
        auto jobData = scheduler.pImpl_->GetJobData(*this);
        return jobData && jobData->job ? jobData->job->GetName() : "Unknown";
    }

    JobCategory JobHandle::GetCategory() const noexcept {
        if (!IsValid()) return JobCategory::General;

        auto& scheduler = JobScheduler::Instance();
        auto jobData = scheduler.pImpl_->GetJobData(*this);
        return jobData ? jobData->category : JobCategory::General;
    }

    JobPriority JobHandle::GetPriority() const noexcept {
        if (!IsValid()) return JobPriority::Normal;

        auto& scheduler = JobScheduler::Instance();
        auto jobData = scheduler.pImpl_->GetJobData(*this);
        return jobData ? jobData->priority : JobPriority::Normal;
    }

    uint64_t JobHandle::GetSubmissionTime() const noexcept {
        if (!IsValid()) return 0;

        auto& scheduler = JobScheduler::Instance();
        auto jobData = scheduler.pImpl_->GetJobData(*this);
        if (!jobData) return 0;

        return std::chrono::duration_cast<std::chrono::microseconds>(
            jobData->submissionTime.time_since_epoch()).count();
    }

    uint64_t JobHandle::GetExecutionTime() const noexcept {
        if (!IsValid()) return 0;

        auto& scheduler = JobScheduler::Instance();
        auto jobData = scheduler.pImpl_->GetJobData(*this);
        if (!jobData || !jobData->isComplete.load()) return 0;

        return std::chrono::duration_cast<std::chrono::microseconds>(
            jobData->completionTime - jobData->executionStartTime).count();
    }

    uint64_t JobHandle::GetCompletionTime() const noexcept {
        if (!IsValid()) return 0;

        auto& scheduler = JobScheduler::Instance();
        auto jobData = scheduler.pImpl_->GetJobData(*this);
        if (!jobData || !jobData->isComplete.load()) return 0;

        return std::chrono::duration_cast<std::chrono::microseconds>(
            jobData->completionTime.time_since_epoch()).count();
    }

    bool JobHandle::operator==(const JobHandle& other) const noexcept {
        return id_ == other.id_;
    }

    bool JobHandle::operator!=(const JobHandle& other) const noexcept {
        return id_ != other.id_;
    }

    bool JobHandle::operator<(const JobHandle& other) const noexcept {
        return id_ < other.id_;
    }

    size_t JobHandle::Hash::operator()(const JobHandle& handle) const noexcept {
        return std::hash<uint64_t>{}(handle.id_);
    }

    // ============================================================================
    // JobDependency Implementation
    // ============================================================================

    JobDependency::JobDependency(std::initializer_list<JobHandle> dependencies) noexcept {
        for (const auto& dep : dependencies) {
            AddDependency(dep);
        }
    }

    JobDependency::JobDependency(std::span<const JobHandle> dependencies) noexcept {
        for (const auto& dep : dependencies) {
            AddDependency(dep);
        }
    }

    void JobDependency::AddDependency(const JobHandle& job) noexcept {
        if (!job.IsValid()) return;

        if (!useHeap_ && count_ < MAX_INLINE_DEPENDENCIES) {
            storage_.inline_[count_++] = job;
        }
        else {
            // Switch to heap allocation
            if (!useHeap_) {
                auto* heap = new std::vector<JobHandle>();
                heap->reserve(MAX_INLINE_DEPENDENCIES * 2);

                // Copy inline dependencies
                for (uint8_t i = 0; i < count_; ++i) {
                    heap->push_back(storage_.inline_[i]);
                }

                storage_.heap_ = heap;
                useHeap_ = true;
            }

            storage_.heap_->push_back(job);
            count_++;
        }
    }

    void JobDependency::AddDependencies(std::span<const JobHandle> jobs) noexcept {
        for (const auto& job : jobs) {
            AddDependency(job);
        }
    }

    void JobDependency::RemoveDependency(const JobHandle& job) noexcept {
        if (useHeap_) {
            auto& vec = *storage_.heap_;
            auto it = std::find(vec.begin(), vec.end(), job);
            if (it != vec.end()) {
                vec.erase(it);
                count_--;
            }
        }
        else {
            for (uint8_t i = 0; i < count_; ++i) {
                if (storage_.inline_[i] == job) {
                    // Shift remaining elements
                    for (uint8_t j = i; j < count_ - 1; ++j) {
                        storage_.inline_[j] = storage_.inline_[j + 1];
                    }
                    count_--;
                    break;
                }
            }
        }
    }

    void JobDependency::Clear() noexcept {
        if (useHeap_) {
            delete storage_.heap_;
            useHeap_ = false;
        }
        count_ = 0;
    }

    bool JobDependency::IsEmpty() const noexcept {
        return count_ == 0;
    }

    bool JobDependency::AreAllComplete() const noexcept {
        if (useHeap_) {
            const auto& vec = *storage_.heap_;
            return std::all_of(vec.begin(), vec.end(),
                [](const JobHandle& handle) { return handle.IsComplete(); });
        }
        else {
            for (uint8_t i = 0; i < count_; ++i) {
                if (!storage_.inline_[i].IsComplete()) {
                    return false;
                }
            }
            return true;
        }
    }

    size_t JobDependency::GetCount() const noexcept {
        return count_;
    }

    std::span<const JobHandle> JobDependency::GetDependencies() const noexcept {
        if (useHeap_) {
            const auto& vec = *storage_.heap_;
            return std::span<const JobHandle>(vec.data(), vec.size());
        }
        else {
            return std::span<const JobHandle>(storage_.inline_.data(), count_);
        }
    }

    JobDependency& JobDependency::operator+=(const JobHandle& job) noexcept {
        AddDependency(job);
        return *this;
    }

    JobDependency& JobDependency::operator+=(const JobDependency& other) noexcept {
        AddDependencies(other.GetDependencies());
        return *this;
    }

    // ============================================================================
    // Coroutine Support Implementation
    // ============================================================================

    template<typename T>
    void JobAwaiter<T>::RegisterContinuation() noexcept {
        auto& scheduler = JobScheduler::Instance();
        scheduler.pImpl_->RegisterContinuation(handle_, continuation_);
    }

    template<typename T>
    template<typename R>
    R JobAwaiter<T>::GetJobResult() const noexcept {
        // In a full implementation, you'd extract the result from the completed job
        // For now, return default value
        return R{};
    }

    template<typename T>
    JobHandle Task<T>::GetHandle() const noexcept {
        // In a full implementation, you'd track the coroutine's job handle
        // For now, return invalid handle
        return JobHandle{};
    }

    // ============================================================================
    // JobScheduler Implementation
    // ============================================================================

    namespace {
        std::unique_ptr<JobScheduler> g_instance;
        std::once_flag g_initFlag;
    }

    bool JobScheduler::Initialize(const JobSystemConfig& config) noexcept {
        try {
            std::call_once(g_initFlag, [&config]() {
                g_instance = std::unique_ptr<JobScheduler>(new JobScheduler());
                g_instance->pImpl_ = std::make_unique<Impl>(config);
                });

            return g_instance && g_instance->pImpl_->StartWorkers();
        }
        catch (...) {
            return false;
        }
    }

    void JobScheduler::Shutdown() noexcept {
        if (g_instance) {
            g_instance->pImpl_->Shutdown();
            g_instance.reset();
        }
    }

    bool JobScheduler::IsInitialized() noexcept {
        return g_instance != nullptr;
    }

    JobScheduler& JobScheduler::Instance() noexcept {
        return *g_instance;
    }

    JobHandle JobScheduler::SubmitJob(std::unique_ptr<IJob> job,
        const JobDependency& dependencies,
        JobPriority priority) noexcept {
        return pImpl_->SubmitJob(std::move(job), dependencies, priority);
    }

    void JobScheduler::WaitForJob(const JobHandle& job) noexcept {
        pImpl_->WaitForJob(job);
    }

    void JobScheduler::WaitForJobs(std::span<const JobHandle> jobs) noexcept {
        for (const auto& job : jobs) {
            WaitForJob(job);
        }
    }

    void JobScheduler::WaitForAll() noexcept {
        pImpl_->WaitForAll();
    }

    bool JobScheduler::TryWaitForJob(const JobHandle& job, uint32_t timeoutMs) noexcept {
        return pImpl_->TryWaitForJob(job, timeoutMs);
    }

    size_t JobScheduler::GetActiveJobCount() const noexcept {
        return pImpl_->GetActiveJobCount();
    }

    size_t JobScheduler::GetPendingJobCount() const noexcept {
        return pImpl_->GetPendingJobCount();
    }

    size_t JobScheduler::GetCompletedJobCount() const noexcept {
        return pImpl_->GetCompletedJobCount();
    }

    uint32_t JobScheduler::GetWorkerThreadCount() const noexcept {
        return static_cast<uint32_t>(pImpl_->workers_.size());
    }

    const JobSystemConfig& JobScheduler::GetConfig() const noexcept {
        return pImpl_->GetConfig();
    }

    JobScheduler::PerformanceStats JobScheduler::GetPerformanceStats() const noexcept {
        return pImpl_->GetStats();
    }

    std::vector<JobScheduler::WorkerStats> JobScheduler::GetWorkerStats() const noexcept {
        return pImpl_->GetWorkerStats();
    }

} // namespace Akhanda::JobSystem