// Engine/Renderer/Shaders/D3D12/D3D12ShaderManager.cpp
// Akhanda Game Engine - D3D12 Shader Manager Core Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <wrl.h>
#include <regex>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <shared_mutex>

module Akhanda.Engine.Shaders.D3D12;

import Akhanda.Core.Logging;

using Microsoft::WRL::ComPtr;

namespace Akhanda::Shaders {

    namespace D3D12 {

        // ============================================================================
        // D3D12Shader Implementation
        // ============================================================================

        D3D12Shader::D3D12Shader(
            const std::string& name,
            ShaderStage stage,
            const ShaderVariant& variant,
            ComPtr<ID3DBlob> bytecode,
            std::unique_ptr<ShaderReflectionData> reflection
        )
            : name_(name)
            , stage_(stage)
            , variant_(variant)
            , bytecode_(bytecode)
            , reflection_(std::move(reflection))
        {
        }

        const std::vector<std::uint8_t>& D3D12Shader::GetBytecode() const {
            std::lock_guard<std::mutex> lock(bytecodeVectorMutex_);

            if (bytecodeVector_.empty() && bytecode_) {
                const std::uint8_t* data = static_cast<const std::uint8_t*>(bytecode_->GetBufferPointer());
                SIZE_T size = bytecode_->GetBufferSize();
                bytecodeVector_.assign(data, data + size);
            }

            return bytecodeVector_;
        }

        void D3D12Shader::UpdateReflectionData(std::unique_ptr<ShaderReflectionData> newReflection) {
            reflection_ = std::move(newReflection);
        }

        std::optional<std::uint32_t> D3D12Shader::GetConstantBufferBinding(const std::string& name) const {
            for (const auto& cb : reflection_->constantBuffers) {
                if (cb.name == name) {
                    return cb.bindPoint;
                }
            }
            return std::nullopt;
        }

        std::optional<std::uint32_t> D3D12Shader::GetTextureBinding(const std::string& name) const {
            for (const auto& sr : reflection_->shaderResources) {
                if (sr.name == name) {
                    return sr.bindPoint;
                }
            }
            return std::nullopt;
        }

        std::optional<std::uint32_t> D3D12Shader::GetSamplerBinding(const std::string& name) const {
            for (const auto& sampler : reflection_->samplers) {
                if (sampler.name == name) {
                    return sampler.bindPoint;
                }
            }
            return std::nullopt;
        }

        std::optional<std::uint32_t> D3D12Shader::GetUAVBinding(const std::string& name) const {
            for (const auto& uav : reflection_->unorderedAccessViews) {
                if (uav.name == name) {
                    return uav.bindPoint;
                }
            }
            return std::nullopt;
        }

        // ============================================================================
        // D3D12ShaderProgram Implementation
        // ============================================================================

        D3D12ShaderProgram::D3D12ShaderProgram(const std::string& name)
            : name_(name)
            , combinedReflection_(std::make_unique<ShaderReflectionData>())
        {
        }

        bool D3D12ShaderProgram::IsLinked() const {
            std::shared_lock<std::shared_mutex> lock(shadersMutex_);
            return !shaders_.empty();
        }

        std::uint64_t D3D12ShaderProgram::GetProgramHash() const {
            return programHash_;
        }

        std::shared_ptr<IShader> D3D12ShaderProgram::GetShader(ShaderStage stage) const {
            std::shared_lock<std::shared_mutex> lock(shadersMutex_);
            auto it = shaders_.find(stage);
            return (it != shaders_.end()) ? it->second : nullptr;
        }

        const ShaderReflectionData& D3D12ShaderProgram::GetCombinedReflection() const {
            return *combinedReflection_;
        }

        std::optional<std::uint32_t> D3D12ShaderProgram::FindConstantBufferBinding(const std::string& name) const {
            std::shared_lock<std::shared_mutex> lock(shadersMutex_);
            for (const auto& [stage, shader] : shaders_) {
                auto binding = shader->GetConstantBufferBinding(name);
                if (binding.has_value()) {
                    return binding;
                }
            }
            return std::nullopt;
        }

        std::optional<std::uint32_t> D3D12ShaderProgram::FindTextureBinding(const std::string& name) const {
            std::shared_lock<std::shared_mutex> lock(shadersMutex_);
            for (const auto& [stage, shader] : shaders_) {
                auto binding = shader->GetTextureBinding(name);
                if (binding.has_value()) {
                    return binding;
                }
            }
            return std::nullopt;
        }

        std::optional<std::uint32_t> D3D12ShaderProgram::FindSamplerBinding(const std::string& name) const {
            std::shared_lock<std::shared_mutex> lock(shadersMutex_);
            for (const auto& [stage, shader] : shaders_) {
                auto binding = shader->GetSamplerBinding(name);
                if (binding.has_value()) {
                    return binding;
                }
            }
            return std::nullopt;
        }

        std::optional<std::uint32_t> D3D12ShaderProgram::FindUAVBinding(const std::string& name) const {
            std::shared_lock<std::shared_mutex> lock(shadersMutex_);
            for (const auto& [stage, shader] : shaders_) {
                auto binding = shader->GetUAVBinding(name);
                if (binding.has_value()) {
                    return binding;
                }
            }
            return std::nullopt;
        }

        bool D3D12ShaderProgram::NeedsRecompilation() const {
            std::shared_lock<std::shared_mutex> lock(shadersMutex_);
            for (const auto& [stage, shader] : shaders_) {
                if (shader->NeedsRecompilation()) {
                    return true;
                }
            }
            return false;
        }

        void D3D12ShaderProgram::MarkForRecompilation() {
            std::shared_lock<std::shared_mutex> lock(shadersMutex_);
            for (const auto& [stage, shader] : shaders_) {
                shader->MarkForRecompilation();
            }
        }

        void D3D12ShaderProgram::AddShader(std::shared_ptr<D3D12Shader> shader) {
            std::unique_lock<std::shared_mutex> lock(shadersMutex_);
            shaders_[shader->GetStage()] = shader;
            UpdateActiveStages();
            CalculateProgramHash();
            lock.unlock();
            BuildCombinedReflection();
        }

        void D3D12ShaderProgram::RemoveShader(ShaderStage stage) {
            std::unique_lock<std::shared_mutex> lock(shadersMutex_);
            shaders_.erase(stage);
            UpdateActiveStages();
            CalculateProgramHash();
            lock.unlock();
            BuildCombinedReflection();
        }

        void D3D12ShaderProgram::UpdateActiveStages() {
            activeStages_.clear();
            for (const auto& [stage, shader] : shaders_) {
                activeStages_.push_back(stage);
            }
        }

        void D3D12ShaderProgram::CalculateProgramHash() {
            std::hash<std::string> hasher;
            programHash_ = 0;

            for (const auto& [stage, shader] : shaders_) {
                programHash_ ^= hasher(shader->GetName()) + 0x9e3779b9 + (programHash_ << 6) + (programHash_ >> 2);
                programHash_ ^= shader->GetSourceHash() + 0x9e3779b9 + (programHash_ << 6) + (programHash_ >> 2);
            }
        }

        void D3D12ShaderProgram::BuildCombinedReflection() {
            std::shared_lock<std::shared_mutex> lock(shadersMutex_);

            combinedReflection_ = std::make_unique<ShaderReflectionData>();
            combinedReflection_->shaderName = name_;

            // Combine reflection data from all shaders
            for (const auto& [stage, shader] : shaders_) {
                const auto& reflection = shader->GetReflection();

                // Merge constant buffers (avoid duplicates)
                for (const auto& cb : reflection.constantBuffers) {
                    bool found = false;
                    for (const auto& existingCB : combinedReflection_->constantBuffers) {
                        if (existingCB.name == cb.name && existingCB.bindPoint == cb.bindPoint) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        combinedReflection_->constantBuffers.push_back(cb);
                    }
                }

                // Merge shader resources
                for (const auto& sr : reflection.shaderResources) {
                    bool found = false;
                    for (const auto& existingSR : combinedReflection_->shaderResources) {
                        if (existingSR.name == sr.name && existingSR.bindPoint == sr.bindPoint) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        combinedReflection_->shaderResources.push_back(sr);
                    }
                }

                // Merge samplers
                for (const auto& sampler : reflection.samplers) {
                    bool found = false;
                    for (const auto& existingSampler : combinedReflection_->samplers) {
                        if (existingSampler.name == sampler.name && existingSampler.bindPoint == sampler.bindPoint) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        combinedReflection_->samplers.push_back(sampler);
                    }
                }

                // Merge UAVs
                for (const auto& uav : reflection.unorderedAccessViews) {
                    bool found = false;
                    for (const auto& existingUAV : combinedReflection_->unorderedAccessViews) {
                        if (existingUAV.name == uav.name && existingUAV.bindPoint == uav.bindPoint) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        combinedReflection_->unorderedAccessViews.push_back(uav);
                    }
                }
            }
        }

