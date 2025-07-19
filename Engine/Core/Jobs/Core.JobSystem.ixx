// Engine/Core/JobSystem/Core.JobSystem.ixx
module;

#include <cstdint>
#include <type_traits>
#include <span>
#include <initializer_list>
#include <array>
#include <vector>
#include <coroutine>
#include <memory>
#include <string>
#include <unordered_map>

export module Akhanda.Core.JobSystem;

import Akhanda.Core.Threading;
import Akhanda.Core.Memory;

export namespace Akhanda::JobSystem {

    // ============================================================================
    // Forward Declarations
    // ============================================================================

    class JobScheduler;
    class JobHandle;
    class JobDependency;
    template<typename T> class Task;
    template<typename T> class JobAwaiter;

    // ============================================================================
    // Job System Configuration
    // ============================================================================

    struct JobSystemConfig {
        uint32_t maxJobs = 16384;              // Maximum concurrent jobs
        uint32_t maxDependencies = 8192;       // Maximum dependency connections
        uint32_t workerQueueSize = 1024;       // Per-worker queue size
        uint32_t jobMemoryPoolSize = 2 * 1024 * 1024; // 2MB for job allocation
        bool enableWorkStealing = true;        // Enable work-stealing between threads
        bool enableCoroutines = true;          // Enable C++20 coroutine support
        bool enableProfiling = true;           // Enable job profiling
        uint32_t stealAttempts = 3;            // Number of steal attempts before yielding
    };

    // ============================================================================
    // Job Priority and Categories
    // ============================================================================

    enum class JobPriority : uint8_t {
        Critical = 0,   // Engine-critical jobs (main thread, render thread)
        High = 1,       // Important game logic, physics
        Normal = 2,     // General gameplay, AI
        Low = 3,        // Asset loading, background tasks
        Idle = 4        // Cleanup, maintenance tasks
    };

    enum class JobCategory : uint16_t {
        General = 0,
        Rendering = 1,
        Physics = 2,
        Audio = 3,
        AI = 4,
        Networking = 5,
        Resources = 6,
        Animation = 7,
        Scripting = 8,
        Custom = 9
    };

    // ============================================================================
    // Job Interface and Base Types
    // ============================================================================

    // Base job interface - all jobs must implement this
    class IJob {
    public:
        virtual ~IJob() = default;
        virtual void Execute() = 0;
        virtual const char* GetName() const noexcept = 0;
        virtual JobCategory GetCategory() const noexcept { return JobCategory::General; }
        virtual JobPriority GetPriority() const noexcept { return JobPriority::Normal; }
    };

    // Function-based job wrapper
    template<typename F>
    class FunctionJob : public IJob {
    public:
        explicit FunctionJob(F&& func, const char* name = "FunctionJob") noexcept
            : func_(std::forward<F>(func)), name_(name) {
        }

        void Execute() override {
            if constexpr (std::is_void_v<std::invoke_result_t<F>>) {
                func_();
            }
            else {
                result_ = func_();
            }
        }

        const char* GetName() const noexcept override { return name_; }

        // Get result for non-void functions
        template<typename R = std::invoke_result_t<F>>
        std::enable_if_t<!std::is_void_v<R>, R> GetResult() const { return result_; }

    private:
        F func_;
        const char* name_;
        std::conditional_t<std::is_void_v<std::invoke_result_t<F>>, int, std::invoke_result_t<F>> result_{};
    };

    // ============================================================================
    // Job Handle - Represents a submitted job
    // ============================================================================

    class JobHandle {
    public:
        JobHandle() noexcept = default;
        explicit JobHandle(uint64_t id) noexcept;
        ~JobHandle() noexcept = default;

        // Copyable and movable
        JobHandle(const JobHandle&) noexcept = default;
        JobHandle& operator=(const JobHandle&) noexcept = default;
        JobHandle(JobHandle&&) noexcept = default;
        JobHandle& operator=(JobHandle&&) noexcept = default;

        // Status checking
        bool IsValid() const noexcept;
        bool IsComplete() const noexcept;
        bool IsRunning() const noexcept;
        bool HasFailed() const noexcept;

        // Synchronization
        void Wait() const noexcept;
        bool TryWait(uint32_t timeoutMs = 0) const noexcept;

        // Job information
        uint64_t GetId() const noexcept;
        const char* GetName() const noexcept;
        JobCategory GetCategory() const noexcept;
        JobPriority GetPriority() const noexcept;

        // Profiling data
        uint64_t GetSubmissionTime() const noexcept;
        uint64_t GetExecutionTime() const noexcept;
        uint64_t GetCompletionTime() const noexcept;

        // Comparison operators
        bool operator==(const JobHandle& other) const noexcept;
        bool operator!=(const JobHandle& other) const noexcept;
        bool operator<(const JobHandle& other) const noexcept;

        // Hash support for containers
        struct Hash {
            size_t operator()(const JobHandle& handle) const noexcept;
        };

    private:
        uint64_t id_ = 0;
    };

