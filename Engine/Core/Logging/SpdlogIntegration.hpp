// SpdlogIntegration.hpp - spdlog Configuration and Setup
#pragma once

// =============================================================================
// spdlog Configuration - Define before including spdlog headers
// =============================================================================

// Performance optimizations
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG  // Set minimum compile-time level
#define SPDLOG_NO_EXCEPTIONS                     // Disable exceptions for performance
#define SPDLOG_PREVENT_CHILD_FD                  // Prevent file descriptor inheritance

// Thread safety configuration
#define SPDLOG_ENABLE_PATTERN_PADDING            // Enable pattern padding for alignment

// Memory optimizations for game engines
#define SPDLOG_FINAL final                       // Allow compiler optimizations
#define SPDLOG_HEADER_ONLY                       // Header-only mode for easier integration

// Use std::format instead of fmt for C++23
#if __cplusplus >= 202302L
#define SPDLOG_USE_STD_FORMAT
#endif

// Windows-specific optimizations
#ifdef _WIN32
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT         // Support wide char conversion
#define SPDLOG_WCHAR_FILENAMES               // Support wide char filenames
#endif

// =============================================================================
// Include spdlog headers with configuration
// =============================================================================

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/details/thread_pool.h>

#include <memory>
#include <string>
#include <vector>
#include <functional>

import Akhanda.Core.Memory;

// =============================================================================
// Akhanda spdlog Integration Configuration
// =============================================================================

namespace Akhanda::Logging::Integration {

    // Performance configuration for game engines
    struct PerformanceConfig {
        // Async configuration
        size_t asyncQueueSize = 8192;           // Messages in async queue
        size_t asyncThreadCount = 1;            // Number of async worker threads

        // Memory management
        bool useMemoryPools = true;             // Use pre-allocated message pools
        size_t messagePoolSize = 1000;          // Pre-allocated message pool size

        // Batching for efficiency
        size_t batchSize = 64;                  // Messages processed per batch
        std::chrono::milliseconds flushInterval{ 100 }; // Auto-flush interval

        // Thread affinity (Windows only)
        bool setThreadAffinity = false;         // Pin async thread to specific CPU
        uint32_t threadAffinityMask = 0x1;     // CPU affinity mask

        // Priority settings
        bool setHighPriority = false;           // Use high thread priority for logging
    };

    // Pattern configuration for different build types
    struct PatternConfig {
        std::string debugPattern = "[%Y-%m-%d %H:%M:%S.%f] [%n] [%l] [thread %t] [%s:%#] %v";
        std::string releasePattern = "[%H:%M:%S.%f] [%n] [%l] %v";
        std::string profilePattern = "[%H:%M:%S.%f] [%n] [%l] [%t] %v";

        // Custom pattern for different sinks
        std::string consolePattern = "%^[%H:%M:%S] [%n] [%l]%$ %v";
        std::string filePattern = "[%Y-%m-%d %H:%M:%S.%f] [%n] [%l] [thread %t] %v";
        std::string editorPattern = "[%H:%M:%S.%f] [%l] %v";
    };

    // Memory integration configuration
    struct MemoryIntegrationConfig {
        bool enableMemoryTracking = true;       // Track memory usage of logging
        bool enableAllocationHooks = true;      // Hook into allocator for detailed tracking
        size_t memoryBudget = 50 * 1024 * 1024; // 50MB memory budget for logging

        // Custom allocator integration
        bool useCustomAllocator = true;         // Use Akhanda's memory system
        MemoryTag loggerMemoryTag = MemoryTag::Logging;
        MemoryTag messageMemoryTag = MemoryTag::Logging;
    };

    // =============================================================================
    // spdlog Setup and Configuration Class
    // =============================================================================

    class SpdlogSetup {
    public:
        static SpdlogSetup& Instance();

        // Configuration
        void SetPerformanceConfig(const PerformanceConfig& config);
        void SetPatternConfig(const PatternConfig& config);
        void SetMemoryIntegrationConfig(const MemoryIntegrationConfig& config);