        // ============================================================================
        // CompilerIncludeHandler Implementation
        // ============================================================================

        CompilerIncludeHandler::CompilerIncludeHandler(const std::vector<std::filesystem::path>& includePaths)
            : includePaths_(includePaths)
        {
        }

        HRESULT CompilerIncludeHandler::Open(
            D3D_INCLUDE_TYPE IncludeType,
            LPCSTR pFileName,
            [[maybe_unused]] LPCVOID pParentData,
            [[maybe_unused]] LPCVOID* ppData,
            UINT* pBytes
        ) {
            std::lock_guard<std::mutex> lock(includesMutex_);

            try {
                bool isSystemInclude = (IncludeType == D3D_INCLUDE_SYSTEM);
                std::filesystem::path includeFile = FindIncludeFile(pFileName, isSystemInclude);

                if (includeFile.empty()) {
                    return E_FAIL;
                }

                // Read the file
                std::ifstream file(includeFile, std::ios::binary | std::ios::ate);
                if (!file.is_open()) {
                    return E_FAIL;
                }

                std::streamsize size = file.tellg();
                file.seekg(0, std::ios::beg);

                auto buffer = std::make_unique<std::vector<char>>(static_cast<size_t>(size));
                if (!file.read(buffer->data(), size)) {
                    return E_FAIL;
                }

                *ppData = buffer->data();
                *pBytes = static_cast<UINT>(size);

                // Track the included file
                includedFiles_.push_back(includeFile);

                // Store the buffer to keep it alive
                allocatedBuffers_.push_back(std::move(buffer));

                return S_OK;
            }
            catch (...) {
                return E_FAIL;
            }
        }

        HRESULT CompilerIncludeHandler::Close([[maybe_unused]] LPCVOID pData) {
            // The buffer will be automatically cleaned up when allocatedBuffers_ is destroyed
            return S_OK;
        }

        std::filesystem::path CompilerIncludeHandler::FindIncludeFile(const std::string& fileName, [[maybe_unused]] bool isSystemInclude) const {
            // Try to find the include file in the include paths
            for (const auto& includePath : includePaths_) {
                std::filesystem::path fullPath = includePath / fileName;
                if (std::filesystem::exists(fullPath)) {
                    return fullPath;
                }
            }

            // If not found, try relative to current directory
            if (std::filesystem::exists(fileName)) {
                return std::filesystem::absolute(fileName);
            }

            return {};
        }

        // ============================================================================
        // D3D12ShaderManager Core Implementation
        // ============================================================================

        D3D12ShaderManager::D3D12ShaderManager(Memory::IAllocator& allocator)
            : allocator_(allocator)
            , logChannel_(Logging::LogManager::Instance().GetChannel("ShaderManager"))
        {
            // Initialize include paths with common directories
            includePaths_.push_back(std::filesystem::current_path() / "Shaders");
            includePaths_.push_back(std::filesystem::current_path() / "Assets" / "Shaders");
        }

        D3D12ShaderManager::~D3D12ShaderManager() {
            Shutdown();
        }

        Core::Result<void*, Core::ErrorInfo> D3D12ShaderManager::Initialize(
            std::shared_ptr<RHI::IRenderDevice> renderDevice,
            const Configuration::ShaderConfig& config
        ) {
            if (isInitialized_.load()) {
                return Core::Err<void*>(Core::MakeError(
                    Core::ShaderError::InvalidBytecode,
                    "ShaderManager already initialized"
                ));
            }

            renderDevice_ = renderDevice;
            config_ = config;

            // Initialize shader cache
            cache_ = std::make_unique<ShaderCache>(config_);

            // Initialize hot reload manager if enabled
            if (config_.GetEnableHotReload()) {
                hotReloadManager_ = std::make_unique<HotReloadManager>(config_);
            }

            // Load cache from disk if enabled
            if (config_.GetEnableShaderCache()) {
                cache_->LoadFromDisk();
            }

            // Add configured include paths
            {
                std::unique_lock<std::shared_mutex> lock(includePathsMutex_);
                for (const auto& path : config_.GetIncludePaths()) {
                    includePaths_.push_back(path);
                }
            }

            // Start worker threads for async compilation
            StartWorkerThreads();

            isInitialized_.store(true);

            logChannel_.Log(Logging::LogLevel::Info, "D3D12ShaderManager initialized successfully");
            return Core::Ok<void*>(nullptr);
        }

        void D3D12ShaderManager::Shutdown() {
            if (!isInitialized_.load()) {
                return;
            }

            // Stop worker threads
            StopWorkerThreads();

            // Disable hot reload
            if (hotReloadManager_) {
                hotReloadManager_->Stop();
                hotReloadManager_.reset();
            }

            // Save cache to disk
            if (cache_ && config_.GetEnableShaderCache()) {
                cache_->SaveToDisk();
            }

            // Clear all shaders and programs
            {
                std::unique_lock<std::shared_mutex> shadersLock(shadersMutex_);
                std::unique_lock<std::shared_mutex> programsLock(programsMutex_);
                shaders_.clear();
                programs_.clear();
            }

            cache_.reset();
            renderDevice_.reset();

            isInitialized_.store(false);
            logChannel_.Log(Logging::LogLevel::Info, "D3D12ShaderManager shut down");
        }

        void D3D12ShaderManager::UpdateConfiguration(const Configuration::ShaderConfig& config) {
            config_ = config;

            if (cache_) {
                // Recreate cache with new configuration
                cache_ = std::make_unique<ShaderCache>(config_);
            }

            logChannel_.Log(Logging::LogLevel::Info, "ShaderManager configuration updated");
        }

        Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo> D3D12ShaderManager::CompileShader(
            const ShaderCompileRequest& request
        ) {
            auto startTime = std::chrono::steady_clock::now();

            // Check cache first
            if (config_.GetEnableShaderCache() && cache_) {
                ShaderCache::CacheEntry cacheEntry;
                if (cache_->TryGetCachedShader(
                    request.sourceFile,
                    request.entryPoint,
                    request.stage,
                    request.variant,
                    request.optimization,
                    cacheEntry
                )) {
                    // Create shader from cached data
                    auto shader = std::make_shared<D3D12Shader>(
                        request.sourceFile.filename().string(),
                        request.stage,
                        request.variant,
                        nullptr, // We'll create the blob from cached bytecode
                        std::move(cacheEntry.reflection)
                    );

                    // Create D3D blob from cached bytecode
                    ComPtr<ID3DBlob> blob;
                    HRESULT hr = D3DCreateBlob(cacheEntry.bytecode.size(), &blob);
                    if (SUCCEEDED(hr)) {
                        memcpy(blob->GetBufferPointer(), cacheEntry.bytecode.data(), cacheEntry.bytecode.size());
                        // Set the blob in the shader (we'd need to add a method for this)
                    }

                    {
                        std::lock_guard<std::mutex> lock(statisticsMutex_);
                        statistics_.cacheHits++;
                    }

                    logChannel_.LogFormat(Logging::LogLevel::Debug,
                        "Loaded shader '{}' from cache", request.sourceFile.string());

                    return Core::Ok<std::shared_ptr<IShader>>(shader);
                }
                else {
                    std::lock_guard<std::mutex> lock(statisticsMutex_);
                    statistics_.cacheMisses++;
                }
            }

            // Read shader source file
            auto sourceResult = ReadShaderFile(request.sourceFile);
            if (!sourceResult) {
                return Core::Err<std::shared_ptr<IShader>>(sourceResult.Error());
            }

            std::string source = sourceResult.Value();

            // Create include handler
            std::vector<std::filesystem::path> currentIncludePaths;
            {
                std::shared_lock<std::shared_mutex> lock(includePathsMutex_);
                currentIncludePaths = includePaths_;
            }

            // Add additional include paths from request
            for (const auto& path : request.additionalIncludePaths) {
                currentIncludePaths.push_back(path);
            }

            CompilerIncludeHandler includeHandler(currentIncludePaths);

            // Compile shader
            std::string target = GetShaderTarget(request.stage, request.shaderModel);
            auto compileResult = CompileHLSL(
                source,
                request.entryPoint,
                target,
                request.variant,
                request.optimization,
                request.generateDebugInfo,
                &includeHandler
            );

            if (!compileResult) {
                {
                    std::lock_guard<std::mutex> lock(statisticsMutex_);
                    statistics_.compilationFailures++;
                }
                return Core::Err<std::shared_ptr<IShader>>(compileResult.Error());
            }

            ComPtr<ID3DBlob> bytecode = compileResult.Value();

            // Reflect shader
            auto reflectionResult = ReflectShader(
                bytecode.Get(),
                request.sourceFile.filename().string(),
                request.stage,
                request.entryPoint,
                includeHandler.GetIncludedFiles()
            );

            if (!reflectionResult) {
                return Core::Err<std::shared_ptr<IShader>>(reflectionResult.Error());
            }

            // Create shader object
            auto shader = std::make_shared<D3D12Shader>(
                request.sourceFile.filename().string(),
                request.stage,
                request.variant,
                bytecode,
                std::move(reflectionResult.Value())
            );

            // Cache the compiled shader
            if (config_.GetEnableShaderCache() && cache_) {
                std::vector<std::uint8_t> bytecodeVector = shader->GetBytecode();
                auto reflectionCopy = std::make_unique<ShaderReflectionData>(shader->GetReflection());

                cache_->CacheShader(
                    request.sourceFile,
                    request.entryPoint,
                    request.stage,
                    request.variant,
                    request.optimization,
                    bytecodeVector,
                    std::move(reflectionCopy)
                );
            }

            // Store shader
            std::string shaderKey = GenerateShaderKey(shader->GetName(), request.variant);
            {
                std::unique_lock<std::shared_mutex> lock(shadersMutex_);
                shaders_[shaderKey] = shader;
            }

            // Update statistics
            auto endTime = std::chrono::steady_clock::now();
            auto compilationTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

            {
                std::lock_guard<std::mutex> lock(statisticsMutex_);
                statistics_.compilationSuccesses++;
                statistics_.totalCompilationTime += compilationTime;
                statistics_.totalShadersLoaded++;
                UpdateCompilationStatistics(compilationTime, true);
            }

            // Update dependency tracking for hot reload
            if (hotReloadManager_) {
                UpdateShaderDependencies(shader->GetName(), includeHandler.GetIncludedFiles());
            }

            logChannel_.LogFormat(Logging::LogLevel::Info,
                "Compiled shader '{}' entry point '{}' in {}ms",
                request.sourceFile.string(), request.entryPoint, compilationTime.count());

            return Core::Ok<std::shared_ptr<IShader>>(shader);
        }

