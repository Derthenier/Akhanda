// Engine/Core/Result/Core.Result.ixx
// Akhanda Game Engine - Universal Error Handling System
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include <string>
#include <variant>
#include <functional>
#include <type_traits>
#include <source_location>
#include <format>

export module Akhanda.Core.Result;

import Akhanda.Core.Logging;

export namespace Akhanda::Core {

    // ============================================================================
    // Error Categories for Different Engine Systems
    // ============================================================================

    enum class ErrorCategory : std::uint32_t {
        Success = 0,
        Memory,
        Renderer,
        Shader,
        Resource,
        Platform,
        Audio,
        Physics,
        AI,
        Network,
        Configuration,
        FileSystem,
        Threading,
        Plugin,
        Internal
    };

    inline std::string ErrorCategoryToString(ErrorCategory category) {
        switch (category) {
        case ErrorCategory::Success: return "Success";
        case ErrorCategory::Memory: return "Memory";
        case ErrorCategory::Renderer: return "Renderer";
        case ErrorCategory::Shader: return "Shader";
        case ErrorCategory::Resource: return "Resource";
        case ErrorCategory::Platform: return "Platform";
        case ErrorCategory::Audio: return "Audio";
        case ErrorCategory::Physics: return "Physics";
        case ErrorCategory::AI: return "AI";
        case ErrorCategory::Network: return "Network";
        case ErrorCategory::Configuration: return "Configuration";
        case ErrorCategory::FileSystem: return "FileSystem";
        case ErrorCategory::Threading: return "Threading";
        case ErrorCategory::Plugin: return "Plugin";
        case ErrorCategory::Internal: return "Internal";
        default: return "Unknown";
        }
    }

    // ============================================================================
    // Common Error Codes for Each System
    // ============================================================================

    enum class MemoryError : std::uint32_t {
        AllocationFailed = 1,
        DeallocationFailed,
        OutOfMemory,
        InvalidAlignment,
        NullPointer,
        BufferOverflow,
        InvalidSize,
        LeakDetected
    };

    enum class RendererError : std::uint32_t {
        DeviceCreationFailed = 1,
        InvalidDevice,
        CommandListFailed,
        ResourceCreationFailed,
        PipelineCreationFailed,
        InvalidFormat,
        UnsupportedFeature,
        SwapChainFailed
    };

    enum class ShaderError : std::uint32_t {
        CompilationFailed = 1,
        FileNotFound,
        SyntaxError,
        SemanticError,
        LinkError,
        UnsupportedShaderModel,
        InvalidEntryPoint,
        ReflectionFailed,
        CacheCorrupted,
        InvalidBytecode
    };

    enum class ResourceError : std::uint32_t {
        LoadFailed = 1,
        InvalidFormat,
        UnsupportedFormat,
        CorruptedData,
        MissingDependency,
        CircularDependency,
        CacheFailed,
        SerializationFailed,
        CompressionFailed
    };

    enum class PlatformError : std::uint32_t {
        WindowCreationFailed = 1,
        InputInitFailed,
        InvalidHandle,
        SystemCallFailed,
        PermissionDenied,
        ResourceBusy,
        TimeoutExpired,
        InterfaceNotSupported
    };

    enum class FileSystemError : std::uint32_t {
        FileNotFound = 1,
        AccessDenied,
        DiskFull,
        InvalidPath,
        ReadFailed,
        WriteFailed,
        DirectoryNotFound,
        FileAlreadyExists,
        CorruptedFile
    };

    enum class ThreadingError : std::uint32_t {
        TaskCreationFailed = 1,
        DeadlockDetected,
        ThreadStartFailed,
        SynchronizationFailed,
        QueueFull,
        InvalidThreadId,
        ResourceContention,
        TimeoutExpired
    };

    // ============================================================================
    // Error Information Structure
    // ============================================================================

    struct ErrorInfo {
        ErrorCategory category = ErrorCategory::Internal;
        std::uint32_t code = 0;
        std::string message;
        std::string context;
        std::source_location location;

        ErrorInfo() = default;

