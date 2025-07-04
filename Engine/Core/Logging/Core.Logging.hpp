// Core.Logging.hpp - Logging Macros Header
#pragma once

// Import the logging module
import Akhanda.Core.Logging;

// =============================================================================
// Convenience Macros
// =============================================================================

// Channel-based logging macros with compile-time filtering
#define LOG_DEBUG(channel, message) \
    do { if constexpr (::Logging::ShouldCompileLog(::Logging::LogLevel::Debug)) { \
        if ((channel).ShouldLog(::Logging::LogLevel::Debug)) { \
            (channel).Debug(message); \
        } \
    } } while(0)

#define LOG_INFO(channel, message) \
    do { if constexpr (::Logging::ShouldCompileLog(::Logging::LogLevel::Info)) { \
        if ((channel).ShouldLog(::Logging::LogLevel::Info)) { \
            (channel).Info(message); \
        } \
    } } while(0)

#define LOG_WARNING(channel, message) \
    do { if constexpr (::Logging::ShouldCompileLog(::Logging::LogLevel::Warning)) { \
        if ((channel).ShouldLog(::Logging::LogLevel::Warning)) { \
            (channel).Warning(message); \
        } \
    } } while(0)

#define LOG_ERROR(channel, message) \
    do { if constexpr (::Logging::ShouldCompileLog(::Logging::LogLevel::Error)) { \
        if ((channel).ShouldLog(::Logging::LogLevel::Error)) { \
            (channel).Error(message); \
        } \
    } } while(0)

#define LOG_FATAL(channel, message) \
    do { if constexpr (::Logging::ShouldCompileLog(::Logging::LogLevel::Fatal)) { \
        if ((channel).ShouldLog(::Logging::LogLevel::Fatal)) { \
            (channel).Fatal(message); \
        } \
    } } while(0)

// Formatted logging macros
#define LOG_DEBUG_FORMAT(channel, fmt, ...) \
    do { if constexpr (::Logging::ShouldCompileLog(::Logging::LogLevel::Debug)) { \
        if ((channel).ShouldLog(::Logging::LogLevel::Debug)) { \
            (channel).DebugFormat(fmt, __VA_ARGS__); \
        } \
    } } while(0)

#define LOG_INFO_FORMAT(channel, fmt, ...) \
    do { if constexpr (::Logging::ShouldCompileLog(::Logging::LogLevel::Info)) { \
        if ((channel).ShouldLog(::Logging::LogLevel::Info)) { \
            (channel).InfoFormat(fmt, __VA_ARGS__); \
        } \
    } } while(0)

#define LOG_WARNING_FORMAT(channel, fmt, ...) \
    do { if constexpr (::Logging::ShouldCompileLog(::Logging::LogLevel::Warning)) { \
        if ((channel).ShouldLog(::Logging::LogLevel::Warning)) { \
            (channel).WarningFormat(fmt, __VA_ARGS__); \
        } \
    } } while(0)

#define LOG_ERROR_FORMAT(channel, fmt, ...) \
    do { if constexpr (::Logging::ShouldCompileLog(::Logging::LogLevel::Error)) { \
        if ((channel).ShouldLog(::Logging::LogLevel::Error)) { \
            (channel).ErrorFormat(fmt, __VA_ARGS__); \
        } \
    } } while(0)

#define LOG_FATAL_FORMAT(channel, fmt, ...) \
    do { if constexpr (::Logging::ShouldCompileLog(::Logging::LogLevel::Fatal)) { \
        if ((channel).ShouldLog(::Logging::LogLevel::Fatal)) { \
            (channel).FatalFormat(fmt, __VA_ARGS__); \
        } \
    } } while(0)

// Scope logging macro
#define LOG_SCOPE(channel, name) \
    ::Logging::ScopeLogger ANONYMOUS_VARIABLE(scopeLogger_)((channel), (name))

// Helper macros for unique variable names
#define ANONYMOUS_VARIABLE(str) CONCATENATE(str, __LINE__)
#define CONCATENATE(x, y) CONCATENATE_IMPL(x, y)
#define CONCATENATE_IMPL(x, y) x##y