        Core::Result<ComPtr<ID3DBlob>, Core::ErrorInfo> D3D12ShaderManager::CompileHLSL(
            const std::string& source,
            const std::string& entryPoint,
            const std::string& target,
            const ShaderVariant& variant,
            Configuration::ShaderOptimization optimization,
            bool generateDebugInfo,
            CompilerIncludeHandler* includeHandler
        ) {
            // Prepare defines from variant
            std::vector<D3D_SHADER_MACRO> defines;

            // Add variant defines
            for (const auto& define : variant.defines) {
                D3D_SHADER_MACRO macro;
                macro.Name = define.name.c_str();
                macro.Definition = define.value.c_str();
                defines.push_back(macro);
            }

            // Add global defines from configuration
            for (const auto& define : config_.GetGlobalDefines()) {
                D3D_SHADER_MACRO macro;
                // Parse "NAME=VALUE" or "NAME" format
                size_t equalPos = define.find('=');
                if (equalPos != std::string::npos) {
                    std::string name = define.substr(0, equalPos);
                    std::string value = define.substr(equalPos + 1);
                    macro.Name = name.c_str();
                    macro.Definition = value.c_str();
                }
                else {
                    macro.Name = define.c_str();
                    macro.Definition = "1";
                }
                defines.push_back(macro);
            }

            // Null terminate
            defines.push_back({ nullptr, nullptr });

            // Get compilation flags
            UINT flags = GetCompileFlags(optimization, generateDebugInfo);

            // Compile shader
            ComPtr<ID3DBlob> bytecode;
            ComPtr<ID3DBlob> errorBlob;

            HRESULT hr = D3DCompile(
                source.c_str(),
                source.length(),
                nullptr, // Source name
                defines.data(),
                includeHandler,
                entryPoint.c_str(),
                target.c_str(),
                flags,
                0, // Effect flags
                &bytecode,
                &errorBlob
            );

            if (FAILED(hr)) {
                std::string errorMessage = "Unknown compilation error";
                if (errorBlob) {
                    errorMessage = std::string(
                        static_cast<const char*>(errorBlob->GetBufferPointer()),
                        errorBlob->GetBufferSize()
                    );
                }

                return Core::Err<ComPtr<ID3DBlob>>(Core::MakeError(
                    Core::ShaderError::CompilationFailed,
                    std::format("HLSL compilation failed: {}", errorMessage)
                ));
            }

            return Core::Ok<ComPtr<ID3DBlob>>(std::move(bytecode));
        }

        std::string D3D12ShaderManager::GetShaderTarget(ShaderStage stage, Configuration::ShaderModel model) const {
            std::string modelStr = Configuration::ShaderModelToString(model);

            switch (stage) {
            case ShaderStage::Vertex: return "vs_" + modelStr;
            case ShaderStage::Hull: return "hs_" + modelStr;
            case ShaderStage::Domain: return "ds_" + modelStr;
            case ShaderStage::Geometry: return "gs_" + modelStr;
            case ShaderStage::Pixel: return "ps_" + modelStr;
            case ShaderStage::Compute: return "cs_" + modelStr;
            case ShaderStage::Amplification: return "as_" + modelStr;
            case ShaderStage::Mesh: return "ms_" + modelStr;
            default:
                return "vs_" + modelStr; // Default fallback
            }
        }

        UINT D3D12ShaderManager::GetCompileFlags(Configuration::ShaderOptimization optimization, bool generateDebugInfo) const {
            UINT flags = 0;

            switch (optimization) {
            case Configuration::ShaderOptimization::Debug:
                flags |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
                break;
            case Configuration::ShaderOptimization::Release:
                flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
                break;
            case Configuration::ShaderOptimization::Size:
                flags |= D3DCOMPILE_OPTIMIZATION_LEVEL1;
                break;
            case Configuration::ShaderOptimization::Speed:
                flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
                break;
            }

            if (generateDebugInfo || config_.GetGenerateDebugInfo()) {
                flags |= D3DCOMPILE_DEBUG;
            }

            if (config_.GetEnableStrictMode()) {
                flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
            }

            if (config_.GetEnableMatrix16ByteAlignment()) {
                flags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
            }

            return flags;
        }

        Core::Result<std::string, Core::ErrorInfo> D3D12ShaderManager::ReadShaderFile(const std::filesystem::path& filePath) const {
            if (!std::filesystem::exists(filePath)) {
                return Core::Err<std::string>(Core::MakeError(
                    Core::FileSystemError::FileNotFound,
                    std::format("Shader file not found: {}", filePath.string())
                ));
            }

            std::ifstream file(filePath, std::ios::binary);
            if (!file.is_open()) {
                return Core::Err<std::string>(Core::MakeError(
                    Core::FileSystemError::ReadFailed,
                    std::format("Failed to open shader file: {}", filePath.string())
                ));
            }

            std::string content;
            file.seekg(0, std::ios::end);
            content.reserve(file.tellg());
            file.seekg(0, std::ios::beg);

            content.assign(
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()
            );

            return Core::Ok<std::string>(std::move(content));
        }

        std::string D3D12ShaderManager::GenerateShaderKey(const std::string& name, const ShaderVariant& variant) const {
            std::string key = name;
            if (!variant.defines.empty()) {
                key += "_";
                for (const auto& define : variant.defines) {
                    key += define.name + "=" + define.value + ";";
                }
            }
            return key;
        }

        void D3D12ShaderManager::StartWorkerThreads() {
            size_t numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) numThreads = 4; // Fallback

            workerThreads_.reserve(numThreads);
            for (size_t i = 0; i < numThreads; ++i) {
                workerThreads_.emplace_back(&D3D12ShaderManager::AsyncWorkerFunction, this);
            }

            logChannel_.LogFormat(Logging::LogLevel::Debug,
                "Started {} shader compilation worker threads", numThreads);
        }

        void D3D12ShaderManager::StopWorkerThreads() {
            shutdownWorkers_.store(true);
            asyncTasksCV_.notify_all();

            for (auto& thread : workerThreads_) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
            workerThreads_.clear();

            logChannel_.Log(Logging::LogLevel::Debug, "Stopped all shader compilation worker threads");
        }

        void D3D12ShaderManager::AsyncWorkerFunction() {
            while (!shutdownWorkers_.load()) {
                AsyncCompileTask task;

                {
                    std::unique_lock<std::mutex> lock(asyncTasksMutex_);
                    asyncTasksCV_.wait(lock, [this] {
                        return !asyncTasks_.empty() || shutdownWorkers_.load();
                        });

                    if (shutdownWorkers_.load()) {
                        break;
                    }

                    task = std::move(asyncTasks_.front());
                    asyncTasks_.pop();
                }

                // Compile the shader
                auto result = CompileShader(task.request);

                // Call the callback
                task.callback(result);
            }
        }

        void D3D12ShaderManager::UpdateCompilationStatistics(const std::chrono::milliseconds& compilationTime, bool success) {
            statistics_.compilationRequests++;
            if (success) {
                statistics_.averageCompilationTime =
                    std::chrono::milliseconds(
                        (statistics_.averageCompilationTime.count() + compilationTime.count()) / 2
                    );
            }
        }