        // Initialization
        void Initialize();
        void Shutdown();

        // Get configured patterns for current build
        std::string GetCurrentPattern() const;
        std::string GetConsolePattern() const;
        std::string GetFilePattern() const;
        std::string GetEditorPattern() const;

        // Memory integration
        void IntegrateWithMemorySystem();
        void SetupCustomAllocator();

        // Performance tuning
        void OptimizeForGameEngine();
        void SetupAsyncLogging();
        void ConfigureThreadAffinity();

        // Sink factory methods
        std::shared_ptr<spdlog::sinks::sink> CreateConsoleSink();
        std::shared_ptr<spdlog::sinks::sink> CreateFileSink(const std::string& filename);
        std::shared_ptr<spdlog::sinks::sink> CreateRotatingFileSink(
            const std::string& filename, size_t maxSize, size_t maxFiles);
        std::shared_ptr<spdlog::sinks::sink> CreateMSVCSink();

        // Logger factory methods
        std::shared_ptr<spdlog::logger> CreateAsyncLogger(
            const std::string& name,
            const std::vector<std::shared_ptr<spdlog::sinks::sink>>& sinks);
        std::shared_ptr<spdlog::logger> CreateSyncLogger(
            const std::string& name,
            const std::vector<std::shared_ptr<spdlog::sinks::sink>>& sinks);

        // Statistics and monitoring
        struct Statistics {
            std::atomic<size_t> totalLoggers{ 0 };
            std::atomic<size_t> activeAsyncThreads{ 0 };
            std::atomic<size_t> memoryUsage{ 0 };
            std::atomic<size_t> messagesInQueue{ 0 };
            std::atomic<double> averageProcessingTime{ 0.0 };
        };

        const Statistics& GetStatistics() const { return stats_; }

    private:
        SpdlogSetup() = default;
        ~SpdlogSetup();

        PerformanceConfig perfConfig_;
        PatternConfig patternConfig_;
        MemoryIntegrationConfig memoryConfig_;

        std::shared_ptr<spdlog::details::thread_pool> threadPool_;
        bool initialized_ = false;

        mutable Statistics stats_;
    };

    // =============================================================================
    // Custom Memory Allocator for spdlog Integration
    // =============================================================================

    class AkhandaLogAllocator {
    public:
        static void* Allocate(size_t size, MemoryTag tag = MemoryTag::Logging);
        static void Deallocate(void* ptr, size_t size, MemoryTag tag = MemoryTag::Logging);

        // spdlog allocator interface
        template<typename T>
        class allocator {
        public:
            using value_type = T;
            using pointer = T*;
            using const_pointer = const T*;
            using reference = T&;
            using const_reference = const T&;
            using size_type = std::size_t;
            using difference_type = std::ptrdiff_t;

            template<typename U>
            struct rebind {
                using other = allocator<U>;
            };

            allocator() noexcept = default;
            template<typename U>
            allocator(const allocator<U>&) noexcept {}

            pointer allocate(size_type n) {
                return static_cast<pointer>(
                    AkhandaLogAllocator::Allocate(n * sizeof(T))
                    );
            }

            void deallocate(pointer p, size_type n) noexcept {
                AkhandaLogAllocator::Deallocate(p, n * sizeof(T));
            }

            template<typename U>
            bool operator==(const allocator<U>&) const noexcept { return true; }

            template<typename U>
            bool operator!=(const allocator<U>&) const noexcept { return false; }
        };

        // Statistics
        static size_t GetTotalAllocated();
        static size_t GetCurrentUsage();
        static size_t GetPeakUsage();
    };

    // =============================================================================
    // Utility Functions for spdlog Integration
    // =============================================================================

