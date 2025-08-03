// Engine/Renderer/Shaders/D3D12/D3D12ShaderManager.ixx
// Akhanda Game Engine - D3D12 Shader Manager Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include <d3d12.h>
#include <d3d12shader.h>
#include <wrl.h>

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <chrono>

export module Akhanda.Engine.Shaders.D3D12;

import Akhanda.Engine.Shaders;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Core.Result;
import Akhanda.Core.Logging;
import Akhanda.Core.Memory;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Core.Threading;

using Microsoft::WRL::ComPtr;

export namespace Akhanda::Shaders::D3D12 {

    // ============================================================================
    // Forward Declarations
    // ============================================================================

    class D3D12Shader;
    class D3D12ShaderProgram;
    class ShaderCache;
    class HotReloadManager;
    class CompilerIncludeHandler;

    // ============================================================================
    // D3D12 Shader Implementation
    // ============================================================================

    class D3D12Shader : public IShader {
    public:
        D3D12Shader(
            const std::string& name,
            ShaderStage stage,
            const ShaderVariant& variant,
            ComPtr<ID3DBlob> bytecode,
            std::unique_ptr<ShaderReflectionData> reflection
        );

        virtual ~D3D12Shader() = default;

        // IShader interface implementation
        const std::string& GetName() const override { return name_; }
        ShaderStage GetStage() const override { return stage_; }
        const ShaderVariant& GetVariant() const override { return variant_; }
        const ShaderReflectionData& GetReflection() const override { return *reflection_; }

        bool IsCompiled() const override { return bytecode_ != nullptr; }
        const std::vector<std::uint8_t>& GetBytecode() const override;
        std::uint64_t GetSourceHash() const override { return reflection_->sourceCodeHash; }
        std::chrono::steady_clock::time_point GetCompilationTime() const override { return reflection_->compilationTime; }

        bool NeedsRecompilation() const override { return needsRecompilation_.load(); }
        void MarkForRecompilation() override { needsRecompilation_.store(true); }

        std::optional<std::uint32_t> GetConstantBufferBinding(const std::string& name) const override;
        std::optional<std::uint32_t> GetTextureBinding(const std::string& name) const override;
        std::optional<std::uint32_t> GetSamplerBinding(const std::string& name) const override;
        std::optional<std::uint32_t> GetUAVBinding(const std::string& name) const override;

        // D3D12-specific methods
        ID3DBlob* GetD3DBlob() const { return bytecode_.Get(); }
        void UpdateReflectionData(std::unique_ptr<ShaderReflectionData> newReflection);

    private:
        std::string name_;
        ShaderStage stage_;
        ShaderVariant variant_;
        ComPtr<ID3DBlob> bytecode_;
        std::unique_ptr<ShaderReflectionData> reflection_;
        mutable std::vector<std::uint8_t> bytecodeVector_; // Cached vector representation
        std::atomic<bool> needsRecompilation_{ false };
        mutable std::mutex bytecodeVectorMutex_;
    };

    // ============================================================================
    // D3D12 Shader Program Implementation
    // ============================================================================

    class D3D12ShaderProgram : public IShaderProgram {
    public:
        D3D12ShaderProgram(const std::string& name);
        virtual ~D3D12ShaderProgram() = default;

        // IShaderProgram interface implementation
        const std::string& GetName() const override { return name_; }
        bool IsLinked() const override;
        std::uint64_t GetProgramHash() const override;

        std::shared_ptr<IShader> GetShader(ShaderStage stage) const override;
        const std::vector<ShaderStage>& GetActiveStages() const override { return activeStages_; }

        const ShaderReflectionData& GetCombinedReflection() const override;

        std::optional<std::uint32_t> FindConstantBufferBinding(const std::string& name) const override;
        std::optional<std::uint32_t> FindTextureBinding(const std::string& name) const override;
        std::optional<std::uint32_t> FindSamplerBinding(const std::string& name) const override;
        std::optional<std::uint32_t> FindUAVBinding(const std::string& name) const override;

        bool NeedsRecompilation() const override;
        void MarkForRecompilation() override;

        // D3D12ShaderProgram specific methods
        void AddShader(std::shared_ptr<D3D12Shader> shader);
        void RemoveShader(ShaderStage stage);
        void BuildCombinedReflection();

