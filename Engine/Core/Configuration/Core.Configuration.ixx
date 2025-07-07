// Core.Configuration.ixx
// Akhanda Game Engine - Configuration Library Module Interface
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include <nlohmann/json.hpp>

export module Akhanda.Core.Configuration;

import std;

// Forward declare nlohmann::json in export scope
export using json = nlohmann::json;

export namespace Akhanda::Configuration {

    // ============================================================================
    // Configuration Error Handling
    // ============================================================================

    enum class ConfigResult : uint32_t {
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
        uint32_t line = 0;
        uint32_t column = 0;

        ConfigError(ConfigResult r, std::string msg, std::string ctx = "")
            : result(r), message(std::move(msg)), context(std::move(ctx)) {
        }

        bool IsSuccess() const noexcept { return result == ConfigResult::Success; }
        explicit operator bool() const noexcept { return IsSuccess(); }
    };

    template<typename T>
    class ConfigResult_t {
    public:
        ConfigResult_t(T&& value) : value_(std::forward<T>(value)), hasValue_(true), error_(ConfigResult::Success, "") {}
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
    // Configuration Value Wrapper
    // ============================================================================

    template<typename T>
    class ConfigValue {
    public:
        ConfigValue() = default;

        explicit ConfigValue(T defaultValue)
            : value_(std::move(defaultValue)), defaultValue_(value_), hasValue_(true) {
        }

        ConfigValue(T defaultValue, T min, T max) requires std::totally_ordered<T>
            : value_(std::move(defaultValue)), defaultValue_(value_),
            minValue_(min), maxValue_(max), hasValidation_(true), hasValue_(true) {
        }

        // Get current value
        const T& Get() const noexcept { return value_; }
        const T& operator*() const noexcept { return value_; }
        const T* operator->() const noexcept { return &value_; }

        // Set value with validation
        ConfigResult_t<T> Set(T newValue) {
            if (hasValidation_) {
                if (newValue < minValue_ || newValue > maxValue_) {
                    return ConfigError(ConfigResult::ValidationError,
                        std::format("Value {} is outside range [{}, {}]", newValue, minValue_, maxValue_));
                }
            }

            if (customValidator_ && !customValidator_(newValue)) {
                return ConfigError(ConfigResult::ValidationError, "Custom validation failed");
            }

            T oldValue = value_;
            value_ = std::move(newValue);
            hasValue_ = true;

            // Notify listeners
            for (auto& callback : changeCallbacks_) {
                callback(oldValue, value_);
            }

            return value_;
        }

        // Reset to default
        void ResetToDefault() noexcept {
            value_ = defaultValue_;
            hasValue_ = true;
        }

        // Validation
        bool HasValidation() const noexcept { return hasValidation_; }
        const T& GetMin() const noexcept { return minValue_; }
        const T& GetMax() const noexcept { return maxValue_; }
        const T& GetDefault() const noexcept { return defaultValue_; }

        // Change notification
        using ChangeCallback = std::function<void(const T& oldValue, const T& newValue)>;
        void OnChange(ChangeCallback callback) { changeCallbacks_.emplace_back(std::move(callback)); }

        // Custom validation
        using Validator = std::function<bool(const T&)>;
        void SetValidator(Validator validator) { customValidator_ = std::move(validator); }

        // JSON serialization
        json ToJson() const {
            json j;
            j["value"] = value_;
            j["default"] = defaultValue_;
            if (hasValidation_) {
                j["min"] = minValue_;
                j["max"] = maxValue_;
            }
            return j;
        }

        ConfigResult_t<bool> FromJson(const json& j) {
            try {
                if (!j.contains("value")) {
                    return ConfigError(ConfigResult::KeyNotFound, "Missing 'value' field");
                }

                T newValue = j["value"].get<T>();
                return Set(std::move(newValue));
            }
            catch (const json::exception& e) {
                return ConfigError(ConfigResult::ParseError,
                    std::format("JSON parse error: {}", e.what()));
            }
        }

    private:
        T value_{};
        T defaultValue_{};
        T minValue_{};
        T maxValue_{};
        bool hasValidation_ = false;
        bool hasValue_ = false;
        std::vector<ChangeCallback> changeCallbacks_;
        Validator customValidator_;
    };

    // ============================================================================
    // Configuration Section Interface
    // ============================================================================

    class IConfigSection {
    public:
        virtual ~IConfigSection() = default;

        // Core interface
        virtual const char* GetSectionName() const noexcept = 0;
        virtual ConfigResult_t<bool> LoadFromJson(const json& sectionJson) = 0;
        virtual json SaveToJson() const = 0;
        virtual void ResetToDefaults() = 0;

        // Validation
        virtual ConfigResult_t<bool> Validate() const = 0;
        virtual std::vector<std::string> GetValidationErrors() const = 0;

