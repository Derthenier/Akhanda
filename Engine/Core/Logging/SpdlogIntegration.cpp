// SpdlogIntegration.cpp - Implementation
#include "Engine/Core/Logging/SpdlogIntegration.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/details/thread_pool.h>

#include <filesystem>
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#endif

import Akhanda.Core.Logging;

using namespace Akhanda::Logging;

// =============================================================================
// Global State
// =============================================================================
namespace Akhanda::Logging::Integration {
    namespace {
        std::atomic<bool> g_initialized{ false };
        std::atomic<size_t> g_totalMemoryAllocated{ 0 };
        std::atomic<size_t> g_currentMemoryUsage{ 0 };
        std::atomic<size_t> g_peakMemoryUsage{ 0 };
        std::mutex g_setupMutex;
    }

    // =============================================================================
    // SpdlogSetup Implementation
    // =============================================================================

    SpdlogSetup& SpdlogSetup::Instance() {
        static SpdlogSetup instance;
        return instance;
    }

    SpdlogSetup::~SpdlogSetup() {
        Shutdown();
    }

    void SpdlogSetup::SetPerformanceConfig(const PerformanceConfig& config) {
        std::lock_guard<std::mutex> lock(g_setupMutex);
        perfConfig_ = config;
    }

    void SpdlogSetup::SetPatternConfig(const PatternConfig& config) {
        std::lock_guard<std::mutex> lock(g_setupMutex);
        patternConfig_ = config;
    }

    void SpdlogSetup::SetMemoryIntegrationConfig(const MemoryIntegrationConfig& config) {
        std::lock_guard<std::mutex> lock(g_setupMutex);
        memoryConfig_ = config;
    }

    void SpdlogSetup::Initialize() {
        std::lock_guard<std::mutex> lock(g_setupMutex);
        if (initialized_) return;

        try {
            // Set up async logging if enabled
            if (perfConfig_.asyncQueueSize > 0) {
                SetupAsyncLogging();
            }

            // Configure global spdlog settings
            OptimizeForGameEngine();

            // Set default pattern based on build configuration
            spdlog::set_pattern(GetCurrentPattern());

            // Configure flush policy
            spdlog::flush_every(perfConfig_.flushInterval);

            initialized_ = true;
            g_initialized.store(true, std::memory_order_release);
        }
        catch (const std::exception& e) {
            // Log initialization failure (if possible)
            if (auto console = spdlog::get("console")) {
                console->error("Failed to initialize spdlog: {}", e.what());
            }
            throw;
        }
    }

    void SpdlogSetup::Shutdown() {
        std::lock_guard<std::mutex> lock(g_setupMutex);
        if (!initialized_) return;

        try {
            // Flush all loggers
            spdlog::apply_all([](std::shared_ptr<spdlog::logger> logger) {
                logger->flush();
                });

            // Shutdown thread pool
            if (threadPool_) {
                threadPool_.reset();
            }

            // Shutdown spdlog
            spdlog::shutdown();

            initialized_ = false;
            g_initialized.store(false, std::memory_order_release);
        }
        catch (const std::exception&) {
            // Ignore shutdown errors
        }
    }

    std::string SpdlogSetup::GetCurrentPattern() const {
#ifdef AKH_DEBUG
        return patternConfig_.debugPattern;
#elif defined(AKH_PROFILE)
        return patternConfig_.profilePattern;
#else
        return patternConfig_.releasePattern;
#endif
    }

    std::string SpdlogSetup::GetConsolePattern() const {
        return patternConfig_.consolePattern;
    }

    std::string SpdlogSetup::GetFilePattern() const {
        return patternConfig_.filePattern;
    }

    std::string SpdlogSetup::GetEditorPattern() const {
        return patternConfig_.editorPattern;
    }

    void SpdlogSetup::SetupMemoryTrackingCallbacks() {
        if (!memoryConfig_.enableMemoryTracking) return;

        // Set up callback-based memory tracking
        auto& logManager = LogManager::Instance();

        logManager.SetMemoryTrackingCallbacks(
            // Allocation callback
            [](size_t size, const char*) {
                g_totalMemoryAllocated.fetch_add(size, std::memory_order_relaxed);
                size_t current = g_currentMemoryUsage.fetch_add(size, std::memory_order_relaxed) + size;

                // Update peak usage
                size_t peak = g_peakMemoryUsage.load(std::memory_order_relaxed);
                while (current > peak && !g_peakMemoryUsage.compare_exchange_weak(peak, current, std::memory_order_relaxed)) {
                    // Retry if another thread updated peak
                }
            },
            // Deallocation callback
            [](size_t size, const char*) {
                g_currentMemoryUsage.fetch_sub(size, std::memory_order_relaxed);
            }
        );
    }

