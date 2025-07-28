// JsonWrapper.cpp
// Akhanda Game Engine - Traditional JSON Wrapper Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#include "JsonWrapper.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

namespace Akhanda::Configuration::detail {

    class JsonImpl {
    public:
        nlohmann::json data;

        JsonImpl() = default;
        JsonImpl(const nlohmann::json& json) : data(json) {}
        JsonImpl(nlohmann::json&& json) : data(std::move(json)) {}
        JsonImpl(const JsonImpl& other) : data(other.data) {}
        JsonImpl(JsonImpl&& other) noexcept : data(std::move(other.data)) {}

        JsonImpl& operator=(const JsonImpl& other) {
            if (this != &other) {
                data = other.data;
            }
            return *this;
        }

        JsonImpl& operator=(JsonImpl&& other) noexcept {
            if (this != &other) {
                data = std::move(other.data);
            }
            return *this;
        }
    };

} // namespace Akhanda::Configuration::detail

namespace Akhanda::Configuration {

    // ============================================================================
    // JsonValue Implementation
    // ============================================================================

    JsonValue::JsonValue() : impl_(std::make_unique<detail::JsonImpl>()) {}

    JsonValue::JsonValue(const JsonValue& other)
        : impl_(std::make_unique<detail::JsonImpl>(*other.impl_)) {
    }

    JsonValue::JsonValue(JsonValue&& other) noexcept = default;

    JsonValue::~JsonValue() = default;

    JsonValue& JsonValue::operator=(const JsonValue& other) {
        if (this != &other) {
            impl_ = std::make_unique<detail::JsonImpl>(*other.impl_);
        }
        return *this;
    }

    JsonValue& JsonValue::operator=(JsonValue&& other) noexcept = default;

    bool JsonValue::IsNull() const { return impl_->data.is_null(); }
    bool JsonValue::IsObject() const { return impl_->data.is_object(); }
    bool JsonValue::IsArray() const { return impl_->data.is_array(); }
    bool JsonValue::IsString() const { return impl_->data.is_string(); }
    bool JsonValue::IsNumber() const { return impl_->data.is_number(); }
    bool JsonValue::IsBool() const { return impl_->data.is_boolean(); }

    // Template specializations for GetValue
    template<>
    std::string JsonValue::GetValue<std::string>() const {
        return impl_->data.get<std::string>();
    }

    template<>
    bool JsonValue::GetValue<bool>() const {
        return impl_->data.get<bool>();
    }

    template<>
    int JsonValue::GetValue<int>() const {
        return impl_->data.get<int>();
    }

    template<>
    std::uint32_t JsonValue::GetValue<std::uint32_t>() const {
        return impl_->data.get<std::uint32_t>();
    }

    template<>
    float JsonValue::GetValue<float>() const {
        return impl_->data.get<float>();
    }

    template<>
    double JsonValue::GetValue<double>() const {
        return impl_->data.get<double>();
    }

    template<>
    std::vector<std::string> JsonValue::GetValue<std::vector<std::string>>() const {
        std::vector<std::string> result;
        if (impl_->data.is_array()) {
            for (const auto& item : impl_->data) {
                if (item.is_string()) {
                    result.push_back(item.get<std::string>());
                }
            }
        }
        return result;
    }

    // Template specializations for GetValueOr
    template<>
    std::string JsonValue::GetValueOr<std::string>(const std::string& defaultValue) const {
        try {
            return impl_->data.get<std::string>();
        }
        catch (...) {
            return defaultValue;
        }
    }

    template<>
    bool JsonValue::GetValueOr<bool>(const bool& defaultValue) const {
        try {
            return impl_->data.get<bool>();
        }
        catch (...) {
            return defaultValue;
        }
    }

    template<>
    int JsonValue::GetValueOr<int>(const int& defaultValue) const {
        try {
            return impl_->data.get<int>();
        }
        catch (...) {
            return defaultValue;
        }
    }

    template<>
    std::uint32_t JsonValue::GetValueOr<std::uint32_t>(const std::uint32_t& defaultValue) const {
        try {
            return impl_->data.get<std::uint32_t>();
        }
        catch (...) {
            return defaultValue;
        }
    }

    template<>
    float JsonValue::GetValueOr<float>(const float& defaultValue) const {
        try {
            return impl_->data.get<float>();
        }
        catch (...) {
            return defaultValue;
        }
    }