    private:
        std::string name_;
        std::unordered_map<ShaderStage, std::shared_ptr<D3D12Shader>> shaders_;
        std::vector<ShaderStage> activeStages_;
        std::unique_ptr<ShaderReflectionData> combinedReflection_;
        std::uint64_t programHash_{ 0 };
        mutable std::shared_mutex shadersMutex_;

        void UpdateActiveStages();
        void CalculateProgramHash();
    };

    // ============================================================================
    // Shader Cache System
    // ============================================================================

    class ShaderCache {
    public:
        ShaderCache(const Configuration::ShaderConfig& config);
        ~ShaderCache();

        struct CacheEntry {
            std::vector<std::uint8_t> bytecode;
            std::unique_ptr<ShaderReflectionData> reflection;
            std::uint64_t sourceHash;
            std::filesystem::file_time_type lastModified;
            std::filesystem::path sourcePath;
            ShaderVariant variant;
            Configuration::ShaderOptimization optimization;

            // Add move constructor and move assignment operator
            CacheEntry() = default;
            CacheEntry(const CacheEntry&) = delete;
            CacheEntry& operator=(const CacheEntry&) = delete;
            CacheEntry(CacheEntry&&) = default;
            CacheEntry& operator=(CacheEntry&&) = default;
        };

        bool TryGetCachedShader(
            const std::filesystem::path& sourcePath,
            const std::string& entryPoint,
            ShaderStage stage,
            const ShaderVariant& variant,
            Configuration::ShaderOptimization optimization,
            CacheEntry& outEntry
        );

        void CacheShader(
            const std::filesystem::path& sourcePath,
            const std::string& entryPoint,
            ShaderStage stage,
            const ShaderVariant& variant,
            Configuration::ShaderOptimization optimization,
            const std::vector<std::uint8_t>& bytecode,
            std::unique_ptr<ShaderReflectionData> reflection
        );

        void InvalidateCache(const std::filesystem::path& sourcePath);
        void ClearCache();
        void SaveToDisk();
        void LoadFromDisk();

        std::uint64_t GetCacheSize() const;
        std::uint32_t GetCachedShaderCount() const;

    private:
        Configuration::ShaderConfig config_;
        std::unordered_map<std::string, CacheEntry> cache_;
        mutable std::shared_mutex cacheMutex_;

        std::string GenerateCacheKey(
            const std::filesystem::path& sourcePath,
            const std::string& entryPoint,
            ShaderStage stage,
            const ShaderVariant& variant,
            Configuration::ShaderOptimization optimization
        ) const;

        std::uint64_t CalculateFileHash(const std::filesystem::path& filePath) const;
    };

    // ============================================================================
    // Hot Reload Manager
    // ============================================================================

    class HotReloadManager {
    public:
        HotReloadManager(const Configuration::ShaderConfig& config);
        ~HotReloadManager();

        void Start();
        void Stop();
        bool IsRunning() const { return isRunning_.load(); }

        void SetCallback(std::shared_ptr<IHotReloadCallback> callback) { callback_ = callback; }

        void AddFileWatch(const std::filesystem::path& filePath);
        void RemoveFileWatch(const std::filesystem::path& filePath);

        void UpdateDependencies(const std::string& shaderName, const std::vector<std::filesystem::path>& dependencies);

    private:
        Configuration::ShaderConfig config_;
        std::shared_ptr<IHotReloadCallback> callback_;
        std::atomic<bool> isRunning_{ false };
        std::thread watchThread_;

        std::unordered_set<std::filesystem::path> watchedFiles_;
        std::unordered_map<std::filesystem::path, std::filesystem::file_time_type> fileTimestamps_;
        std::unordered_map<std::filesystem::path, std::vector<std::string>> fileDependents_; // file -> shaders that depend on it
        mutable std::shared_mutex watchDataMutex_;

        void WatchThreadFunction();
        void CheckForChanges();
        void OnFileChanged(const std::filesystem::path& filePath);
    };

    // ============================================================================
    // Main D3D12 Shader Manager Implementation
    // ============================================================================

    class D3D12ShaderManager : public IShaderManager {
    public:
        D3D12ShaderManager(Memory::IAllocator& allocator);
        virtual ~D3D12ShaderManager();

        // ========================================================================
        // Initialization and Configuration
        // ========================================================================