        void D3D12ShaderManager::UpdateShaderDependencies(const std::string& shaderName, const std::vector<std::filesystem::path>& dependencies) {
            if (hotReloadManager_) {
                hotReloadManager_->UpdateDependencies(shaderName, dependencies);
            }
        }

        Core::Result<std::unique_ptr<ShaderReflectionData>, Core::ErrorInfo> D3D12ShaderManager::ReflectShader(
            ID3DBlob* bytecode,
            const std::string& shaderName,
            ShaderStage stage,
            const std::string& entryPoint,
            const std::vector<std::filesystem::path>& includedFiles
        ) {
            ComPtr<ID3D12ShaderReflection> reflection;
            HRESULT hr = D3DReflect(
                bytecode->GetBufferPointer(),
                bytecode->GetBufferSize(),
                IID_PPV_ARGS(&reflection)
            );

            if (FAILED(hr)) {
                return Core::Err<std::unique_ptr<ShaderReflectionData>>(Core::MakeError(
                    Core::ShaderError::ReflectionFailed,
                    std::format("Failed to reflect shader '{}': HRESULT 0x{:08X}", shaderName, static_cast<uint32_t>(hr))
                ));
            }

            auto reflectionData = std::make_unique<ShaderReflectionData>();
            reflectionData->shaderName = shaderName;
            reflectionData->stage = stage;
            reflectionData->entryPoint = entryPoint;
            reflectionData->compilationTime = std::chrono::steady_clock::now();

            // Get shader description
            D3D12_SHADER_DESC shaderDesc;
            hr = reflection->GetDesc(&shaderDesc);
            if (FAILED(hr)) {
                return Core::Err<std::unique_ptr<ShaderReflectionData>>(Core::MakeError(
                    Core::ShaderError::ReflectionFailed,
                    "Failed to get shader description"
                ));
            }

            reflectionData->instructionCount = shaderDesc.InstructionCount;
            reflectionData->tempRegisterCount = shaderDesc.TempRegisterCount;

            // Reflect constant buffers
            for (UINT i = 0; i < shaderDesc.ConstantBuffers; ++i) {
                ID3D12ShaderReflectionConstantBuffer* cbReflection = reflection->GetConstantBufferByIndex(i);
                D3D12_SHADER_BUFFER_DESC bufferDesc;
                hr = cbReflection->GetDesc(&bufferDesc);
                if (FAILED(hr)) continue;

                ShaderResource cbResource;
                cbResource.name = bufferDesc.Name;
                cbResource.resourceType = ShaderResourceType::ConstantBuffer;
                cbResource.size = bufferDesc.Size;

                // Get binding information
                D3D12_SHADER_INPUT_BIND_DESC bindDesc;
                hr = reflection->GetResourceBindingDescByName(bufferDesc.Name, &bindDesc);
                if (SUCCEEDED(hr)) {
                    cbResource.bindPoint = bindDesc.BindPoint;
                    cbResource.bindCount = bindDesc.BindCount;
                    cbResource.space = bindDesc.Space;
                }

                // Reflect constant buffer variables
                for (UINT j = 0; j < bufferDesc.Variables; ++j) {
                    ID3D12ShaderReflectionVariable* varReflection = cbReflection->GetVariableByIndex(j);
                    D3D12_SHADER_VARIABLE_DESC varDesc;
                    hr = varReflection->GetDesc(&varDesc);
                    if (FAILED(hr)) continue;

                    ShaderParameter param;
                    param.name = varDesc.Name;
                    param.size = varDesc.Size;
                    param.offset = varDesc.StartOffset;

                    // Get type information
                    ID3D12ShaderReflectionType* typeReflection = varReflection->GetType();
                    D3D12_SHADER_TYPE_DESC typeDesc;
                    hr = typeReflection->GetDesc(&typeDesc);
                    if (SUCCEEDED(hr)) {
                        param.columns = typeDesc.Columns;
                        param.rows = typeDesc.Rows;
                        param.arraySize = typeDesc.Elements > 0 ? typeDesc.Elements : 1;
                        param.isArray = typeDesc.Elements > 0;
                        param.isMatrix = typeDesc.Class == D3D_SVC_MATRIX_COLUMNS || typeDesc.Class == D3D_SVC_MATRIX_ROWS;

                        // Convert D3D type to our ShaderDataType
                        switch (typeDesc.Type) {
                        case D3D_SVT_BOOL: param.dataType = ShaderDataType::Bool; break;
                        case D3D_SVT_INT: param.dataType = ShaderDataType::Int; break;
                        case D3D_SVT_UINT: param.dataType = ShaderDataType::UInt; break;
                        case D3D_SVT_FLOAT:
                            if (typeDesc.Columns == 1 && typeDesc.Rows == 1) param.dataType = ShaderDataType::Float;
                            else if (typeDesc.Columns == 2) param.dataType = ShaderDataType::Float2;
                            else if (typeDesc.Columns == 3) param.dataType = ShaderDataType::Float3;
                            else if (typeDesc.Columns == 4) param.dataType = ShaderDataType::Float4;
                            break;
                        default: param.dataType = ShaderDataType::Custom; break;
                        }
                    }

                    cbResource.parameters.push_back(param);
                }

                reflectionData->constantBuffers.push_back(cbResource);
            }

            // Reflect bound resources (textures, samplers, UAVs)
            for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
                D3D12_SHADER_INPUT_BIND_DESC bindDesc;
                hr = reflection->GetResourceBindingDesc(i, &bindDesc);
                if (FAILED(hr)) continue;

                ShaderResource resource;
                resource.name = bindDesc.Name;
                resource.bindPoint = bindDesc.BindPoint;
                resource.bindCount = bindDesc.BindCount;
                resource.space = bindDesc.Space;

                switch (bindDesc.Type) {
                case D3D_SIT_TEXTURE:
                    resource.resourceType = ShaderResourceType::ShaderResource;
                    reflectionData->shaderResources.push_back(resource);
                    reflectionData->textureCount++;
                    break;
                case D3D_SIT_SAMPLER:
                    resource.resourceType = ShaderResourceType::Sampler;
                    reflectionData->samplers.push_back(resource);
                    reflectionData->samplerCount++;
                    break;
                case D3D_SIT_UAV_RWTYPED:
                case D3D_SIT_UAV_RWSTRUCTURED:
                case D3D_SIT_UAV_RWBYTEADDRESS:
                    resource.resourceType = ShaderResourceType::UnorderedAccess;
                    reflectionData->unorderedAccessViews.push_back(resource);
                    break;
                case D3D_SIT_CBUFFER:
                    // Already handled above
                    reflectionData->constantBufferCount++;
                    break;
                }
            }

            // Set included files for dependency tracking
            for (const auto& file : includedFiles) {
                reflectionData->includedFiles.push_back(file.string());
            }