    void SpdlogSetup::ClearMemoryTrackingCallbacks() {
        auto& logManager = LogManager::Instance();
        logManager.ClearMemoryTrackingCallbacks();
    }

    void SpdlogSetup::OptimizeForGameEngine() {
        // Set error handler to prevent exceptions
        spdlog::set_error_handler([](const std::string& msg) {
            // Log error to debug output without throwing
#ifdef _WIN32
            OutputDebugStringA(("spdlog error: " + msg + "\n").c_str());
#endif
            });

        // Configure automatic flushing for critical errors
        spdlog::flush_on(spdlog::level::err);

        // Set global level based on build configuration
#ifdef AKH_DEBUG
        spdlog::set_level(spdlog::level::debug);
#elif defined(AKH_PROFILE)
        spdlog::set_level(spdlog::level::info);
#else
        spdlog::set_level(spdlog::level::warn);
#endif
    }

    void SpdlogSetup::SetupAsyncLogging() {
        if (threadPool_) return; // Already set up

        try {
            threadPool_ = std::make_shared<spdlog::details::thread_pool>(
                perfConfig_.asyncQueueSize,
                perfConfig_.asyncThreadCount
            );

            stats_.activeAsyncThreads.store(perfConfig_.asyncThreadCount, std::memory_order_relaxed);

            // Configure thread affinity if requested
            if (perfConfig_.setThreadAffinity) {
                ConfigureThreadAffinity();
            }
        }
        catch (const std::exception& e) {
            // Fall back to synchronous logging if async setup fails
            threadPool_.reset();
            throw std::runtime_error("Failed to setup async logging: " + std::string(e.what()));
        }
    }

    void SpdlogSetup::ConfigureThreadAffinity() {
#ifdef _WIN32
        if (!perfConfig_.setThreadAffinity) return;

        // This would require access to the thread pool's internal threads
        // For now, we'll just set the priority
        if (perfConfig_.setHighPriority) {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
        }
#endif
    }

    std::shared_ptr<spdlog::sinks::sink> SpdlogSetup::CreateConsoleSink() {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sink->set_pattern(GetConsolePattern());
        sink->set_level(spdlog::level::debug);
        return sink;
    }