        Core::Result<void*, Core::ErrorInfo> Initialize(
            std::shared_ptr<RHI::IRenderDevice> renderDevice,
            const Configuration::ShaderConfig& config
        ) override;

        void Shutdown() override;
        bool IsInitialized() const override { return isInitialized_.load(); }

        void UpdateConfiguration(const Configuration::ShaderConfig& config) override;
        const Configuration::ShaderConfig& GetConfiguration() const override { return config_; }

        // ========================================================================
        // Shader Compilation
        // ========================================================================

        Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo> CompileShader(
            const ShaderCompileRequest& request
        ) override;

        void CompileShaderAsync(
            const ShaderCompileRequest& request,
            std::function<void(Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo>)> callback
        ) override;

        Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo> CreateShaderFromBytecode(
            const std::string& name,
            ShaderStage stage,
            const std::vector<std::uint8_t>& bytecode,
            const ShaderVariant& variant
        ) override;

        // ========================================================================
        // Shader Program Management
        // ========================================================================

        Core::Result<std::shared_ptr<IShaderProgram>, Core::ErrorInfo> CreateShaderProgram(
            const std::string& name,
            const std::vector<std::shared_ptr<IShader>>& shaders
        ) override;

        Core::Result<std::shared_ptr<IShaderProgram>, Core::ErrorInfo> CreateGraphicsProgram(
            const std::string& name,
            const std::filesystem::path& vertexShaderPath,
            const std::filesystem::path& pixelShaderPath,
            const std::filesystem::path& geometryShaderPath,
            const std::filesystem::path& hullShaderPath,
            const std::filesystem::path& domainShaderPath,
            const ShaderVariant& variant
        ) override;

        Core::Result<std::shared_ptr<IShaderProgram>, Core::ErrorInfo> CreateComputeProgram(
            const std::string& name,
            const std::filesystem::path& computeShaderPath,
            const ShaderVariant& variant
        ) override;

        // ========================================================================
        // Shader Retrieval and Management
        // ========================================================================

        std::shared_ptr<IShader> GetShader(const std::string& name, const ShaderVariant& variant) const override;
        std::shared_ptr<IShaderProgram> GetShaderProgram(const std::string& name) const override;

        std::vector<std::string> GetLoadedShaderNames() const override;
        std::vector<std::string> GetLoadedProgramNames() const override;

        bool HasShader(const std::string& name, const ShaderVariant& variant) const override;
        bool HasShaderProgram(const std::string& name) const override;

        void UnloadShader(const std::string& name, const ShaderVariant& variant) override;
        void UnloadShaderProgram(const std::string& name) override;
        void UnloadAllShaders() override;

        // ========================================================================
        // Hot-Reload System
        // ========================================================================

        void EnableHotReload(std::shared_ptr<IHotReloadCallback> callback) override;
        void DisableHotReload() override;
        bool IsHotReloadEnabled() const override;

        void ForceRecompileShader(const std::string& name, const ShaderVariant& variant) override;
        void ForceRecompileProgram(const std::string& name) override;
        void ForceRecompileAll() override;

        void AddFileWatch(const std::filesystem::path& filePath) override;
        void RemoveFileWatch(const std::filesystem::path& filePath) override;

        // ========================================================================
        // Cache Management
        // ========================================================================

        void ClearCache() override;
        void ValidateCache() override;
        std::uint64_t GetCacheSize() const override;
        std::uint32_t GetCachedShaderCount() const override;

        void SaveCacheToDisk() override;
        void LoadCacheFromDisk() override;

        // ========================================================================
        // Statistics and Debugging
        // ========================================================================

        const Statistics& GetStatistics() const override { return statistics_; }
        void ResetStatistics() override;

        void DumpShaderInfo(const std::string& shaderName, std::ostream& output) const override;
        void DumpAllShadersInfo(std::ostream& output) const override;

        // ========================================================================
        // Utility Functions
        // ========================================================================

        std::uint64_t GenerateVariantHash(const ShaderVariant& variant) const override;

        Core::Result<void*, Core::ErrorInfo> ValidateShaderSource(
            const std::filesystem::path& sourcePath,
            ShaderStage stage
        ) const override;

        std::vector<Configuration::ShaderModel> GetSupportedShaderModels() const override;

        bool SupportsShaderModel(Configuration::ShaderModel model) const override;
        bool SupportsRayTracing() const override;
        bool SupportsMeshShaders() const override;
        bool SupportsVariableRateShading() const override;