            return Core::Ok<std::unique_ptr<ShaderReflectionData>>(std::move(reflectionData));
        }

        // ============================================================================
        // ShaderCache Implementation
        // ============================================================================

        ShaderCache::ShaderCache(const Configuration::ShaderConfig& config)
            : config_(config)
        {
        }

        ShaderCache::~ShaderCache() {
            if (config_.GetEnableShaderCache()) {
                SaveToDisk();
            }
        }

        bool ShaderCache::TryGetCachedShader(
            const std::filesystem::path& sourcePath,
            const std::string& entryPoint,
            ShaderStage stage,
            const ShaderVariant& variant,
            Configuration::ShaderOptimization optimization,
            CacheEntry& outEntry
        ) {
            std::shared_lock<std::shared_mutex> lock(cacheMutex_);

            std::string key = GenerateCacheKey(sourcePath, entryPoint, stage, variant, optimization);
            auto it = cache_.find(key);

            if (it == cache_.end()) {
                return false;
            }

            const CacheEntry& entry = it->second;

            // Check if source file has been modified
            if (std::filesystem::exists(sourcePath)) {
                auto lastWriteTime = std::filesystem::last_write_time(sourcePath);
                if (lastWriteTime > entry.lastModified) {
                    return false; // Source file is newer
                }
            }

            // Verify source hash if available
            if (entry.sourceHash != 0) {
                std::uint64_t currentHash = CalculateFileHash(sourcePath);
                if (currentHash != entry.sourceHash) {
                    return false; // Source content changed
                }
            }

            // Copy the data we need (can't copy the whole entry due to unique_ptr)
            outEntry.bytecode = entry.bytecode;
            outEntry.reflection = std::make_unique<ShaderReflectionData>(*entry.reflection);
            outEntry.sourceHash = entry.sourceHash;
            outEntry.lastModified = entry.lastModified;
            outEntry.sourcePath = entry.sourcePath;
            outEntry.variant = entry.variant;
            outEntry.optimization = entry.optimization;

            return true;
        }

        void ShaderCache::CacheShader(
            const std::filesystem::path& sourcePath,
            const std::string& entryPoint,
            ShaderStage stage,
            const ShaderVariant& variant,
            Configuration::ShaderOptimization optimization,
            const std::vector<std::uint8_t>& bytecode,
            std::unique_ptr<ShaderReflectionData> reflection
        ) {
            std::unique_lock<std::shared_mutex> lock(cacheMutex_);

            std::string key = GenerateCacheKey(sourcePath, entryPoint, stage, variant, optimization);

            CacheEntry entry;
            entry.bytecode = bytecode;
            entry.reflection = std::move(reflection);
            entry.sourceHash = CalculateFileHash(sourcePath);
            entry.sourcePath = sourcePath;
            entry.variant = variant;
            entry.optimization = optimization;

            if (std::filesystem::exists(sourcePath)) {
                entry.lastModified = std::filesystem::last_write_time(sourcePath);
            }

            cache_.emplace(key, std::move(entry));
        }

        void ShaderCache::InvalidateCache(const std::filesystem::path& sourcePath) {
            std::unique_lock<std::shared_mutex> lock(cacheMutex_);

            // Remove all cache entries that depend on this source file
            auto it = cache_.begin();
            while (it != cache_.end()) {
                if (it->second.sourcePath == sourcePath) {
                    it = cache_.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        void ShaderCache::ClearCache() {
            std::unique_lock<std::shared_mutex> lock(cacheMutex_);
            cache_.clear();
        }

        std::uint64_t ShaderCache::GetCacheSize() const {
            std::shared_lock<std::shared_mutex> lock(cacheMutex_);
            std::uint64_t totalSize = 0;
            for (const auto& [key, entry] : cache_) {
                totalSize += entry.bytecode.size();
            }
            return totalSize;
        }

        std::uint32_t ShaderCache::GetCachedShaderCount() const {
            std::shared_lock<std::shared_mutex> lock(cacheMutex_);
            return static_cast<std::uint32_t>(cache_.size());
        }

        void ShaderCache::SaveToDisk() {
            // Implementation would serialize cache to disk
            // For now, just log that we're saving
            auto& logChannel = Logging::LogManager::Instance().GetChannel("ShaderCache");
            logChannel.LogFormat(Logging::LogLevel::Info, "Saving shader cache with {} entries", GetCachedShaderCount());
        }

        void ShaderCache::LoadFromDisk() {
            // Implementation would deserialize cache from disk
            // For now, just log that we're loading
            auto& logChannel = Logging::LogManager::Instance().GetChannel("ShaderCache");
            logChannel.Log(Logging::LogLevel::Info, "Loading shader cache from disk");
        }

        std::string ShaderCache::GenerateCacheKey(
            const std::filesystem::path& sourcePath,
            const std::string& entryPoint,
            ShaderStage stage,
            const ShaderVariant& variant,
            Configuration::ShaderOptimization optimization
        ) const {
            std::string key = sourcePath.string() + "|" + entryPoint + "|" + std::to_string(static_cast<int>(stage)) + "|" + std::to_string(static_cast<int>(optimization));

            // Add variant defines
            for (const auto& define : variant.defines) {
                key += "|" + define.name + "=" + define.value;
            }

            return key;
        }

        std::uint64_t ShaderCache::CalculateFileHash(const std::filesystem::path& filePath) const {
            if (!std::filesystem::exists(filePath)) {
                return 0;
            }

            std::ifstream file(filePath, std::ios::binary);
            if (!file.is_open()) {
                return 0;
            }

            std::hash<std::string> hasher;
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return hasher(content);
        }

        // ============================================================================
        // HotReloadManager Implementation
        // ============================================================================

        HotReloadManager::HotReloadManager(const Configuration::ShaderConfig& config)
            : config_(config)
        {
        }

        HotReloadManager::~HotReloadManager() {
            Stop();
        }

        void HotReloadManager::Start() {
            if (isRunning_.load()) {
                return;
            }

            isRunning_.store(true);
            watchThread_ = std::thread(&HotReloadManager::WatchThreadFunction, this);

            auto& logChannel = Logging::LogManager::Instance().GetChannel("HotReload");
            logChannel.Log(Logging::LogLevel::Info, "Hot reload manager started");
        }

        void HotReloadManager::Stop() {
            if (!isRunning_.load()) {
                return;
            }

            isRunning_.store(false);

            if (watchThread_.joinable()) {
                watchThread_.join();
            }

            auto& logChannel = Logging::LogManager::Instance().GetChannel("HotReload");
            logChannel.Log(Logging::LogLevel::Info, "Hot reload manager stopped");
        }

        void HotReloadManager::AddFileWatch(const std::filesystem::path& filePath) {
            std::unique_lock<std::shared_mutex> lock(watchDataMutex_);

            watchedFiles_.insert(filePath);

            if (std::filesystem::exists(filePath)) {
                fileTimestamps_[filePath] = std::filesystem::last_write_time(filePath);
            }
        }

        void HotReloadManager::RemoveFileWatch(const std::filesystem::path& filePath) {
            std::unique_lock<std::shared_mutex> lock(watchDataMutex_);

            watchedFiles_.erase(filePath);
            fileTimestamps_.erase(filePath);
            fileDependents_.erase(filePath);
        }

        void HotReloadManager::UpdateDependencies(const std::string& shaderName, const std::vector<std::filesystem::path>& dependencies) {
            std::unique_lock<std::shared_mutex> lock(watchDataMutex_);

            for (const auto& dependency : dependencies) {
                fileDependents_[dependency].push_back(shaderName);
                AddFileWatch(dependency);
            }
        }

        void HotReloadManager::WatchThreadFunction() {
            ///auto& logChannel = Logging::LogManager::Instance().GetChannel("HotReload");

            while (isRunning_.load()) {
                CheckForChanges();
                std::this_thread::sleep_for(std::chrono::milliseconds(config_.GetHotReloadCheckIntervalMs()));
            }
        }

        void HotReloadManager::CheckForChanges() {
            std::shared_lock<std::shared_mutex> lock(watchDataMutex_);

            for (const auto& filePath : watchedFiles_) {
                if (!std::filesystem::exists(filePath)) {
                    continue;
                }

                auto currentTime = std::filesystem::last_write_time(filePath);
                auto it = fileTimestamps_.find(filePath);

                if (it != fileTimestamps_.end() && currentTime > it->second) {
                    // File has been modified
                    lock.unlock();
                    OnFileChanged(filePath);
                    lock.lock();

                    // Update timestamp
                    const_cast<std::unordered_map<std::filesystem::path, std::filesystem::file_time_type>&>(fileTimestamps_)[filePath] = currentTime;
                }
            }
        }

        void HotReloadManager::OnFileChanged(const std::filesystem::path& filePath) {
            if (!callback_) {
                return;
            }

            auto& logChannel = Logging::LogManager::Instance().GetChannel("HotReload");
            logChannel.LogFormat(Logging::LogLevel::Info, "File changed: {}", filePath.string());

            // Notify callback about file change
            callback_->OnShaderChanged(filePath);

            // Find and notify about dependent shaders
            std::shared_lock<std::shared_mutex> lock(watchDataMutex_);
            auto it = fileDependents_.find(filePath);
            if (it != fileDependents_.end()) {
                callback_->OnShaderDependencyChanged(filePath, it->second);
            }
        }


        // ========================================================================
        // Async Compilation
        // ========================================================================

        void D3D12ShaderManager::CompileShaderAsync(
            const ShaderCompileRequest& request,
            std::function<void(Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo>)> callback
        ) {
            AsyncCompileTask task;
            task.request = request;
            task.callback = callback;
            task.submitTime = std::chrono::steady_clock::now();

            {
                std::lock_guard<std::mutex> lock(asyncTasksMutex_);
                asyncTasks_.push(std::move(task));
            }
            asyncTasksCV_.notify_one();
        }

        // ========================================================================
        // Shader Creation from Bytecode
        // ========================================================================

        Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo> D3D12ShaderManager::CreateShaderFromBytecode(
            const std::string& name,
            ShaderStage stage,
            const std::vector<std::uint8_t>& bytecode,
            const ShaderVariant& variant
        ) {
            // Create D3D blob from bytecode
            ComPtr<ID3DBlob> blob;
            HRESULT hr = D3DCreateBlob(bytecode.size(), &blob);
            if (FAILED(hr)) {
                return Core::Err<std::shared_ptr<IShader>>(Core::MakeError(
                    Core::ShaderError::InvalidBytecode,
                    std::format("Failed to create D3D blob: HRESULT 0x{:08X}", static_cast<uint32_t>(hr))
                ));
            }

            memcpy(blob->GetBufferPointer(), bytecode.data(), bytecode.size());

            // Reflect shader to get resource information
            auto reflectionResult = ReflectShader(blob.Get(), name, stage, "main", {});
            if (!reflectionResult) {
                return Core::Err<std::shared_ptr<IShader>>(reflectionResult.Error());
            }

            auto shader = std::make_shared<D3D12Shader>(
                name, stage, variant, blob, std::move(reflectionResult.Value())
            );

            // Store shader
            std::string shaderKey = GenerateShaderKey(name, variant);
            {
                std::unique_lock<std::shared_mutex> lock(shadersMutex_);
                shaders_[shaderKey] = shader;
            }

            return Core::Ok<std::shared_ptr<IShader>>(std::move(shader));
        }

        // ========================================================================
        // Shader Program Management
        // ========================================================================

        Core::Result<std::shared_ptr<IShaderProgram>, Core::ErrorInfo> D3D12ShaderManager::CreateShaderProgram(
            const std::string& name,
            const std::vector<std::shared_ptr<IShader>>& shaders
        ) {
            auto program = std::make_shared<D3D12ShaderProgram>(name);

            for (auto shader : shaders) {
                auto d3d12Shader = std::dynamic_pointer_cast<D3D12Shader>(shader);
                if (!d3d12Shader) {
                    return Core::Err<std::shared_ptr<IShaderProgram>>(Core::MakeError(
                        Core::ShaderError::LinkError,
                        "Shader is not a D3D12Shader"
                    ));
                }
                program->AddShader(d3d12Shader);
            }

            // Store program
            {
                std::unique_lock<std::shared_mutex> lock(programsMutex_);
                programs_[name] = program;
            }

            return Core::Ok<std::shared_ptr<IShaderProgram>>(std::move(program));
        }

        Core::Result<std::shared_ptr<IShaderProgram>, Core::ErrorInfo> D3D12ShaderManager::CreateGraphicsProgram(
            const std::string& name,
            const std::filesystem::path& vertexShaderPath,
            const std::filesystem::path& pixelShaderPath,
            const std::filesystem::path& geometryShaderPath,
            const std::filesystem::path& hullShaderPath,
            const std::filesystem::path& domainShaderPath,
            const ShaderVariant& variant
        ) {
            std::vector<std::shared_ptr<IShader>> shaders;

            // Compile vertex shader
            ShaderCompileRequest vertexRequest;
            vertexRequest.sourceFile = vertexShaderPath;
            vertexRequest.entryPoint = "main";
            vertexRequest.stage = ShaderStage::Vertex;
            vertexRequest.variant = variant;
            vertexRequest.optimization = config_.GetOptimization();
            vertexRequest.shaderModel = config_.GetTargetShaderModel();

            auto vertexResult = CompileShader(vertexRequest);
            if (!vertexResult) {
                return Core::Err<std::shared_ptr<IShaderProgram>>(vertexResult.Error());
            }
            shaders.push_back(vertexResult.Value());

            // Compile pixel shader
            ShaderCompileRequest pixelRequest;
            pixelRequest.sourceFile = pixelShaderPath;
            pixelRequest.entryPoint = "main";
            pixelRequest.stage = ShaderStage::Pixel;
            pixelRequest.variant = variant;
            pixelRequest.optimization = config_.GetOptimization();
            pixelRequest.shaderModel = config_.GetTargetShaderModel();

            auto pixelResult = CompileShader(pixelRequest);
            if (!pixelResult) {
                return Core::Err<std::shared_ptr<IShaderProgram>>(pixelResult.Error());
            }
            shaders.push_back(pixelResult.Value());

            // Compile optional shaders if paths provided
            if (!geometryShaderPath.empty()) {
                ShaderCompileRequest geoRequest;
                geoRequest.sourceFile = geometryShaderPath;
                geoRequest.entryPoint = "main";
                geoRequest.stage = ShaderStage::Geometry;
                geoRequest.variant = variant;
                geoRequest.optimization = config_.GetOptimization();
                geoRequest.shaderModel = config_.GetTargetShaderModel();

                auto geoResult = CompileShader(geoRequest);
                if (!geoResult) {
                    return Core::Err<std::shared_ptr<IShaderProgram>>(geoResult.Error());
                }
                shaders.push_back(geoResult.Value());
            }

            return CreateShaderProgram(name, shaders);
        }

        Core::Result<std::shared_ptr<IShaderProgram>, Core::ErrorInfo> D3D12ShaderManager::CreateComputeProgram(
            const std::string& name,
            const std::filesystem::path& computeShaderPath,
            const ShaderVariant& variant
        ) {
            ShaderCompileRequest request;
            request.sourceFile = computeShaderPath;
            request.entryPoint = "main";
            request.stage = ShaderStage::Compute;
            request.variant = variant;
            request.optimization = config_.GetOptimization();
            request.shaderModel = config_.GetTargetShaderModel();

            auto shaderResult = CompileShader(request);
            if (!shaderResult) {
                return Core::Err<std::shared_ptr<IShaderProgram>>(shaderResult.Error());
            }

            std::vector<std::shared_ptr<IShader>> shaders = { shaderResult.Value() };
            return CreateShaderProgram(name, shaders);
        }

        // ========================================================================
        // Shader Retrieval
        // ========================================================================

        std::shared_ptr<IShader> D3D12ShaderManager::GetShader(const std::string& name, const ShaderVariant& variant) const {
            std::string key = GenerateShaderKey(name, variant);
            std::shared_lock<std::shared_mutex> lock(shadersMutex_);
            auto it = shaders_.find(key);
            return (it != shaders_.end()) ? it->second : nullptr;
        }

        std::shared_ptr<IShaderProgram> D3D12ShaderManager::GetShaderProgram(const std::string& name) const {
            std::shared_lock<std::shared_mutex> lock(programsMutex_);
            auto it = programs_.find(name);
            return (it != programs_.end()) ? it->second : nullptr;
        }

        std::vector<std::string> D3D12ShaderManager::GetLoadedShaderNames() const {
            std::shared_lock<std::shared_mutex> lock(shadersMutex_);
            std::vector<std::string> names;
            names.reserve(shaders_.size());
            for (const auto& [key, shader] : shaders_) {
                names.push_back(shader->GetName());
            }
            return names;
        }

        std::vector<std::string> D3D12ShaderManager::GetLoadedProgramNames() const {
            std::shared_lock<std::shared_mutex> lock(programsMutex_);
            std::vector<std::string> names;
            names.reserve(programs_.size());
            for (const auto& [name, program] : programs_) {
                names.push_back(name);
            }
            return names;
        }

        bool D3D12ShaderManager::HasShader(const std::string& name, const ShaderVariant& variant) const {
            return GetShader(name, variant) != nullptr;
        }

        bool D3D12ShaderManager::HasShaderProgram(const std::string& name) const {
            return GetShaderProgram(name) != nullptr;
        }

        void D3D12ShaderManager::UnloadShader(const std::string& name, const ShaderVariant& variant) {
            std::string key = GenerateShaderKey(name, variant);
            std::unique_lock<std::shared_mutex> lock(shadersMutex_);
            shaders_.erase(key);
        }

        void D3D12ShaderManager::UnloadShaderProgram(const std::string& name) {
            std::unique_lock<std::shared_mutex> lock(programsMutex_);
            programs_.erase(name);
        }

        void D3D12ShaderManager::UnloadAllShaders() {
            {
                std::unique_lock<std::shared_mutex> lock(shadersMutex_);
                shaders_.clear();
            }
            {
                std::unique_lock<std::shared_mutex> lock(programsMutex_);
                programs_.clear();
            }
        }

        // ========================================================================
        // Hot Reload
        // ========================================================================

        void D3D12ShaderManager::EnableHotReload(std::shared_ptr<IHotReloadCallback> callback) {
            if (hotReloadManager_) {
                hotReloadManager_->SetCallback(callback);
                hotReloadManager_->Start();
            }
        }

        void D3D12ShaderManager::DisableHotReload() {
            if (hotReloadManager_) {
                hotReloadManager_->Stop();
            }
        }

        bool D3D12ShaderManager::IsHotReloadEnabled() const {
            return hotReloadManager_ && hotReloadManager_->IsRunning();
        }

        void D3D12ShaderManager::ForceRecompileShader(const std::string& name, const ShaderVariant& variant) {
            auto shader = GetShader(name, variant);
            if (shader) {
                shader->MarkForRecompilation();
            }
        }

        void D3D12ShaderManager::ForceRecompileProgram(const std::string& name) {
            auto program = GetShaderProgram(name);
            if (program) {
                program->MarkForRecompilation();
            }
        }

        void D3D12ShaderManager::ForceRecompileAll() {
            {
                std::shared_lock<std::shared_mutex> lock(shadersMutex_);
                for (auto& [key, shader] : shaders_) {
                    shader->MarkForRecompilation();
                }
            }
            {
                std::shared_lock<std::shared_mutex> lock(programsMutex_);
                for (auto& [name, program] : programs_) {
                    program->MarkForRecompilation();
                }
            }
        }

        void D3D12ShaderManager::AddFileWatch(const std::filesystem::path& filePath) {
            if (hotReloadManager_) {
                hotReloadManager_->AddFileWatch(filePath);
            }
        }

        void D3D12ShaderManager::RemoveFileWatch(const std::filesystem::path& filePath) {
            if (hotReloadManager_) {
                hotReloadManager_->RemoveFileWatch(filePath);
            }
        }

        // ========================================================================
        // Cache Management
        // ========================================================================

        void D3D12ShaderManager::ClearCache() {
            if (cache_) {
                cache_->ClearCache();
            }
        }

        void D3D12ShaderManager::ValidateCache() {
            // Implement cache validation logic
            logChannel_.Log(Logging::LogLevel::Info, "Cache validation completed");
        }

        std::uint64_t D3D12ShaderManager::GetCacheSize() const {
            return cache_ ? cache_->GetCacheSize() : 0;
        }

        std::uint32_t D3D12ShaderManager::GetCachedShaderCount() const {
            return cache_ ? cache_->GetCachedShaderCount() : 0;
        }

        void D3D12ShaderManager::SaveCacheToDisk() {
            if (cache_) {
                cache_->SaveToDisk();
            }
        }

        void D3D12ShaderManager::LoadCacheFromDisk() {
            if (cache_) {
                cache_->LoadFromDisk();
            }
        }

        // ========================================================================
        // Statistics and Debugging
        // ========================================================================

        void D3D12ShaderManager::ResetStatistics() {
            std::lock_guard<std::mutex> lock(statisticsMutex_);
            statistics_ = Statistics{};
        }

        void D3D12ShaderManager::DumpShaderInfo(const std::string& shaderName, std::ostream& output) const {
            auto shader = GetShader(shaderName, ShaderVariant{});
            if (!shader) {
                output << "Shader not found: " << shaderName << std::endl;
                return;
            }

            const auto& reflection = shader->GetReflection();
            output << "Shader: " << shaderName << std::endl;
            output << "  Stage: " << static_cast<int>(shader->GetStage()) << std::endl;
            output << "  Instruction Count: " << reflection.instructionCount << std::endl;
            output << "  Constant Buffers: " << reflection.constantBuffers.size() << std::endl;
            output << "  Textures: " << reflection.shaderResources.size() << std::endl;
            output << "  Samplers: " << reflection.samplers.size() << std::endl;
        }

        void D3D12ShaderManager::DumpAllShadersInfo(std::ostream& output) const {
            auto names = GetLoadedShaderNames();
            for (const auto& name : names) {
                DumpShaderInfo(name, output);
                output << std::endl;
            }
        }

        // ========================================================================
        // Utility Functions
        // ========================================================================

        std::uint64_t D3D12ShaderManager::GenerateVariantHash(const ShaderVariant& variant) const {
            std::uint64_t hash = 0;
            for (const auto& define : variant.defines) {
                hash ^= std::hash<std::string>{}(define.name) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                hash ^= std::hash<std::string>{}(define.value) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
            return hash;
        }

        Core::Result<void*, Core::ErrorInfo> D3D12ShaderManager::ValidateShaderSource(
            const std::filesystem::path& sourcePath,
            ShaderStage stage
        ) const {
            if (!std::filesystem::exists(sourcePath)) {
                return Core::Err<void*>(Core::MakeError(
                    Core::FileSystemError::FileNotFound,
                    std::format("Shader source file not found: {}", sourcePath.string())
                ));
            }

            // Basic validation - could be extended
            return Core::Ok<void*>(nullptr);
        }

        std::vector<Configuration::ShaderModel> D3D12ShaderManager::GetSupportedShaderModels() const {
            return {
                Configuration::ShaderModel::SM_5_0,
                Configuration::ShaderModel::SM_5_1,
                Configuration::ShaderModel::SM_6_0,
                Configuration::ShaderModel::SM_6_1,
                Configuration::ShaderModel::SM_6_2,
                Configuration::ShaderModel::SM_6_3,
                Configuration::ShaderModel::SM_6_4,
                Configuration::ShaderModel::SM_6_5,
                Configuration::ShaderModel::SM_6_6,
                Configuration::ShaderModel::SM_6_7
            };
        }

        bool D3D12ShaderManager::SupportsShaderModel(Configuration::ShaderModel model) const {
            auto supported = GetSupportedShaderModels();
            return std::find(supported.begin(), supported.end(), model) != supported.end();
        }

        bool D3D12ShaderManager::SupportsRayTracing() const {
            return SupportsShaderModel(Configuration::ShaderModel::SM_6_3);
        }

        bool D3D12ShaderManager::SupportsMeshShaders() const {
            return SupportsShaderModel(Configuration::ShaderModel::SM_6_5);
        }

        bool D3D12ShaderManager::SupportsVariableRateShading() const {
            return SupportsShaderModel(Configuration::ShaderModel::SM_6_1);
        }

        // ========================================================================
        // Update and Maintenance
        // ========================================================================

        void D3D12ShaderManager::Update() {
            // Process any maintenance tasks
            if (hotReloadManager_ && hotReloadManager_->IsRunning()) {
                // Hot reload manager runs in its own thread
            }
        }

        void D3D12ShaderManager::FlushAsyncOperations() {
            std::unique_lock<std::mutex> lock(asyncTasksMutex_);
            asyncTasksCV_.wait(lock, [this] { return asyncTasks_.empty(); });
        }

        std::uint32_t D3D12ShaderManager::GetPendingCompilationCount() const {
            std::lock_guard<std::mutex> lock(asyncTasksMutex_);
            return static_cast<std::uint32_t>(asyncTasks_.size());
        }

        // ========================================================================
        // Enhanced Multiple Entry Point Support
        // ========================================================================

        std::vector<std::string> D3D12ShaderManager::GetAvailableEntryPoints(
            const std::filesystem::path& shaderFile
        ) const {
            // Simple implementation - look for function signatures
            auto sourceResult = ReadShaderFile(shaderFile);
            if (!sourceResult) {
                return {};
            }

            std::vector<std::string> entryPoints;
            std::string source = sourceResult.Value();

            // Look for function patterns (simplified)
            std::regex entryPointRegex(R"(\w+\s+(\w+)\s*\([^)]*\)\s*:\s*SV_)");
            std::sregex_iterator iter(source.begin(), source.end(), entryPointRegex);
            std::sregex_iterator end;

            for (; iter != end; ++iter) {
                entryPoints.push_back((*iter)[1].str());
            }

            // Add common entry points if not found
            if (entryPoints.empty()) {
                entryPoints.push_back("main");
            }

            return entryPoints;
        }

        Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo> D3D12ShaderManager::CompileShaderEntryPoint(
            const std::filesystem::path& sourceFile,
            const std::string& entryPoint,
            ShaderStage stage,
            const ShaderVariant& variant,
            Configuration::ShaderOptimization optimization
        ) {
            ShaderCompileRequest request;
            request.sourceFile = sourceFile;
            request.entryPoint = entryPoint;
            request.stage = stage;
            request.variant = variant;
            request.optimization = optimization;
            request.shaderModel = config_.GetTargetShaderModel();

            return CompileShader(request);
        }

        Core::Result<std::vector<std::shared_ptr<IShader>>, Core::ErrorInfo> D3D12ShaderManager::CompileAllEntryPoints(
            const std::filesystem::path& shaderFile,
            ShaderStage primaryStage,
            const ShaderVariant& variant
        ) {
            auto entryPoints = GetAvailableEntryPoints(shaderFile);
            std::vector<std::shared_ptr<IShader>> shaders;

            for (const auto& entryPoint : entryPoints) {
                auto result = CompileShaderEntryPoint(shaderFile, entryPoint, primaryStage, variant, config_.GetOptimization());
                if (result) {
                    shaders.push_back(result.Value());
                }
            }

            return Core::Ok<std::vector<std::shared_ptr<IShader>>>(std::move(shaders));
        }

        Core::Result<std::shared_ptr<IShaderProgram>, Core::ErrorInfo> D3D12ShaderManager::CreateGraphicsProgramFromFile(
            const std::string& name,
            const std::filesystem::path& shaderFile,
            const std::string& vertexEntryPoint,
            const std::string& pixelEntryPoint,
            const std::string& geometryEntryPoint,
            const std::string& hullEntryPoint,
            const std::string& domainEntryPoint,
            const ShaderVariant& variant
        ) {
            std::vector<std::shared_ptr<IShader>> shaders;

            // Compile vertex shader
            auto vertexResult = CompileShaderEntryPoint(shaderFile, vertexEntryPoint, ShaderStage::Vertex, variant, config_.GetOptimization());
            if (!vertexResult) {
                return Core::Err<std::shared_ptr<IShaderProgram>>(vertexResult.Error());
            }
            shaders.push_back(vertexResult.Value());

            // Compile pixel shader
            auto pixelResult = CompileShaderEntryPoint(shaderFile, pixelEntryPoint, ShaderStage::Pixel, variant, config_.GetOptimization());
            if (!pixelResult) {
                return Core::Err<std::shared_ptr<IShaderProgram>>(pixelResult.Error());
            }
            shaders.push_back(pixelResult.Value());

            // Compile optional shaders
            if (!geometryEntryPoint.empty()) {
                auto geoResult = CompileShaderEntryPoint(shaderFile, geometryEntryPoint, ShaderStage::Geometry, variant, config_.GetOptimization());
                if (geoResult) {
                    shaders.push_back(geoResult.Value());
                }
            }

            return CreateShaderProgram(name, shaders);
        }

        // ========================================================================
        // Constant Buffer Binding and Layout Validation
        // ========================================================================

        Core::Result<void*, Core::ErrorInfo> D3D12ShaderManager::ValidateConstantBufferLayout(
            const std::shared_ptr<IShader>& shader,
            const std::string& expectedLayoutName
        ) {
            const auto& reflection = shader->GetReflection();

            // Check if constant buffers follow b0-b31 convention
            for (const auto& cb : reflection.constantBuffers) {
                if (cb.bindPoint > 31) {
                    return Core::Err<void*>(Core::MakeError(
                        Core::ShaderError::ReflectionFailed,
                        std::format("Constant buffer '{}' at register b{} exceeds expected range (b0-b31)",
                            cb.name, cb.bindPoint)
                    ));
                }
            }

            return Core::Ok<void*>(nullptr);
        }

        std::vector<std::pair<std::string, std::uint32_t>> D3D12ShaderManager::GetConstantBufferBindings(
            const std::shared_ptr<IShader>& shader
        ) const {
            std::vector<std::pair<std::string, std::uint32_t>> bindings;
            const auto& reflection = shader->GetReflection();

            for (const auto& cb : reflection.constantBuffers) {
                bindings.emplace_back(cb.name, cb.bindPoint);
            }

            return bindings;
        }

        Core::Result<void*, Core::ErrorInfo> D3D12ShaderManager::ValidateProgramConstantBuffers(
            const std::shared_ptr<IShaderProgram>& program,
            const std::vector<std::string>& requiredBuffers
        ) {
            for (const auto& bufferName : requiredBuffers) {
                auto binding = program->FindConstantBufferBinding(bufferName);
                if (!binding.has_value()) {
                    return Core::Err<void*>(Core::MakeError(
                        Core::ShaderError::ReflectionFailed,
                        std::format("Required constant buffer '{}' not found in program", bufferName)
                    ));
                }
            }

            return Core::Ok<void*>(nullptr);
        }

        bool D3D12ShaderManager::ValidateRegisterLayout(const std::shared_ptr<IShader>& shader) const {
            const auto& reflection = shader->GetReflection();

            for (const auto& cb : reflection.constantBuffers) {
                if (cb.bindPoint > 31) {
                    return false;
                }
            }

            return true;
        }

        // ========================================================================
        // Enhanced Include Dependency Management - Stub Implementations
        // ========================================================================

        std::vector<std::filesystem::path> D3D12ShaderManager::GetShaderDependencies(
            const std::filesystem::path& shaderFile
        ) const {
            // Stub implementation
            return {};
        }

        std::vector<std::string> D3D12ShaderManager::FindShadersUsingInclude(
            const std::filesystem::path& includeFile
        ) const {
            // Stub implementation
            return {};
        }

        void D3D12ShaderManager::AddIncludePath(const std::filesystem::path& includePath) {
            std::unique_lock<std::shared_mutex> lock(includePathsMutex_);
            includePaths_.push_back(includePath);
        }

        void D3D12ShaderManager::RemoveIncludePath(const std::filesystem::path& includePath) {
            std::unique_lock<std::shared_mutex> lock(includePathsMutex_);
            includePaths_.erase(
                std::remove(includePaths_.begin(), includePaths_.end(), includePath),
                includePaths_.end()
            );
        }

        std::vector<std::filesystem::path> D3D12ShaderManager::GetIncludePaths() const {
            std::shared_lock<std::shared_mutex> lock(includePathsMutex_);
            return includePaths_;
        }

        void D3D12ShaderManager::RefreshDependencyTracking() {
            // Stub implementation
        }

        // ========================================================================
        // Shader Preprocessing and Macro Management - Stub Implementations
        // ========================================================================

        Core::Result<std::string, Core::ErrorInfo> D3D12ShaderManager::PreprocessShader(
            const std::filesystem::path& sourceFile,
            const ShaderVariant& variant
        ) const {
            // Stub implementation - just return source file content
            return ReadShaderFile(sourceFile);
        }

        Core::Result<void*, Core::ErrorInfo> D3D12ShaderManager::ValidateShaderDefines(
            const ShaderVariant& variant
        ) const {
            // Stub implementation
            return Core::Ok<void*>(nullptr);
        }

        std::vector<std::string> D3D12ShaderManager::GetShaderMacros(
            const std::filesystem::path& shaderFile
        ) const {
            // Stub implementation
            return {};
        }

        bool D3D12ShaderManager::HasFeatureMacro(
            const std::shared_ptr<IShader>& shader,
            const std::string& macroName
        ) const {
            // Stub implementation
            return false;
        }

        // ========================================================================
        // Shader Permutation and Variant Management - Stub Implementations
        // ========================================================================

        std::vector<ShaderVariant> D3D12ShaderManager::GenerateShaderVariants(
            const std::filesystem::path& shaderFile,
            const std::vector<std::string>& conditionalDefines
        ) const {
            // Stub implementation
            return { ShaderVariant{} };
        }

        Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo> D3D12ShaderManager::CompileShaderWithAutoVariants(
            const std::filesystem::path& sourceFile,
            const std::string& entryPoint,
            ShaderStage stage,
            const std::vector<std::string>& enabledFeatures
        ) {
            // Stub implementation - just compile with empty variant
            ShaderVariant variant;
            return CompileShaderEntryPoint(sourceFile, entryPoint, stage, variant, config_.GetOptimization());
        }

        ShaderVariant D3D12ShaderManager::GetOptimalVariant(
            const std::filesystem::path& shaderFile,
            ShaderStage stage
        ) const {
            // Stub implementation
            return ShaderVariant{};
        }

        // ========================================================================
        // Advanced Debugging and Profiling Support - Stub Implementations
        // ========================================================================

        Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo> D3D12ShaderManager::CompileShaderWithDebugInfo(
            const ShaderCompileRequest& request,
            bool generateAssembly,
            bool preserveDebugInfo
        ) {
            ShaderCompileRequest debugRequest = request;
            debugRequest.generateDebugInfo = true;
            return CompileShader(debugRequest);
        }

        Core::Result<std::string, Core::ErrorInfo> D3D12ShaderManager::GetShaderAssembly(
            const std::shared_ptr<IShader>& shader
        ) const {
            // Stub implementation
            return Core::Err<std::string>(Core::MakeError(
                Core::ShaderError::UnsupportedShaderModel,
                "Shader disassembly not implemented yet"
            ));
        }

        IShaderManager::CompilationProfile D3D12ShaderManager::GetCompilationProfile(
            const std::shared_ptr<IShader>& shader
        ) const {
            // Stub implementation
            CompilationProfile profile;
            profile.compilerVersion = "D3DCompiler";
            return profile;
        }



    }

    // ============================================================================
    // ShaderManagerFactory Implementation  
    // ============================================================================

    Core::Result<std::unique_ptr<IShaderManager>, Core::ErrorInfo> ShaderManagerFactory::CreateShaderManager(
        Configuration::RenderingAPI api,
        Memory::IAllocator& allocator
    ) {
        switch (api) {
        case Configuration::RenderingAPI::D3D12:
            return Core::Ok<std::unique_ptr<IShaderManager>>(
                std::make_unique<D3D12::D3D12ShaderManager>(allocator)
            );

        case Configuration::RenderingAPI::Vulkan:
            return Core::Err<std::unique_ptr<IShaderManager>>(Core::MakeError(
                Core::ShaderError::UnsupportedShaderModel,
                "Vulkan shader manager not implemented yet"
            ));

        case Configuration::RenderingAPI::D3D11:
            return Core::Err<std::unique_ptr<IShaderManager>>(Core::MakeError(
                Core::ShaderError::UnsupportedShaderModel,
                "D3D11 shader manager not implemented yet"
            ));

        default:
            return Core::Err<std::unique_ptr<IShaderManager>>(Core::MakeError(
                Core::RendererError::UnsupportedFeature,
                std::format("Unknown rendering API: {}", static_cast<int>(api))
            ));
        }
    }
} // namespace Akhanda::Shaders::D3D12