    // ============================================================================
    // Job Dependencies
    // ============================================================================

    class JobDependency {
    public:
        JobDependency() noexcept = default;
        explicit JobDependency(std::initializer_list<JobHandle> dependencies) noexcept;
        explicit JobDependency(std::span<const JobHandle> dependencies) noexcept;
        ~JobDependency() noexcept = default;

        // Copyable and movable
        JobDependency(const JobDependency&) noexcept = default;
        JobDependency& operator=(const JobDependency&) noexcept = default;
        JobDependency(JobDependency&&) noexcept = default;
        JobDependency& operator=(JobDependency&&) noexcept = default;

        // Dependency management
        void AddDependency(const JobHandle& job) noexcept;
        void AddDependencies(std::span<const JobHandle> jobs) noexcept;
        void RemoveDependency(const JobHandle& job) noexcept;
        void Clear() noexcept;

        // Status checking
        bool IsEmpty() const noexcept;
        bool AreAllComplete() const noexcept;
        size_t GetCount() const noexcept;

        // Iteration
        std::span<const JobHandle> GetDependencies() const noexcept;

        // Operators for convenience
        JobDependency& operator+=(const JobHandle& job) noexcept;
        JobDependency& operator+=(const JobDependency& other) noexcept;

    private:
        static constexpr size_t MAX_INLINE_DEPENDENCIES = 8;

        // Small vector optimization for dependencies
        union {
            std::array<JobHandle, MAX_INLINE_DEPENDENCIES> inline_;
            std::vector<JobHandle>* heap_;
        } storage_{};

        uint8_t count_ = 0;
        bool useHeap_ = false;
    };

    // ============================================================================
    // Coroutine Support
    // ============================================================================

    // Job awaiter for coroutine integration
    template<typename T = void>
    class JobAwaiter {
    public:
        explicit JobAwaiter(JobHandle handle) noexcept : handle_(handle) {}

        bool await_ready() const noexcept { return handle_.IsComplete(); }

        void await_suspend(std::coroutine_handle<> continuation) noexcept {
            continuation_ = continuation;
            // Register continuation with job system
            RegisterContinuation();
        }

        T await_resume() const noexcept {
            if constexpr (!std::is_void_v<T>) {
                return GetJobResult<T>();
            }
        }

    private:
        JobHandle handle_;
        std::coroutine_handle<> continuation_;

        void RegisterContinuation() noexcept;

        template<typename R>
        R GetJobResult() const noexcept;
    };

    // Task coroutine type
    template<typename T = void>
    class Task {
    public:
        struct promise_type {
            Task get_return_object() noexcept {
                return Task{ std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            std::suspend_never initial_suspend() noexcept { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }

            void unhandled_exception() noexcept {
                exception_ = std::current_exception();
            }

            template<typename U>
            void return_value(U&& value) noexcept {
                if constexpr (!std::is_void_v<T>) {
                    result_ = std::forward<U>(value);
                }
            }

            void return_void() noexcept requires std::is_void_v<T> {}

            // Await transform for JobHandle
            JobAwaiter<void> await_transform(JobHandle handle) noexcept {
                return JobAwaiter<void>{ handle };
            }

            // Await transform for other Tasks
            template<typename U>
            JobAwaiter<U> await_transform(Task<U>&& task) noexcept {
                return JobAwaiter<U>{ task.GetHandle() };
            }

            std::conditional_t<std::is_void_v<T>, int, T> result_{};
            std::exception_ptr exception_;
        };

        explicit Task(std::coroutine_handle<promise_type> handle) noexcept
            : handle_(handle) {
        }

        ~Task() noexcept {
            if (handle_) {
                handle_.destroy();
            }
        }

        // Non-copyable, movable
        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;
        Task(Task&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}
        Task& operator=(Task&& other) noexcept {
            if (this != &other) {
                if (handle_) handle_.destroy();
                handle_ = std::exchange(other.handle_, {});
            }
            return *this;
        }

        // Get result
        T GetResult() const requires (!std::is_void_v<T>) {
            if (handle_.promise().exception_) {
                std::rethrow_exception(handle_.promise().exception_);
            }
            return handle_.promise().result_;
        }

        void GetResult() const requires std::is_void_v<T> {
            if (handle_.promise().exception_) {
                std::rethrow_exception(handle_.promise().exception_);
            }
        }

        bool IsComplete() const noexcept {
            return handle_ && handle_.done();
        }

        JobHandle GetHandle() const noexcept;

    private:
        std::coroutine_handle<promise_type> handle_;
    };

    // ============================================================================
    // Job Scheduler - Main job system interface
    // ============================================================================

    class JobScheduler {
    public:
        static bool Initialize(const JobSystemConfig& config = {}) noexcept;
        static void Shutdown() noexcept;
        static bool IsInitialized() noexcept;
        static JobScheduler& Instance() noexcept;

        ~JobScheduler() noexcept = default;
        JobScheduler(const JobScheduler&) = delete;
        JobScheduler& operator=(const JobScheduler&) = delete;