    std::shared_ptr<spdlog::sinks::sink> SpdlogSetup::CreateFileSink(const std::wstring& filename) {
        // Ensure directory exists
        std::filesystem::path filePath(filename);
        if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path());
        }

        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true);
        sink->set_pattern(GetFilePattern());
        sink->set_level(spdlog::level::debug);
        return sink;
    }

    std::shared_ptr<spdlog::sinks::sink> SpdlogSetup::CreateRotatingFileSink(
        const std::wstring& filename, size_t maxSize, size_t maxFiles) {

        // Ensure directory exists
        std::filesystem::path filePath(filename);
        if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path());
        }

        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filename, maxSize, maxFiles);
        sink->set_pattern(GetFilePattern());
        sink->set_level(spdlog::level::debug);
        return sink;
    }

    std::shared_ptr<spdlog::sinks::sink> SpdlogSetup::CreateMSVCSink() {
        auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
        sink->set_pattern(GetConsolePattern());
        sink->set_level(spdlog::level::debug);
        return sink;
    }

    std::shared_ptr<spdlog::logger> SpdlogSetup::CreateAsyncLogger(
        const std::string& name,
        const std::vector<std::shared_ptr<spdlog::sinks::sink>>& sinks) {

        if (!threadPool_) {
            SetupAsyncLogging();
        }

        auto logger = std::make_shared<spdlog::async_logger>(
            name, sinks.begin(), sinks.end(), threadPool_, spdlog::async_overflow_policy::block);

        logger->set_level(spdlog::level::debug);
        spdlog::register_logger(logger);

        stats_.totalLoggers.fetch_add(1, std::memory_order_relaxed);
        return logger;
    }

    std::shared_ptr<spdlog::logger> SpdlogSetup::CreateSyncLogger(
        const std::string& name,
        const std::vector<std::shared_ptr<spdlog::sinks::sink>>& sinks) {

        auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::debug);
        spdlog::register_logger(logger);

        stats_.totalLoggers.fetch_add(1, std::memory_order_relaxed);
        return logger;
    }

    // =============================================================================
    // AkhandaLogAllocator Implementation
    // =============================================================================

    void* AkhandaLogAllocator::Allocate(size_t size) {
        void* ptr = std::malloc(size);
        if (ptr) {
            g_totalMemoryAllocated.fetch_add(size, std::memory_order_relaxed);
            g_currentMemoryUsage.fetch_add(size, std::memory_order_relaxed);

            // Update peak usage
            size_t current = g_currentMemoryUsage.load(std::memory_order_relaxed);
            size_t peak = g_peakMemoryUsage.load(std::memory_order_relaxed);
            while (current > peak && !g_peakMemoryUsage.compare_exchange_weak(peak, current, std::memory_order_relaxed)) {
                // Retry if another thread updated peak
            }
        }
        return ptr;
    }

    void AkhandaLogAllocator::Deallocate(void* ptr, size_t size) {
        if (ptr) {
            std::free(ptr);
            g_currentMemoryUsage.fetch_sub(size, std::memory_order_relaxed);
        }
    }

    size_t AkhandaLogAllocator::GetTotalAllocated() {
        return g_totalMemoryAllocated.load(std::memory_order_relaxed);
    }

    size_t AkhandaLogAllocator::GetCurrentUsage() {
        return g_currentMemoryUsage.load(std::memory_order_relaxed);
    }

    size_t AkhandaLogAllocator::GetPeakUsage() {
        return g_peakMemoryUsage.load(std::memory_order_relaxed);
    }

    // =============================================================================
    // Utility Functions Implementation
    // =============================================================================

    void ConfigureForDevelopment() {
        auto& setup = SpdlogSetup::Instance();

        PerformanceConfig perfConfig;
        perfConfig.asyncQueueSize = 4096;  // Smaller queue for development
        perfConfig.asyncThreadCount = 1;
        perfConfig.batchSize = 32;
        perfConfig.setHighPriority = false;
        setup.SetPerformanceConfig(perfConfig);

        PatternConfig patternConfig;
        // Use detailed debug pattern
        patternConfig.debugPattern = "[%Y-%m-%d %H:%M:%S.%f] [%n] [%l] [thread %t] [%s:%#] %v";
        setup.SetPatternConfig(patternConfig);

        MemoryIntegrationConfig memConfig;
        memConfig.enableMemoryTracking = true;
        memConfig.enableCallbackHooks = true;
        setup.SetMemoryIntegrationConfig(memConfig);
    }

    void ConfigureForProfiling() {
        auto& setup = SpdlogSetup::Instance();

        PerformanceConfig perfConfig;
        perfConfig.asyncQueueSize = 8192;  // Larger queue for performance
        perfConfig.asyncThreadCount = 1;
        perfConfig.batchSize = 64;
        perfConfig.setHighPriority = true;
        setup.SetPerformanceConfig(perfConfig);

        PatternConfig patternConfig;
        // Use balanced pattern for profiling
        patternConfig.profilePattern = "[%H:%M:%S.%f] [%n] [%l] [%t] %v";
        setup.SetPatternConfig(patternConfig);

        MemoryIntegrationConfig memConfig;
        memConfig.enableMemoryTracking = true;
        memConfig.enableCallbackHooks = true;
        setup.SetMemoryIntegrationConfig(memConfig);
    }

    void ConfigureForRelease() {
        auto& setup = SpdlogSetup::Instance();

        PerformanceConfig perfConfig;
        perfConfig.asyncQueueSize = 8192;
        perfConfig.asyncThreadCount = 1;
        perfConfig.batchSize = 64;
        perfConfig.setHighPriority = false;
        setup.SetPerformanceConfig(perfConfig);

        PatternConfig patternConfig;
        // Use minimal pattern for release
        patternConfig.releasePattern = "[%H:%M:%S.%f] [%n] [%l] %v";
        setup.SetPatternConfig(patternConfig);

        MemoryIntegrationConfig memConfig;
        memConfig.enableMemoryTracking = false;  // Disable in release
        memConfig.enableCallbackHooks = false;
        setup.SetMemoryIntegrationConfig(memConfig);
    }

    void ConfigureForShipping() {
        auto& setup = SpdlogSetup::Instance();

        PerformanceConfig perfConfig;
        perfConfig.asyncQueueSize = 2048;  // Minimal queue
        perfConfig.asyncThreadCount = 1;
        perfConfig.batchSize = 32;
        perfConfig.setHighPriority = false;
        setup.SetPerformanceConfig(perfConfig);

        PatternConfig patternConfig;
        // Minimal pattern for shipping
        patternConfig.releasePattern = "[%H:%M:%S] [%l] %v";
        setup.SetPatternConfig(patternConfig);

        MemoryIntegrationConfig memConfig;
        memConfig.enableMemoryTracking = false;
        memConfig.enableCallbackHooks = false;
        setup.SetMemoryIntegrationConfig(memConfig);

        // Set shipping log level
        spdlog::set_level(spdlog::level::err);  // Only errors and fatals
    }

    void SetupBasicLogging() {
        auto& setup = SpdlogSetup::Instance();

        // Create basic sinks
        std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks;
        sinks.push_back(setup.CreateConsoleSink());
        sinks.push_back(setup.CreateMSVCSink());

        // Create default logger
        auto logger = setup.CreateAsyncLogger("akhanda_console", sinks);
        spdlog::set_default_logger(logger);
    }

    void SetupFileLogging(const std::wstring& logDirectory) {
        auto& setup = SpdlogSetup::Instance();

        // Create log directory
        std::filesystem::create_directories(logDirectory);

        // Create timestamped log file
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);

        wchar_t filename[256];
        std::wcsftime(filename, sizeof(filename), L"akhanda_%Y%m%d_%H%M%S.log", &tm);

        std::wstring logFile = logDirectory + L"/" + filename;

        // Create file sink
        std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks;
        sinks.push_back(setup.CreateConsoleSink());
        sinks.push_back(setup.CreateMSVCSink());
        sinks.push_back(setup.CreateFileSink(logFile));

        // Create logger with file output
        auto logger = setup.CreateAsyncLogger("akhanda_file", sinks);
        spdlog::set_default_logger(logger);
    }

    void SetupAsyncLogging(size_t queueSize) {
        auto& setup = SpdlogSetup::Instance();

        // Configure async settings
        PerformanceConfig config = {};
        config.asyncQueueSize = queueSize;
        config.asyncThreadCount = 1;
        config.batchSize = 64;
        setup.SetPerformanceConfig(config);

        // Initialize async logging
        setup.SetupAsyncLogging();
    }

    void SetupEditorLogging() {
        // Editor logging setup is handled by LogManager's editor integration
        // This function exists for API completeness
        LogManager::Instance();

        // The editor sink is created when SetEditorCallback is called
        // No additional setup needed here
    }

    void EnablePerformanceMonitoring() {
        // Enable detailed performance tracking
        auto& setup = SpdlogSetup::Instance();

        MemoryIntegrationConfig config;
        config.enableMemoryTracking = true;
        config.enableCallbackHooks = true;
        setup.SetMemoryIntegrationConfig(config);

        setup.SetupMemoryTrackingCallbacks();
    }

    double GetAverageLoggingLatency() {
        auto& setup = SpdlogSetup::Instance();
        return setup.GetStatistics().averageProcessingTime.load(std::memory_order_relaxed);
    }

    size_t GetQueueDepth() {
        auto& setup = SpdlogSetup::Instance();
        return setup.GetStatistics().messagesInQueue.load(std::memory_order_relaxed);
    }

    void SetupMemoryTrackingCallbacks() {
        SpdlogSetup::Instance().SetupMemoryTrackingCallbacks();
    }

    void ClearMemoryTrackingCallbacks() {
        SpdlogSetup::Instance().ClearMemoryTrackingCallbacks();
    }

    void IntegrateLoggingWithMemorySystem(MemoryAllocationCallback allocCallback,
        MemoryDeallocationCallback deallocCallback) {
        auto& logManager = LogManager::Instance();
        logManager.SetMemoryTrackingCallbacks(std::move(allocCallback), std::move(deallocCallback));
    }
}