// Engine-specific convenience macros
#define ENGINE_LOG_DEBUG(message) LOG_DEBUG(::Logging::Channels::Engine(), message)
#define ENGINE_LOG_INFO(message) LOG_INFO(::Logging::Channels::Engine(), message)
#define ENGINE_LOG_WARNING(message) LOG_WARNING(::Logging::Channels::Engine(), message)
#define ENGINE_LOG_ERROR(message) LOG_ERROR(::Logging::Channels::Engine(), message)
#define ENGINE_LOG_FATAL(message) LOG_FATAL(::Logging::Channels::Engine(), message)

#define ENGINE_LOG_DEBUG_FORMAT(fmt, ...) LOG_DEBUG_FORMAT(::Logging::Channels::Engine(), fmt, __VA_ARGS__)
#define ENGINE_LOG_INFO_FORMAT(fmt, ...) LOG_INFO_FORMAT(::Logging::Channels::Engine(), fmt, __VA_ARGS__)
#define ENGINE_LOG_WARNING_FORMAT(fmt, ...) LOG_WARNING_FORMAT(::Logging::Channels::Engine(), fmt, __VA_ARGS__)
#define ENGINE_LOG_ERROR_FORMAT(fmt, ...) LOG_ERROR_FORMAT(::Logging::Channels::Engine(), fmt, __VA_ARGS__)
#define ENGINE_LOG_FATAL_FORMAT(fmt, ...) LOG_FATAL_FORMAT(::Logging::Channels::Engine(), fmt, __VA_ARGS__)

// Renderer logging macros
#define RENDERER_LOG_DEBUG(message) LOG_DEBUG(::Logging::Channels::Renderer(), message)
#define RENDERER_LOG_INFO(message) LOG_INFO(::Logging::Channels::Renderer(), message)
#define RENDERER_LOG_WARNING(message) LOG_WARNING(::Logging::Channels::Renderer(), message)
#define RENDERER_LOG_ERROR(message) LOG_ERROR(::Logging::Channels::Renderer(), message)

#define RENDERER_LOG_DEBUG_FORMAT(fmt, ...) LOG_DEBUG_FORMAT(::Logging::Channels::Renderer(), fmt, __VA_ARGS__)
#define RENDERER_LOG_INFO_FORMAT(fmt, ...) LOG_INFO_FORMAT(::Logging::Channels::Renderer(), fmt, __VA_ARGS__)

// Editor logging macros
#define EDITOR_LOG_DEBUG(message) LOG_DEBUG(::Logging::Channels::Editor(), message)
#define EDITOR_LOG_INFO(message) LOG_INFO(::Logging::Channels::Editor(), message)
#define EDITOR_LOG_WARNING(message) LOG_WARNING(::Logging::Channels::Editor(), message)
#define EDITOR_LOG_ERROR(message) LOG_ERROR(::Logging::Channels::Editor(), message)

// AI logging macros
#define AI_LOG_DEBUG(message) LOG_DEBUG(::Logging::Channels::AI(), message)
#define AI_LOG_INFO(message) LOG_INFO(::Logging::Channels::AI(), message)
#define AI_LOG_WARNING(message) LOG_WARNING(::Logging::Channels::AI(), message)
#define AI_LOG_ERROR(message) LOG_ERROR(::Logging::Channels::AI(), message)

// Physics logging macros
#define PHYSICS_LOG_DEBUG(message) LOG_DEBUG(::Logging::Channels::Physics(), message)
#define PHYSICS_LOG_INFO(message) LOG_INFO(::Logging::Channels::Physics(), message)

// Game logging macros
#define GAME_LOG_DEBUG(message) LOG_DEBUG(::Logging::Channels::Game(), message)
#define GAME_LOG_INFO(message) LOG_INFO(::Logging::Channels::Game(), message)
#define GAME_LOG_WARNING(message) LOG_WARNING(::Logging::Channels::Game(), message)
#define GAME_LOG_ERROR(message) LOG_ERROR(::Logging::Channels::Game(), message)

// Plugin logging macros
#define PLUGIN_LOG_DEBUG(message) LOG_DEBUG(::Logging::Channels::Plugin(), message)
#define PLUGIN_LOG_INFO(message) LOG_INFO(::Logging::Channels::Plugin(), message)
#define PLUGIN_LOG_WARNING(message) LOG_WARNING(::Logging::Channels::Plugin(), message)
#define PLUGIN_LOG_ERROR(message) LOG_ERROR(::Logging::Channels::Plugin(), message)