    template<>
    double JsonValue::GetValueOr<double>(const double& defaultValue) const {
        try {
            return impl_->data.get<double>();
        }
        catch (...) {
            return defaultValue;
        }
    }

    template<>
    std::vector<std::string> JsonValue::GetValueOr<std::vector<std::string>>(const std::vector<std::string>& defaultValue) const {
        try {
            return GetValue<std::vector<std::string>>();
        }
        catch (...) {
            return defaultValue;
        }
    }

    // Template specializations for SetValue
    template<>
    void JsonValue::SetValue<std::string>(const std::string& value) {
        impl_->data = value;
    }

    template<>
    void JsonValue::SetValue<bool>(const bool& value) {
        impl_->data = value;
    }

    template<>
    void JsonValue::SetValue<int>(const int& value) {
        impl_->data = value;
    }

    template<>
    void JsonValue::SetValue<std::uint32_t>(const std::uint32_t& value) {
        impl_->data = value;
    }

    template<>
    void JsonValue::SetValue<float>(const float& value) {
        impl_->data = value;
    }

    template<>
    void JsonValue::SetValue<double>(const double& value) {
        impl_->data = value;
    }

    template<>
    void JsonValue::SetValue<std::vector<std::string>>(const std::vector<std::string>& value) {
        impl_->data = nlohmann::json::array();
        for (const auto& str : value) {
            impl_->data.push_back(str);
        }
    }

    bool JsonValue::Contains(const std::string& key) const {
        return impl_->data.contains(key);
    }

    JsonValue JsonValue::operator[](const std::string& key) const {
        JsonValue result;
        result.impl_->data = impl_->data[key];
        return result;
    }

    void JsonValue::SetMember(const std::string& key, const JsonValue& value) {
        impl_->data[key] = value.impl_->data;
    }

    size_t JsonValue::Size() const {
        return impl_->data.size();
    }

    JsonValue JsonValue::At(size_t index) const {
        JsonValue result;
        result.impl_->data = impl_->data.at(index);
        return result;
    }

    std::string JsonValue::ToString(int indent) const {
        return impl_->data.dump(indent);
    }

    JsonValue JsonValue::FromFile(const std::filesystem::path& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filePath.string());
        }

        JsonValue result;
        file >> result.impl_->data;
        return result;
    }

    bool JsonValue::ToFile(const std::filesystem::path& filePath, int indent) const {
        try {
            std::ofstream file(filePath);
            if (!file.is_open()) {
                return false;
            }

            file << impl_->data.dump(indent);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    JsonValue JsonValue::Parse(const std::string& jsonString) {
        JsonValue result;
        result.impl_->data = nlohmann::json::parse(jsonString);
        return result;
    }

    bool JsonValue::operator==(const JsonValue& other) const {
        return impl_->data == other.impl_->data;
    }

    bool JsonValue::operator!=(const JsonValue& other) const {
        return !(*this == other);
    }

    // ============================================================================
    // ConfigJson Implementation
    // ============================================================================

    JsonValue ConfigJson::CreateObject() {
        JsonValue result;
        result.impl_->data = nlohmann::json::object();
        return result;
    }

    JsonValue ConfigJson::CreateArray() {
        JsonValue result;
        result.impl_->data = nlohmann::json::array();
        return result;
    }

    JsonValue ConfigJson::Merge(const JsonValue& base, const JsonValue& overlay) {
        JsonValue result = base;
        if (base.IsObject() && overlay.IsObject()) {
            for (auto it = overlay.impl_->data.begin(); it != overlay.impl_->data.end(); ++it) {
                result.impl_->data[it.key()] = it.value();
            }
        }
        return result;
    }

    JsonValue ConfigJson::GetSection(const JsonValue& json, const std::string& sectionName) {
        if (json.Contains(sectionName)) {
            return json[sectionName];
        }
        return CreateObject();
    }

    void ConfigJson::SetSection(JsonValue& json, const std::string& sectionName, const JsonValue& section) {
        json.impl_->data[sectionName] = section.impl_->data;
    }

    bool ConfigJson::ValidateConfigStructure(const JsonValue& json) {
        return json.IsObject();
    }

    std::string ConfigJson::GetErrorMessage(const std::exception& e, const std::string& context) {
        std::string message = e.what();
        if (!context.empty()) {
            message = context + ": " + message;
        }
        return message;
    }

} // namespace Akhanda::Configuration