        // Metadata
        virtual uint32_t GetVersion() const noexcept { return 1; }
        virtual std::string GetDescription() const { return "Configuration section"; }

        // Change notification
        using SectionChangeCallback = std::function<void(const IConfigSection&)>;
        void OnSectionChange(SectionChangeCallback callback) {
            sectionChangeCallbacks_.emplace_back(std::move(callback));
        }

    protected:
        void NotifySectionChanged() {
            for (auto& callback : sectionChangeCallbacks_) {
                callback(*this);
            }
        }

    private:
        std::vector<SectionChangeCallback> sectionChangeCallbacks_;
    };

    // ============================================================================
    // Configuration Section Concept
    // ============================================================================

    template<typename T>
    concept ConfigSection = std::derived_from<T, IConfigSection>&& requires {
        { T::SECTION_NAME } -> std::convertible_to<const char*>;
    }&& std::default_initializable<T>;

    // ============================================================================
    // Configuration Registry
    // ============================================================================

    class ConfigSectionRegistry {
    public:
        using CreateFunc = std::function<std::unique_ptr<IConfigSection>()>;

        template<ConfigSection T>
        static void RegisterSection() {
            auto& instance = GetInstance();
            std::string name = T::SECTION_NAME;

            instance.creators_[name] = []() -> std::unique_ptr<IConfigSection> {
                return std::make_unique<T>();
                };

            instance.typeInfo_[name] = typeid(T);
        }

        static std::unique_ptr<IConfigSection> CreateSection(const std::string& name) {
            auto& instance = GetInstance();
            auto it = instance.creators_.find(name);
            return it != instance.creators_.end() ? it->second() : nullptr;
        }

        static std::vector<std::string> GetRegisteredSections() {
            auto& instance = GetInstance();
            std::vector<std::string> sections;
            sections.reserve(instance.creators_.size());

            for (const auto& [name, _] : instance.creators_) {
                sections.emplace_back(name);
            }
            return sections;
        }

        static bool IsSectionRegistered(const std::string& name) {
            auto& instance = GetInstance();
            return instance.creators_.contains(name);
        }

    private:
        static ConfigSectionRegistry& GetInstance() {
            static ConfigSectionRegistry instance;
            return instance;
        }

        std::unordered_map<std::string, CreateFunc> creators_;
        std::unordered_map<std::string, std::type_info> typeInfo_;
    };

    // ============================================================================
    // Configuration Helper Macros
    // ============================================================================

#define AKH_DECLARE_CONFIG_SECTION(ClassName, SectionName) \
        public: \
            static constexpr const char* SECTION_NAME = SectionName; \
            const char* GetSectionName() const noexcept override { return SECTION_NAME; } \
        private:

#define AKH_REGISTER_CONFIG_SECTION(ClassName) \
        namespace { \
            struct ClassName##_Registrar { \
                ClassName##_Registrar() { \
                    ::Akhanda::Configuration::ConfigSectionRegistry::RegisterSection<ClassName>(); \
                } \
            }; \
            static ClassName##_Registrar g_##ClassName##_registrar; \
        }

    // ============================================================================
    // Common Configuration Types
    // ============================================================================

    struct Resolution {
        uint32_t width = 1920;
        uint32_t height = 1080;

        bool operator==(const Resolution&) const = default;

        json ToJson() const {
            return json{ {"width", width}, {"height", height} };
        }

        static ConfigResult_t<Resolution> FromJson(const json& j) {
            try {
                Resolution res;
                res.width = j.at("width").get<uint32_t>();
                res.height = j.at("height").get<uint32_t>();

                if (res.width == 0 || res.height == 0) {
                    return ConfigError(ConfigResult::ValidationError, "Resolution cannot be zero");
                }

                return res;
            }
            catch (const json::exception& e) {
                return ConfigError(ConfigResult::ParseError,
                    std::format("Failed to parse Resolution: {}", e.what()));
            }
        }
    };

    // JSON serialization helpers for ConfigValue
    template<typename T>
    void to_json(json& j, const ConfigValue<T>& value) {
        j = value.ToJson();
    }

    template<typename T>
    void from_json(const json& j, ConfigValue<T>& value) {
        auto result = value.FromJson(j);
        if (!result) {
            throw json::other_error::create(0, result.Error().message, nullptr);
        }
    }

    // JSON serialization for Resolution
    inline void to_json(json& j, const Resolution& res) {
        j = res.ToJson();
    }

    inline void from_json(const json& j, Resolution& res) {
        auto result = Resolution::FromJson(j);
        if (!result) {
            throw json::other_error::create(0, result.Error().message, nullptr);
        }
        res = result.Value();
    }

} // namespace Akhanda::Configuration