    // Convert Akhanda LogLevel to spdlog level
    constexpr spdlog::level::level_enum ToSpdlogLevel(LogLevel level) noexcept {
        switch (level) {
        case LogLevel::Debug:   return spdlog::level::debug;
        case LogLevel::Info:    return spdlog::level::info;
        case LogLevel::Warning: return spdlog::level::warn;
        case LogLevel::Error:   return spdlog::level::err;
        case LogLevel::Fatal:   return spdlog::level::critical;
        default:                return spdlog::level::info;
        }
    }

    // Convert spdlog level to Akhanda LogLevel
    constexpr LogLevel FromSpdlogLevel(spdlog::level::level_enum level) noexcept {
        switch (level) {
        case spdlog::level::debug:    return LogLevel::Debug;
        case spdlog::level::info:     return LogLevel::Info;
        case spdlog::level::warn:     return LogLevel::Warning;
        case spdlog::level::err:      return LogLevel::Error;
        case spdlog::level::critical: return LogLevel::Fatal;
        default:                      return LogLevel::Info;
        }
    }

    // Optimal spdlog configuration for different scenarios
    void ConfigureForDevelopment();   // Maximum debugging info
    void ConfigureForProfiling();     // Balanced performance/info
    void ConfigureForRelease();       // Minimal overhead
    void ConfigureForShipping();      // Errors and fatals only

    // Quick setup functions
    void SetupBasicLogging();         // Console + MSVC output
    void SetupFileLogging(const std::string& logDirectory);
    void SetupAsyncLogging(size_t queueSize = 8192);
    void SetupEditorLogging();        // Enable editor integration

    // Performance monitoring
    void EnablePerformanceMonitoring();
    double GetAverageLoggingLatency();
    size_t GetQueueDepth();

    // =============================================================================
    // Build System Integration Instructions
    // =============================================================================

    /*
    Add to your vcxproj PropertySheets/Core.props:

    <PropertyGroup>
        <!-- spdlog configuration -->
        <SpdlogDir>$(SolutionDir)ThirdParty\spdlog</SpdlogDir>
    </PropertyGroup>

    <ItemDefinitionGroup>
        <ClCompile>
            <AdditionalIncludeDirectories>$(SpdlogDir)\include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
            <PreprocessorDefinitions>
                SPDLOG_HEADER_ONLY;
                SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG;
                SPDLOG_NO_EXCEPTIONS;
                %(PreprocessorDefinitions)
            </PreprocessorDefinitions>
        </ClCompile>
    </ItemDefinitionGroup>

    For Release builds, change SPDLOG_LEVEL_DEBUG to SPDLOG_LEVEL_WARN in Release.props
    */

} // namespace Akhanda::Logging::Integration

// =============================================================================
// Quick Setup Macros for Different Configurations
// =============================================================================

// Initialize logging for different build configurations
#ifdef AKH_DEBUG
#define AKHANDA_SETUP_LOGGING() \
        Akhanda::Logging::Integration::ConfigureForDevelopment(); \
        Akhanda::Logging::Integration::SetupBasicLogging(); \
        Akhanda::Logging::Integration::SetupEditorLogging()
#elif defined(AKH_PROFILE)
#define AKHANDA_SETUP_LOGGING() \
        Akhanda::Logging::Integration::ConfigureForProfiling(); \
        Akhanda::Logging::Integration::SetupAsyncLogging(); \
        Akhanda::Logging::Integration::SetupFileLogging("Logs")
#else
#define AKHANDA_SETUP_LOGGING() \
        Akhanda::Logging::Integration::ConfigureForRelease(); \
        Akhanda::Logging::Integration::SetupAsyncLogging(4096)
#endif

// Shutdown logging
#define AKHANDA_SHUTDOWN_LOGGING() \
    Akhanda::Logging::Integration::SpdlogSetup::Instance().Shutdown()

// Quick memory integration
#define AKHANDA_INTEGRATE_LOGGING_MEMORY() \
    Akhanda::Logging::Integration::SpdlogSetup::Instance().IntegrateWithMemorySystem()