        ErrorInfo(ErrorCategory cat, std::uint32_t c, std::string msg,
            std::string ctx = "",
            std::source_location loc = std::source_location::current())
            : category(cat), code(c), message(std::move(msg)),
            context(std::move(ctx)), location(loc) {
        }

        // Convenience constructors for specific error types
        ErrorInfo(MemoryError error, std::string msg, std::string ctx = "",
            std::source_location loc = std::source_location::current())
            : category(ErrorCategory::Memory), code(static_cast<std::uint32_t>(error)),
            message(std::move(msg)), context(std::move(ctx)), location(loc) {
        }

        ErrorInfo(RendererError error, std::string msg, std::string ctx = "",
            std::source_location loc = std::source_location::current())
            : category(ErrorCategory::Renderer), code(static_cast<std::uint32_t>(error)),
            message(std::move(msg)), context(std::move(ctx)), location(loc) {
        }

        ErrorInfo(ShaderError error, std::string msg, std::string ctx = "",
            std::source_location loc = std::source_location::current())
            : category(ErrorCategory::Shader), code(static_cast<std::uint32_t>(error)),
            message(std::move(msg)), context(std::move(ctx)), location(loc) {
        }

        ErrorInfo(ResourceError error, std::string msg, std::string ctx = "",
            std::source_location loc = std::source_location::current())
            : category(ErrorCategory::Resource), code(static_cast<std::uint32_t>(error)),
            message(std::move(msg)), context(std::move(ctx)), location(loc) {
        }

        ErrorInfo(PlatformError error, std::string msg, std::string ctx = "",
            std::source_location loc = std::source_location::current())
            : category(ErrorCategory::Platform), code(static_cast<std::uint32_t>(error)),
            message(std::move(msg)), context(std::move(ctx)), location(loc) {
        }

        ErrorInfo(FileSystemError error, std::string msg, std::string ctx = "",
            std::source_location loc = std::source_location::current())
            : category(ErrorCategory::FileSystem), code(static_cast<std::uint32_t>(error)),
            message(std::move(msg)), context(std::move(ctx)), location(loc) {
        }

        ErrorInfo(ThreadingError error, std::string msg, std::string ctx = "",
            std::source_location loc = std::source_location::current())
            : category(ErrorCategory::Threading), code(static_cast<std::uint32_t>(error)),
            message(std::move(msg)), context(std::move(ctx)), location(loc) {
        }

        bool IsSuccess() const noexcept {
            return category == ErrorCategory::Success && code == 0;
        }

        std::string ToString() const {
            if (IsSuccess()) {
                return "Success";
            }

            std::string result = std::format("[{}:{}] {}",
                ErrorCategoryToString(category), code, message);

            if (!context.empty()) {
                result += std::format(" (Context: {})", context);
            }

            result += std::format(" [{}:{}:{}]",
                location.file_name(), location.line(), location.column());

            return result;
        }

        void LogError(const std::string& channelName = "Error") const {
            if (!IsSuccess()) {
                auto& channel = Logging::LogManager::Instance().GetChannel(channelName);
                channel.Log(Logging::LogLevel::Error, ToString());
            }
        }

        void LogWarning(const std::string& channelName = "Warning") const {
            if (!IsSuccess()) {
                auto& channel = Logging::LogManager::Instance().GetChannel(channelName);
                channel.Log(Logging::LogLevel::Warning, ToString());
            }
        }
    };

    // ============================================================================
    // Success Helper
    // ============================================================================

    inline ErrorInfo Success() {
        return ErrorInfo{ ErrorCategory::Success, 0, "Success" };
    }

    // ============================================================================
    // Result<T, E> Template Class
    // ============================================================================

    template<typename T, typename E = ErrorInfo>
    class Result {
    private:
        std::variant<T, E> storage_;

    public:
        using ValueType = T;
        using ErrorType = E;

        // Constructors
        Result() = delete; // Force explicit construction

        // Success constructor
        Result(T&& value) : storage_(std::forward<T>(value)) {}
        Result(const T& value) : storage_(value) {}

        // Error constructor
        Result(E&& error) : storage_(std::forward<E>(error)) {}
        Result(const E& error) : storage_(error) {}