        // ========================================================================
        // Job Submission
        // ========================================================================

        // Submit raw job
        JobHandle SubmitJob(std::unique_ptr<IJob> job,
            const JobDependency& dependencies = {},
            JobPriority priority = JobPriority::Normal) noexcept;

        // Submit function as job
        template<typename F>
        JobHandle SubmitJob(F&& func,
            const char* name = "FunctionJob",
            const JobDependency& dependencies = {},
            JobPriority priority = JobPriority::Normal) noexcept {
            auto job = std::make_unique<FunctionJob<std::decay_t<F>>>(
                std::forward<F>(func), name);
            return SubmitJob(std::move(job), dependencies, priority);
        }

        // Submit coroutine task
        template<typename T>
        JobHandle SubmitTask(Task<T> task,
            const JobDependency& dependencies = {},
            JobPriority priority = JobPriority::Normal) noexcept;

        // Batch job submission
        std::vector<JobHandle> SubmitJobs(std::span<std::unique_ptr<IJob>> jobs,
            const JobDependency& dependencies = {},
            JobPriority priority = JobPriority::Normal) noexcept;

        // ========================================================================
        // Job Management
        // ========================================================================

        // Wait for jobs
        void WaitForJob(const JobHandle& job) noexcept;
        void WaitForJobs(std::span<const JobHandle> jobs) noexcept;
        void WaitForAll() noexcept;
        bool TryWaitForJob(const JobHandle& job, uint32_t timeoutMs) noexcept;

        // Job control
        bool CancelJob(const JobHandle& job) noexcept;
        bool SetJobPriority(const JobHandle& job, JobPriority priority) noexcept;

        // Job queries
        size_t GetActiveJobCount() const noexcept;
        size_t GetPendingJobCount() const noexcept;
        size_t GetCompletedJobCount() const noexcept;
        std::vector<JobHandle> GetActiveJobs() const noexcept;

        // ========================================================================
        // Worker Thread Management
        // ========================================================================

        // Thread control
        bool StartWorkers() noexcept;
        void StopWorkers() noexcept;
        bool AreWorkersRunning() const noexcept;

        // Worker queries
        uint32_t GetWorkerThreadCount() const noexcept;
        std::vector<Threading::Thread*> GetWorkerThreads() const noexcept;

        // Per-thread stats
        struct WorkerStats {
            uint32_t threadId;
            std::string threadName;
            uint64_t jobsExecuted = 0;
            uint64_t jobsStolen = 0;
            uint64_t stealAttempts = 0;
            uint64_t idleTime = 0;
            double cpuUtilization = 0.0;
        };

        std::vector<WorkerStats> GetWorkerStats() const noexcept;
        void ResetWorkerStats() noexcept;

        // ========================================================================
        // Configuration and Debugging
        // ========================================================================

        const JobSystemConfig& GetConfig() const noexcept;
        void SetWorkStealingEnabled(bool enabled) noexcept;
        bool IsWorkStealingEnabled() const noexcept;

        // Memory usage
        size_t GetMemoryUsage() const noexcept;
        size_t GetMemoryPeak() const noexcept;
        void DumpMemoryStats() const noexcept;

        // Performance profiling
        struct PerformanceStats {
            uint64_t totalJobsSubmitted = 0;
            uint64_t totalJobsCompleted = 0;
            uint64_t totalJobsFailed = 0;
            uint64_t totalExecutionTime = 0; // microseconds
            uint64_t averageJobTime = 0;     // microseconds
            double throughputPerSecond = 0.0;
            uint32_t currentLoad = 0;        // percentage
        };

        PerformanceStats GetPerformanceStats() const noexcept;
        void ResetPerformanceStats() noexcept;

        // Debug utilities
        void DumpJobGraph() const noexcept;
        void DumpWorkerState() const noexcept;

    private:
        JobScheduler() noexcept = default;

        class Impl;
        std::unique_ptr<Impl> pImpl_;

        friend class JobHandle;
        friend class WorkerThread;
    };

    // ============================================================================
    // Convenience Functions and Macros
    // ============================================================================

    // Global convenience functions
    inline JobScheduler& GetJobScheduler() noexcept {
        return JobScheduler::Instance();
    }

    template<typename F>
    inline JobHandle SubmitJob(F&& func, const char* name = "Job") noexcept {
        return GetJobScheduler().SubmitJob(std::forward<F>(func), name);
    }

    template<typename F>
    inline JobHandle SubmitJobWithDeps(F&& func, const JobDependency& deps,
        const char* name = "Job") noexcept {
        return GetJobScheduler().SubmitJob(std::forward<F>(func), name, deps);
    }

    inline void WaitForJob(const JobHandle& job) noexcept {
        GetJobScheduler().WaitForJob(job);
    }

    inline void WaitForAll() noexcept {
        GetJobScheduler().WaitForAll();
    }
} // namespace Akhanda::JobSystem