        // ========================================================================
        // Update and Maintenance
        // ========================================================================

        void Update() override;
        void FlushAsyncOperations() override;
        std::uint32_t GetPendingCompilationCount() const override;

        // ========================================================================
        // Enhanced Multiple Entry Point Support
        // ========================================================================

        std::vector<std::string> GetAvailableEntryPoints(
            const std::filesystem::path& shaderFile
        ) const override;

        Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo> CompileShaderEntryPoint(
            const std::filesystem::path& sourceFile,
            const std::string& entryPoint,
            ShaderStage stage,
            const ShaderVariant& variant,
            Configuration::ShaderOptimization optimization
        ) override;

        Core::Result<std::vector<std::shared_ptr<IShader>>, Core::ErrorInfo> CompileAllEntryPoints(
            const std::filesystem::path& shaderFile,
            ShaderStage primaryStage,
            const ShaderVariant& variant
        ) override;

        Core::Result<std::shared_ptr<IShaderProgram>, Core::ErrorInfo> CreateGraphicsProgramFromFile(
            const std::string& name,
            const std::filesystem::path& shaderFile,
            const std::string& vertexEntryPoint,
            const std::string& pixelEntryPoint,
            const std::string& geometryEntryPoint,
            const std::string& hullEntryPoint,
            const std::string& domainEntryPoint,
            const ShaderVariant& variant
        ) override;

        // ========================================================================
        // Constant Buffer Binding and Layout Validation
        // ========================================================================

        Core::Result<void*, Core::ErrorInfo> ValidateConstantBufferLayout(
            const std::shared_ptr<IShader>& shader,
            const std::string& expectedLayoutName
        ) override;

        std::vector<std::pair<std::string, std::uint32_t>> GetConstantBufferBindings(
            const std::shared_ptr<IShader>& shader
        ) const override;

        Core::Result<void*, Core::ErrorInfo> ValidateProgramConstantBuffers(
            const std::shared_ptr<IShaderProgram>& program,
            const std::vector<std::string>& requiredBuffers
        ) override;

        bool ValidateRegisterLayout(
            const std::shared_ptr<IShader>& shader
        ) const override;

        // ========================================================================
        // Enhanced Include Dependency Management
        // ========================================================================

        std::vector<std::filesystem::path> GetShaderDependencies(
            const std::filesystem::path& shaderFile
        ) const override;

        std::vector<std::string> FindShadersUsingInclude(
            const std::filesystem::path& includeFile
        ) const override;

        void AddIncludePath(const std::filesystem::path& includePath) override;
        void RemoveIncludePath(const std::filesystem::path& includePath) override;
        std::vector<std::filesystem::path> GetIncludePaths() const override;
        void RefreshDependencyTracking() override;

        // ========================================================================
        // Shader Preprocessing and Macro Management
        // ========================================================================

        Core::Result<std::string, Core::ErrorInfo> PreprocessShader(
            const std::filesystem::path& sourceFile,
            const ShaderVariant& variant
        ) const override;

        Core::Result<void*, Core::ErrorInfo> ValidateShaderDefines(
            const ShaderVariant& variant
        ) const override;

        std::vector<std::string> GetShaderMacros(
            const std::filesystem::path& shaderFile
        ) const override;

        bool HasFeatureMacro(
            const std::shared_ptr<IShader>& shader,
            const std::string& macroName
        ) const override;

        // ========================================================================
        // Shader Permutation and Variant Management
        // ========================================================================

        std::vector<ShaderVariant> GenerateShaderVariants(
            const std::filesystem::path& shaderFile,
            const std::vector<std::string>& conditionalDefines
        ) const override;

        Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo> CompileShaderWithAutoVariants(
            const std::filesystem::path& sourceFile,
            const std::string& entryPoint,
            ShaderStage stage,
            const std::vector<std::string>& enabledFeatures
        ) override;

        ShaderVariant GetOptimalVariant(
            const std::filesystem::path& shaderFile,
            ShaderStage stage
        ) const override;

        // ========================================================================
        // Advanced Debugging and Profiling Support
        // ========================================================================

        Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo> CompileShaderWithDebugInfo(
            const ShaderCompileRequest& request,
            bool generateAssembly,
            bool preserveDebugInfo
        ) override;