        // Copy and move constructors
        Result(const Result&) = default;
        Result(Result&&) = default;
        Result& operator=(const Result&) = default;
        Result& operator=(Result&&) = default;

        // State queries
        bool IsSuccess() const noexcept {
            return std::holds_alternative<T>(storage_);
        }

        bool IsError() const noexcept {
            return std::holds_alternative<E>(storage_);
        }

        explicit operator bool() const noexcept {
            return IsSuccess();
        }

        // Value access (throws std::bad_variant_access if error)
        const T& Value() const& {
            return std::get<T>(storage_);
        }

        T& Value()& {
            return std::get<T>(storage_);
        }

        T&& Value()&& {
            return std::get<T>(std::move(storage_));
        }

        // Error access (throws std::bad_variant_access if success)
        const E& Error() const& {
            return std::get<E>(storage_);
        }

        E& Error()& {
            return std::get<E>(storage_);
        }

        E&& Error()&& {
            return std::get<E>(std::move(storage_));
        }

        // Safe value access with default
        T ValueOr(const T& defaultValue) const& {
            return IsSuccess() ? Value() : defaultValue;
        }

        T ValueOr(T&& defaultValue)&& {
            return IsSuccess() ? std::move(*this).Value() : std::move(defaultValue);
        }

        // Safe value access with factory function
        template<typename F>
        T ValueOrElse(F&& factory) const& {
            return IsSuccess() ? Value() : factory(Error());
        }

        template<typename F>
        T ValueOrElse(F&& factory)&& {
            return IsSuccess() ? std::move(*this).Value() : factory(std::move(*this).Error());
        }

        // Monadic operations
        template<typename F>
        auto Map(F&& func) const& -> Result<std::invoke_result_t<F, const T&>, E> {
            if (IsSuccess()) {
                return Result<std::invoke_result_t<F, const T&>, E>(func(Value()));
            }
            else {
                return Result<std::invoke_result_t<F, const T&>, E>(Error());
            }
        }

        template<typename F>
        auto Map(F&& func) && -> Result<std::invoke_result_t<F, T&&>, E> {
            if (IsSuccess()) {
                return Result<std::invoke_result_t<F, T&&>, E>(func(std::move(*this).Value()));
            }
            else {
                return Result<std::invoke_result_t<F, T&&>, E>(std::move(*this).Error());
            }
        }

        template<typename F>
        auto MapError(F&& func) const& -> Result<T, std::invoke_result_t<F, const E&>> {
            if (IsError()) {
                return Result<T, std::invoke_result_t<F, const E&>>(func(Error()));
            }
            else {
                return Result<T, std::invoke_result_t<F, const E&>>(Value());
            }
        }

        template<typename F>
        auto MapError(F&& func) && -> Result<T, std::invoke_result_t<F, E&&>> {
            if (IsError()) {
                return Result<T, std::invoke_result_t<F, E&&>>(func(std::move(*this).Error()));
            }
            else {
                return Result<T, std::invoke_result_t<F, E&&>>(std::move(*this).Value());
            }
        }

        template<typename F>
        auto AndThen(F&& func) const& -> std::invoke_result_t<F, const T&> {
            if (IsSuccess()) {
                return func(Value());
            }
            else {
                return std::invoke_result_t<F, const T&>(Error());
            }
        }

        template<typename F>
        auto AndThen(F&& func) && -> std::invoke_result_t<F, T&&> {
            if (IsSuccess()) {
                return func(std::move(*this).Value());
            }
            else {
                return std::invoke_result_t<F, T&&>(std::move(*this).Error());
            }
        }

        template<typename F>
        auto OrElse(F&& func) const& -> Result<T, std::invoke_result_t<F, const E&>> {
            if (IsError()) {
                return func(Error());
            }
            else {
                return Result<T, std::invoke_result_t<F, const E&>>(Value());
            }
        }

        template<typename F>
        auto OrElse(F&& func) && -> Result<T, std::invoke_result_t<F, E&&>> {
            if (IsError()) {
                return func(std::move(*this).Error());
            }
            else {
                return Result<T, std::invoke_result_t<F, E&&>>(std::move(*this).Value());
            }
        }

