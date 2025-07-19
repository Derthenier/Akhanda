// Core.Configuration.ixx (Fixed with Traditional Header)
// Akhanda Game Engine - Configuration Library Module Interface
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include "JsonWrapper.hpp"
#include <functional>

export module Akhanda.Core.Configuration;

export namespace Akhanda::Configuration {

    // Use JsonValue instead of raw nlohmann::json
    using json = JsonValue;

    // ============================================================================
    // Configuration Error Handling
    // ============================================================================

    enum class ConfigResult : std::uint32_t {
        Success = 0,
        FileNotFound,
        ParseError,
        ValidationError,
        TypeMismatch,
        KeyNotFound,
        ReadOnlyError,
        MemoryError
    };

    struct ConfigError {
        ConfigResult result;
        std::string message;
        std::string context;
        std::uint32_t line;
        std::uint32_t column;

        ConfigError(ConfigResult r, std::string msg, std::string ctx = "")
            : result(r), message(std::move(msg)), context(std::move(ctx)), line(0), column(0) {
        }

        bool IsSuccess() const noexcept { return result == ConfigResult::Success; }
        explicit operator bool() const noexcept { return IsSuccess(); }
    };

    template<typename T>
    class ConfigResult_t {
    public:
        ConfigResult_t(T&& value) : value_(std::forward<T>(value)), hasValue_(true), error_(ConfigResult::Success, "") {}
        ConfigResult_t(const T& value) : value_(value), hasValue_(true), error_(ConfigResult::Success, "") {}
        ConfigResult_t(ConfigError error) : hasValue_(false), error_(std::move(error)) {}

        bool HasValue() const noexcept { return hasValue_ && error_.IsSuccess(); }
        bool HasError() const noexcept { return !error_.IsSuccess(); }

        const T& Value() const& {
            if (HasError()) throw std::runtime_error(error_.message);
            return value_;
        }

        T&& Value()&& {
            if (HasError()) throw std::runtime_error(error_.message);
            return std::move(value_);
        }

        const ConfigError& Error() const noexcept { return error_; }

        explicit operator bool() const noexcept { return HasValue(); }

    private:
        T value_{};
        bool hasValue_ = false;
        ConfigError error_;
    };

    // ============================================================================
    // Configuration Value Template
    // ============================================================================

    template<typename T>
    class ConfigValue {
    public:
        ConfigValue() = default;

        explicit ConfigValue(T defaultValue)
            : value_(defaultValue), defaultValue_(defaultValue), hasValue_(true) {
        }

        // Constructor with range validation - only for comparable types
        template<typename U = T>
        ConfigValue(T defaultValue, T minValue, T maxValue,
            typename std::enable_if_t<std::is_arithmetic_v<U>>* = nullptr)
            : value_(defaultValue), defaultValue_(defaultValue)
            , minValue_(minValue), maxValue_(maxValue)
            , hasValidation_(true), hasValue_(true) {
        }

        // Value access
        const T& Get() const noexcept { return hasValue_ ? value_ : defaultValue_; }

        ConfigResult_t<bool> Set(T newValue) {
            // Only do range validation for arithmetic types
            if constexpr (std::is_arithmetic_v<T>) {
                if (hasValidation_) {
                    if (newValue < minValue_ || newValue > maxValue_) {
                        return ConfigError(ConfigResult::ValidationError,
                            std::format("Value out of range [{}, {}]", minValue_, maxValue_));
                    }
                }
            }

            if (customValidator_ && !customValidator_(newValue)) {
                return ConfigError(ConfigResult::ValidationError, "Custom validation failed");
            }

            T oldValue = value_;
            value_ = newValue;
            hasValue_ = true;

            // Notify change callbacks
            for (const auto& callback : changeCallbacks_) {
                try {
                    callback(oldValue, newValue);
                }
                catch (...) {
                    // Log error but don't fail the operation
                }
            }

            return true;
        }

        // Value properties
        bool HasValue() const noexcept { return hasValue_; }
        void Reset() noexcept { value_ = defaultValue_; hasValue_ = true; }
        bool HasValidation() const noexcept { return hasValidation_; }

        // Only provide min/max accessors for arithmetic types
        template<typename U = T>
        typename std::enable_if_t<std::is_arithmetic_v<U>, const T&> GetMin() const noexcept {
            return minValue_;
        }

