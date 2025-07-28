// JsonWrapper.hpp
// Akhanda Game Engine - Traditional JSON Wrapper Header
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include <string>
#include <filesystem>
#include <memory>
#include <cstdint>

namespace Akhanda::Configuration {

    // Forward declaration to hide nlohmann::json from module interface
    namespace detail {
        class JsonImpl;
    }

    class ConfigJson;

    // ============================================================================
    // JSON Wrapper Class - Completely hides nlohmann::json
    // ============================================================================

    class JsonValue {
    private:
        std::unique_ptr<detail::JsonImpl> impl_;

        friend class ConfigJson;

    public:
        // Constructors
        JsonValue();
        JsonValue(const JsonValue& other);
        JsonValue(JsonValue&& other) noexcept;
        ~JsonValue();

        // Assignment operators
        JsonValue& operator=(const JsonValue& other);
        JsonValue& operator=(JsonValue&& other) noexcept;

        // Type checking
        bool IsNull() const;
        bool IsObject() const;
        bool IsArray() const;
        bool IsString() const;
        bool IsNumber() const;
        bool IsBool() const;

        // Value access with type safety
        template<typename T>
        T GetValue() const;

        template<typename T>
        T GetValueOr(const T& defaultValue) const;

        // Set values
        template<typename T>
        void SetValue(const T& value);

        // Object operations
        bool Contains(const std::string& key) const;
        JsonValue operator[](const std::string& key) const;
        void SetMember(const std::string& key, const JsonValue& value);

        // Array operations
        size_t Size() const;
        JsonValue At(size_t index) const;

        // String operations
        std::string ToString(int indent = -1) const;

        // File operations
        static JsonValue FromFile(const std::filesystem::path& filePath);
        bool ToFile(const std::filesystem::path& filePath, int indent = 2) const;

        // Parse from string
        static JsonValue Parse(const std::string& jsonString);

        // Comparison operators
        bool operator==(const JsonValue& other) const;
        bool operator!=(const JsonValue& other) const;

        // Internal access (for implementation only)
        detail::JsonImpl* GetImpl() const { return impl_.get(); }
    };

    // ============================================================================
    // Configuration-specific JSON utilities
    // ============================================================================

    class ConfigJson {
    public:
        // Create empty object
        static JsonValue CreateObject();

        // Create empty array
        static JsonValue CreateArray();

        // Merge two JSON objects
        static JsonValue Merge(const JsonValue& base, const JsonValue& overlay);

        // Extract section from larger JSON
        static JsonValue GetSection(const JsonValue& json, const std::string& sectionName);

        // Set section in larger JSON
        static void SetSection(JsonValue& json, const std::string& sectionName, const JsonValue& section);

        // Validate JSON structure for configuration
        static bool ValidateConfigStructure(const JsonValue& json);

        // Convert JSON exceptions to friendly error messages
        static std::string GetErrorMessage(const std::exception& e, const std::string& context = "");
    };

    // ============================================================================
    // Template helpers for common configuration types
    // ============================================================================

    template<typename T>
    T GetConfigValue(const JsonValue& json, const std::string& key, const T& defaultValue = T{}) {
        if (!json.Contains(key)) {
            return defaultValue;
        }

        try {
            auto valueJson = json[key];
            return valueJson.GetValue<T>();
        }
        catch (...) {
            return defaultValue;
        }
    }

    template<typename T>
    void SetConfigValue(JsonValue& json, const std::string& key, const T& value) {
        JsonValue valueJson;
        valueJson.SetValue(value);
        json.SetMember(key, valueJson);
    }

    // Template specializations declared here, defined in .cpp
    template<> std::string JsonValue::GetValue<std::string>() const;
    template<> bool JsonValue::GetValue<bool>() const;
    template<> int JsonValue::GetValue<int>() const;
    template<> std::uint32_t JsonValue::GetValue<std::uint32_t>() const;
    template<> float JsonValue::GetValue<float>() const;
    template<> double JsonValue::GetValue<double>() const;
    template<> std::vector<std::string> JsonValue::GetValue<std::vector<std::string>>() const;

    template<> std::string JsonValue::GetValueOr<std::string>(const std::string& defaultValue) const;
    template<> bool JsonValue::GetValueOr<bool>(const bool& defaultValue) const;
    template<> int JsonValue::GetValueOr<int>(const int& defaultValue) const;
    template<> std::uint32_t JsonValue::GetValueOr<std::uint32_t>(const std::uint32_t& defaultValue) const;
    template<> float JsonValue::GetValueOr<float>(const float& defaultValue) const;
    template<> double JsonValue::GetValueOr<double>(const double& defaultValue) const;
    template<> std::vector<std::string> JsonValue::GetValueOr<std::vector<std::string>>(const std::vector<std::string>& defaultValue) const;

    template<> void JsonValue::SetValue<std::string>(const std::string& value);
    template<> void JsonValue::SetValue<bool>(const bool& value);
    template<> void JsonValue::SetValue<int>(const int& value);
    template<> void JsonValue::SetValue<std::uint32_t>(const std::uint32_t& value);
    template<> void JsonValue::SetValue<float>(const float& value);
    template<> void JsonValue::SetValue<double>(const double& value);
    template<> void JsonValue::SetValue<std::vector<std::string>>(const std::vector<std::string>& value);

} // namespace Akhanda::Configuration