        // Utility methods
        void LogOnError(const std::string& channelName = "Error") const {
            if constexpr (std::is_same_v<E, ErrorInfo>) {
                if (IsError()) {
                    Error().LogError(channelName);
                }
            }
        }

        void LogOnErrorAsWarning(const std::string& channelName = "Warning") const {
            if constexpr (std::is_same_v<E, ErrorInfo>) {
                if (IsError()) {
                    Error().LogWarning(channelName);
                }
            }
        }
    };

    // ============================================================================
    // Type Aliases for Common Result Types
    // ============================================================================

    template<typename T>
    using MemoryResult = Result<T, ErrorInfo>;

    template<typename T>
    using RendererResult = Result<T, ErrorInfo>;

    template<typename T>
    using ShaderResult = Result<T, ErrorInfo>;

    template<typename T>
    using ResourceResult = Result<T, ErrorInfo>;

    template<typename T>
    using PlatformResult = Result<T, ErrorInfo>;

    template<typename T>
    using FileSystemResult = Result<T, ErrorInfo>;

    template<typename T>
    using ThreadingResult = Result<T, ErrorInfo>;

    // Void result types
    using VoidResult = Result<void*, ErrorInfo>;

    // ============================================================================
    // Helper Functions and Macros
    // ============================================================================

    // Helper to create success results
    template<typename T>
    Result<T, ErrorInfo> Ok(T&& value) {
        return Result<T, ErrorInfo>(std::forward<T>(value));
    }

    // Helper to create error results
    template<typename T>
    Result<T, ErrorInfo> Err(ErrorInfo error) {
        return Result<T, ErrorInfo>(std::move(error));
    }

    // Helper for void success
    inline VoidResult OkVoid() {
        return VoidResult(static_cast<void*>(nullptr));
    }

    // Helper to create errors with automatic location
    template<typename ErrorType>
    ErrorInfo MakeError(ErrorType error, const std::string& message,
        const std::string& context = "",
        std::source_location loc = std::source_location::current()) {
        return ErrorInfo(error, message, context, loc);
    }

    // ============================================================================
    // Error Propagation Macros
    // ============================================================================

    // These macros help with error propagation similar to Rust's ? operator




    // ============================================================================
    // Template-Based Error Handling Utilities (Module-Safe)
    // ============================================================================

    // Template function equivalent to TRY macro behavior
    template<typename ResultType>
    constexpr bool ShouldPropagate(const ResultType& result) {
        return !result.IsSuccess();
    }

    // Template function for safe value extraction
    template<typename ResultType>
    constexpr auto ExtractValue(ResultType&& result) -> decltype(std::forward<ResultType>(result).Value()) {
        if (!result.IsSuccess()) {
            throw std::runtime_error("Attempted to extract value from error result");
        }
        return std::forward<ResultType>(result).Value();
    }

    // Template function for conditional error creation
    template<typename ErrorType>
    constexpr VoidResult EnsureCondition(bool condition, ErrorType error, const std::string& message,
        std::source_location loc = std::source_location::current()) {
        if (condition) {
            return OkVoid();
        }
        else {
            return Err<void*>(MakeError(error, message, "", loc));
        }
    }

    // Template function for HRESULT checking (Windows-specific)
#ifdef _WIN32
    template<typename T = void*>
    Result<T, ErrorInfo> CheckHResult(long hr, const std::string& message,
        std::source_location loc = std::source_location::current()) {
        if (hr < 0) {
            return Err<T>(MakeError(RendererError::DeviceCreationFailed,
                std::format("{}: HRESULT 0x{:08X}", message, static_cast<std::uint32_t>(hr)), "", loc));
        }
        if constexpr (std::is_same_v<T, void*>) {
            return Ok(static_cast<void*>(nullptr));
        }
        else {
            // For non-void types, caller must provide the value separately
            static_assert(std::is_same_v<T, void*>, "CheckHResult for non-void types requires separate value construction");
        }
    }
#endif

} // namespace Akhanda::Core