        template<typename U = T>
        typename std::enable_if_t<std::is_arithmetic_v<U>, const T&> GetMax() const noexcept {
            return maxValue_;
        }

        const T& GetDefault() const noexcept { return defaultValue_; }

        // Change notification
        using ChangeCallback = std::function<void(const T& oldValue, const T& newValue)>;
        void OnChange(ChangeCallback callback) { changeCallbacks_.emplace_back(std::move(callback)); }

        // Custom validation
        using Validator = std::function<bool(const T&)>;
        void SetValidator(Validator validator) { customValidator_ = std::move(validator); }

        // JSON serialization
        JsonValue ToJson() const {
            auto j = ConfigJson::CreateObject();
            SetConfigValue(j, "value", value_);
            SetConfigValue(j, "default", defaultValue_);
            if constexpr (std::is_arithmetic_v<T>) {
                if (hasValidation_) {
                    SetConfigValue(j, "min", minValue_);
                    SetConfigValue(j, "max", maxValue_);
                }
            }
            return j;
        }

        ConfigResult_t<bool> FromJson(const JsonValue& j) {
            try {
                if (!j.Contains("value")) {
                    return ConfigError(ConfigResult::KeyNotFound, "Missing 'value' field");
                }

                T newValue = GetConfigValue<T>(j, "value");
                return Set(std::move(newValue));
            }
            catch (const std::exception& e) {
                return ConfigError(ConfigResult::ParseError,
                    std::format("JSON parse error: {}", e.what()));
            }
        }

    private:
        T value_{};
        T defaultValue_{};
        T minValue_{};  // Only used for arithmetic types
        T maxValue_{};  // Only used for arithmetic types
        bool hasValidation_ = false;
        bool hasValue_ = false;
        std::vector<ChangeCallback> changeCallbacks_;
        Validator customValidator_;
    };

    // ============================================================================
    // Resolution Type (Common configuration type)
    // ============================================================================

    struct Resolution {
        std::uint32_t width = 1920;
        std::uint32_t height = 1080;

        constexpr Resolution() = default;
        constexpr Resolution(std::uint32_t w, std::uint32_t h) : width(w), height(h) {}

        constexpr bool operator==(const Resolution& other) const noexcept {
            return width == other.width && height == other.height;
        }

        constexpr bool operator!=(const Resolution& other) const noexcept {
            return !(*this == other);
        }

        constexpr float AspectRatio() const noexcept {
            return height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 0.0f;
        }

        JsonValue ToJson() const {
            auto j = ConfigJson::CreateObject();
            SetConfigValue(j, "width", width);
            SetConfigValue(j, "height", height);
            return j;
        }

        static ConfigResult_t<Resolution> FromJson(const JsonValue& j) {
            try {
                Resolution res;
                res.width = GetConfigValue<std::uint32_t>(j, "width", 1920);
                res.height = GetConfigValue<std::uint32_t>(j, "height", 1080);

                if (res.width == 0 || res.height == 0) {
                    return ConfigError(ConfigResult::ValidationError, "Resolution cannot be zero");
                }

                return res;
            }
            catch (const std::exception& e) {
                return ConfigError(ConfigResult::ParseError,
                    std::format("Failed to parse Resolution: {}", e.what()));
            }
        }
    };

    // ============================================================================
    // Configuration Section Interface
    // ============================================================================

    class IConfigSection {
    public:
        virtual ~IConfigSection() = default;

        // Core interface
        virtual const char* GetSectionName() const noexcept = 0;
        virtual ConfigResult_t<bool> LoadFromJson(const JsonValue& sectionJson) = 0;
        virtual JsonValue SaveToJson() const = 0;
        virtual ConfigResult_t<bool> Validate() const = 0;
        virtual std::vector<std::string> GetValidationErrors() const = 0;
        virtual void ResetToDefaults() = 0;

        // Optional interface
        virtual std::string GetDescription() const { return ""; }
        virtual std::string GetVersion() const { return "1.0"; }
        virtual bool RequiresRestart() const { return false; }
    };

    // Type trait for configuration sections (Traditional approach)
    template<typename T>
    struct is_config_section {
        static constexpr bool value = std::is_base_of_v<IConfigSection, T>&& std::is_default_constructible_v<T>;
    };

    template<typename T>
    constexpr bool is_config_section_v = is_config_section<T>::value;

} // namespace Akhanda::Configuration