        Core::Result<std::string, Core::ErrorInfo> GetShaderAssembly(
            const std::shared_ptr<IShader>& shader
        ) const override;

        CompilationProfile GetCompilationProfile(
            const std::shared_ptr<IShader>& shader
        ) const override;

        void SetProfilingEnabled(bool enabled) override { profilingEnabled_.store(enabled); }
        bool IsProfilingEnabled() const override { return profilingEnabled_.load(); }

    private:
        // ========================================================================
        // Private Implementation Details
        // ========================================================================

        Memory::IAllocator& allocator_;
        std::shared_ptr<RHI::IRenderDevice> renderDevice_;
        Configuration::ShaderConfig config_;
        std::atomic<bool> isInitialized_{ false };

        // Shader storage
        std::unordered_map<std::string, std::shared_ptr<D3D12Shader>> shaders_;
        std::unordered_map<std::string, std::shared_ptr<D3D12ShaderProgram>> programs_;
        mutable std::shared_mutex shadersMutex_;
        mutable std::shared_mutex programsMutex_;

        // Sub-systems
        std::unique_ptr<ShaderCache> cache_;
        std::unique_ptr<HotReloadManager> hotReloadManager_;

        // Include paths
        std::vector<std::filesystem::path> includePaths_;
        mutable std::shared_mutex includePathsMutex_;

        // Async compilation
        struct AsyncCompileTask {
            ShaderCompileRequest request;
            std::function<void(Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo>)> callback;
            std::chrono::steady_clock::time_point submitTime;
        };

        std::queue<AsyncCompileTask> asyncTasks_;
        std::vector<std::thread> workerThreads_;
        mutable std::mutex asyncTasksMutex_;
        std::condition_variable asyncTasksCV_;
        std::atomic<bool> shutdownWorkers_{ false };

        // Statistics
        mutable Statistics statistics_;
        mutable std::mutex statisticsMutex_;

        // Profiling
        std::atomic<bool> profilingEnabled_{ false };

        // Logging
        Logging::LogChannel& logChannel_;

        // ========================================================================
        // Private Helper Methods
        // ========================================================================

        // Compilation helpers
        Core::Result<ComPtr<ID3DBlob>, Core::ErrorInfo> CompileHLSL(
            const std::string& source,
            const std::string& entryPoint,
            const std::string& target,
            const ShaderVariant& variant,
            Configuration::ShaderOptimization optimization,
            bool generateDebugInfo,
            CompilerIncludeHandler* includeHandler
        );

        Core::Result<std::unique_ptr<ShaderReflectionData>, Core::ErrorInfo> ReflectShader(
            ID3DBlob* bytecode,
            const std::string& shaderName,
            ShaderStage stage,
            const std::string& entryPoint,
            const std::vector<std::filesystem::path>& includedFiles
        );

        std::string GetShaderTarget(ShaderStage stage, Configuration::ShaderModel model) const;
        UINT GetCompileFlags(Configuration::ShaderOptimization optimization, bool generateDebugInfo) const;

        // Resource management helpers
        std::string GenerateShaderKey(const std::string& name, const ShaderVariant& variant) const;

        // File I/O helpers
        Core::Result<std::string, Core::ErrorInfo> ReadShaderFile(const std::filesystem::path& filePath) const;
        std::uint64_t CalculateSourceHash(const std::string& source, const ShaderVariant& variant) const;

        // Dependency tracking
        void UpdateShaderDependencies(const std::string& shaderName, const std::vector<std::filesystem::path>& dependencies);

        // Worker thread management
        void StartWorkerThreads();
        void StopWorkerThreads();
        void AsyncWorkerFunction();

        // Statistics updates
        void UpdateCompilationStatistics(const std::chrono::milliseconds& compilationTime, bool success);

        /// <summary>
        /// Gets D3D compilation flags based on optimization settings
        /// </summary>
        /// <param name="optimization">Optimization level from configuration</param>
        /// <returns>D3D compile flags</returns>
        UINT GetCompilationFlags(Configuration::ShaderOptimization optimization) const;

        /// <summary>
        /// Gets target shader profile string for the given stage
        /// </summary>
        /// <param name="stage">Shader stage (VS, PS, CS, etc.)</param>
        /// <returns>Target profile string (e.g., "vs_6_0")</returns>
        std::string GetTargetProfile(ShaderStage stage) const;
    };

} // namespace Akhanda::Shaders::D3D12