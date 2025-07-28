// Engine/Renderer/Shaders/IShaderManager.ixx
// Akhanda Game Engine - Shader Manager Interface
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <span>
#include <filesystem>
#include <chrono>

export module Akhanda.Engine.Shaders;

import Akhanda.Core.Memory;
import Akhanda.Core.Logging;
import Akhanda.Core.Math;
import Akhanda.Core.Result;
import Akhanda.Core.Configuration;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Engine.RHI.Interfaces;

export namespace Akhanda::Shaders {

    // ============================================================================
    // Forward Declarations
    // ============================================================================

    class IShaderManager;
    class IShader;
    class IShaderProgram;
    class IShaderReflection;
    class ShaderCompiler;
    class ShaderCache;
    class HotReloadManager;

    // ============================================================================
    // Shader System Enumerations
    // ============================================================================

    enum class ShaderStage : std::uint32_t {
        Vertex = 0,
        Hull = 1,           // Tessellation control
        Domain = 2,         // Tessellation evaluation  
        Geometry = 3,
        Pixel = 4,          // Fragment shader
        Compute = 5,
        Amplification = 6,  // Mesh shader pipeline
        Mesh = 7,           // Mesh shader pipeline
        RayGeneration = 8,  // Ray tracing
        Miss = 9,           // Ray tracing
        ClosestHit = 10,    // Ray tracing
        AnyHit = 11,        // Ray tracing
        Intersection = 12,  // Ray tracing
        Callable = 13       // Ray tracing
    };

    enum class ShaderDataType : std::uint32_t {
        Bool = 0,
        Int,
        UInt,
        Float,
        Double,
        Float2,
        Float3,
        Float4,
        Int2,
        Int3,
        Int4,
        UInt2,
        UInt3,
        UInt4,
        Matrix2x2,
        Matrix3x3,
        Matrix4x4,
        Matrix2x3,
        Matrix2x4,
        Matrix3x2,
        Matrix3x4,
        Matrix4x2,
        Matrix4x3,
        Sampler,
        Texture1D,
        Texture2D,
        Texture3D,
        TextureCube,
        Texture1DArray,
        Texture2DArray,
        TextureCubeArray,
        Buffer,
        StructuredBuffer,
        RWBuffer,
        RWStructuredBuffer,
        ByteAddressBuffer,
        RWByteAddressBuffer,
        AppendStructuredBuffer,
        ConsumeStructuredBuffer,
        RWTexture1D,
        RWTexture2D,
        RWTexture3D,
        RWTexture1DArray,
        RWTexture2DArray,
        Custom
    };

    enum class ShaderResourceType : std::uint32_t {
        ConstantBuffer = 0,     // CBV
        ShaderResource = 1,     // SRV
        UnorderedAccess = 2,    // UAV
        Sampler = 3,            // Sampler
        RootConstant = 4        // Root constants (32-bit values)
    };

    enum class ShaderCompilationResult : std::uint32_t {
        Success = 0,
        FileNotFound,
        SyntaxError,
        SemanticError,
        LinkError,
        InternalError,
        UnsupportedShaderModel,
        InvalidEntryPoint,
        CacheCorrupted,
        OutOfMemory
    };

    // ============================================================================
    // Shader Parameter and Reflection Data Structures
    // ============================================================================

    struct ShaderParameter {
        std::string name;
        ShaderDataType dataType;
        std::uint32_t size;           // Size in bytes
        std::uint32_t offset;         // Offset within constant buffer
        std::uint32_t arraySize;      // Array element count (1 if not array)
        std::uint32_t columns;        // Matrix columns
        std::uint32_t rows;           // Matrix rows
        bool isArray;
        bool isMatrix;
        bool isStruct;
        std::string structTypeName;   // For custom structs

        // Default value support
        std::vector<std::uint8_t> defaultValue;
        bool hasDefaultValue = false;
    };

    struct ShaderResource {
        std::string name;
        ShaderResourceType resourceType;
        std::uint32_t bindPoint;          // Register number (b0, t0, u0, s0)
        std::uint32_t bindCount;          // Number of resources (for arrays)
        std::uint32_t space;              // Register space (space0, space1, etc.)
        ShaderStage visibleStages;        // Which stages can access this resource
        std::uint32_t size;               // Size for constant buffers

        // For constant buffers, contains parameter layout
        std::vector<ShaderParameter> parameters;
    };

    struct ShaderReflectionData {
        std::string shaderName;
        ShaderStage stage;
        std::string entryPoint;
        std::string shaderModel;
        std::string targetProfile;             // Actual compilation target (e.g., "vs_6_0")

        // Resource bindings
        std::vector<ShaderResource> constantBuffers;
        std::vector<ShaderResource> shaderResources;
        std::vector<ShaderResource> unorderedAccessViews;
        std::vector<ShaderResource> samplers;
        std::vector<ShaderResource> rootConstants;

        // Input/Output signatures
        std::vector<ShaderParameter> inputSignature;
        std::vector<ShaderParameter> outputSignature;

        // Thread group dimensions (for compute shaders)
        std::uint32_t threadGroupSizeX = 1;
        std::uint32_t threadGroupSizeY = 1;
        std::uint32_t threadGroupSizeZ = 1;

        // Statistics
        std::uint32_t instructionCount = 0;
        std::uint32_t tempRegisterCount = 0;
        std::uint32_t constantBufferCount = 0;
        std::uint32_t textureCount = 0;
        std::uint32_t samplerCount = 0;

        // Root signature hash for validation
        std::uint64_t rootSignatureHash = 0;

        // Compilation metadata
        std::chrono::steady_clock::time_point compilationTime;
        std::uint64_t sourceCodeHash = 0;
        std::vector<std::string> includedFiles;  // For hot-reload dependency tracking

        // Enhanced dependency and feature tracking
        struct IncludeDependency {
            std::filesystem::path filePath;
            std::uint64_t fileHash;
            std::filesystem::file_time_type modificationTime;
            bool isSystemInclude;               // <> vs "" includes
        };
        std::vector<IncludeDependency> dependencies;

        // Feature usage detection
        std::vector<std::string> detectedFeatures;      // Features found in shader (e.g., "RAYTRACING")
        std::vector<std::string> requiredExtensions;    // Required GPU extensions
        std::vector<std::string> conditionalDefines;    // #ifdef defines found in shader

        // Alternative entry points found in the same file
        std::vector<std::string> availableEntryPoints;

        // Register layout validation
        bool usesStandardRegisterLayout = true;         // Follows Akhanda register conventions
        std::vector<std::string> registerLayoutWarnings; // Issues with register usage

        // Performance hints
        struct PerformanceHint {
            std::string message;
            enum class Severity : std::uint8_t {
                Info = 0,
                Warning = 1,
                Error = 2
            } severity = Severity::Info;
            std::uint32_t line = 0;
            std::uint32_t column = 0;
        };
        std::vector<PerformanceHint> performanceHints;

        // Shader complexity metrics
        float estimatedGPUCost = 0.0f;                  // Estimated relative GPU cost
        std::uint32_t estimatedMemoryUsage = 0;         // Estimated VRAM usage in bytes
        bool isDynamicallyUniform = true;               // All branches are uniform

        // Compilation environment
        std::string compilerVersion;
        std::vector<std::string> compilerFlags;
        std::chrono::milliseconds compilationDuration = std::chrono::milliseconds(0);
    };

    // ============================================================================
    // Shader Variant System
    // ============================================================================

    struct ShaderDefine {
        std::string name;
        std::string value;

        ShaderDefine() = default;
        ShaderDefine(const std::string& n, const std::string& v) : name(n), value(v) {}
        ShaderDefine(const std::string& n, std::int32_t v) : name(n), value(std::to_string(v)) {}
        ShaderDefine(const std::string& n, std::uint32_t v) : name(n), value(std::to_string(v)) {}
        ShaderDefine(const std::string& n, float v) : name(n), value(std::to_string(v)) {}
        ShaderDefine(const std::string& n, bool v) : name(n), value(v ? "1" : "0") {}

        bool operator==(const ShaderDefine& other) const {
            return name == other.name && value == other.value;
        }

        bool operator<(const ShaderDefine& other) const {
            if (name != other.name) return name < other.name;
            return value < other.value;
        }
    };

    struct ShaderVariant {
        std::vector<ShaderDefine> defines;
        std::uint64_t hash = 0;  // Computed hash of defines for fast lookup

        void ComputeHash() {
            // Simple hash combination for shader variant identification
            hash = 0;
            for (const auto& define : defines) {
                hash ^= std::hash<std::string>{}(define.name) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                hash ^= std::hash<std::string>{}(define.value) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
        }

        bool operator==(const ShaderVariant& other) const {
            return hash == other.hash && defines == other.defines;
        }
    };

    // ============================================================================
    // Shader Compilation Request and Result
    // ============================================================================

    struct ShaderCompileRequest {
        std::filesystem::path sourceFile;
        std::string entryPoint;
        ShaderStage stage;
        ShaderVariant variant;
        Configuration::ShaderOptimization optimization;
        Configuration::ShaderModel shaderModel;
        bool generateDebugInfo = false;
        std::vector<std::string> additionalIncludePaths;

        // Enhanced features for multiple entry points and better control
        std::vector<std::string> alternativeEntryPoints;  // Additional entry points to try if primary fails
        bool autoDetectStage = false;                     // Auto-detect shader stage from entry point name
        bool validateRegisterLayout = true;               // Validate constant buffer register layout
        bool enableStrictValidation = true;               // Enable strict HLSL validation
        std::string targetProfile;                        // Override target profile (e.g., "vs_6_0")

        // Compilation behavior control
        bool enableWarningsAsErrors = false;              // Treat warnings as compilation errors  
        bool enableOptimizeForSize = false;               // Optimize for size instead of speed
        bool preserveDebugInfo = false;                   // Keep debug info even in release builds
        std::uint32_t maxInstructionCount = 0;            // Maximum allowed instruction count (0 = no limit)

        // Priority for compilation queue
        enum class Priority : std::uint32_t {
            Low = 0,
            Normal = 1,
            High = 2,
            Critical = 3  // Blocks rendering
        } priority = Priority::Normal;

        // Source modification tracking
        std::uint64_t expectedSourceHash = 0;             // Expected source hash for validation
        std::filesystem::file_time_type expectedModTime;  // Expected modification time

        // Dependency management
        std::vector<std::filesystem::path> explicitDependencies;  // Additional files to track for hot-reload
        bool trackIncludeDependencies = true;             // Automatically track #include dependencies
    };

    struct ShaderCompileResult {
        ShaderCompilationResult result;
        std::string errorMessage;
        std::string warningMessages;

        // Compiled shader data
        std::vector<std::uint8_t> bytecode;
        std::unique_ptr<ShaderReflectionData> reflection;

        // Compilation statistics
        std::chrono::milliseconds compilationTime{ 0 };
        std::uint64_t bytecodeSize = 0;
        std::uint32_t warningCount = 0;

        bool IsSuccess() const { return result == ShaderCompilationResult::Success; }
        bool HasWarnings() const { return warningCount > 0 && !warningMessages.empty(); }
    };

    // ============================================================================
    // Shader and Shader Program Interfaces
    // ============================================================================

    class IShader {
    public:
        virtual ~IShader() = default;

        // Basic properties
        virtual const std::string& GetName() const = 0;
        virtual ShaderStage GetStage() const = 0;
        virtual const ShaderVariant& GetVariant() const = 0;
        virtual const ShaderReflectionData& GetReflection() const = 0;

        // Compilation info
        virtual bool IsCompiled() const = 0;
        virtual const std::vector<std::uint8_t>& GetBytecode() const = 0;
        virtual std::uint64_t GetSourceHash() const = 0;
        virtual std::chrono::steady_clock::time_point GetCompilationTime() const = 0;

        // Hot-reload support
        virtual bool NeedsRecompilation() const = 0;
        virtual void MarkForRecompilation() = 0;

        // Resource binding helpers
        virtual std::optional<std::uint32_t> GetConstantBufferBinding(const std::string& name) const = 0;
        virtual std::optional<std::uint32_t> GetTextureBinding(const std::string& name) const = 0;
        virtual std::optional<std::uint32_t> GetSamplerBinding(const std::string& name) const = 0;
        virtual std::optional<std::uint32_t> GetUAVBinding(const std::string& name) const = 0;
    };

    class IShaderProgram {
    public:
        virtual ~IShaderProgram() = default;

        // Program properties
        virtual const std::string& GetName() const = 0;
        virtual bool IsLinked() const = 0;
        virtual std::uint64_t GetProgramHash() const = 0;

        // Shader access
        virtual std::shared_ptr<IShader> GetShader(ShaderStage stage) const = 0;
        virtual const std::vector<ShaderStage>& GetActiveStages() const = 0;

        // Combined reflection data for all stages
        virtual const ShaderReflectionData& GetCombinedReflection() const = 0;

        // Resource binding (searches across all stages)
        virtual std::optional<std::uint32_t> FindConstantBufferBinding(const std::string& name) const = 0;
        virtual std::optional<std::uint32_t> FindTextureBinding(const std::string& name) const = 0;
        virtual std::optional<std::uint32_t> FindSamplerBinding(const std::string& name) const = 0;
        virtual std::optional<std::uint32_t> FindUAVBinding(const std::string& name) const = 0;

        // Hot-reload support
        virtual bool NeedsRecompilation() const = 0;
        virtual void MarkForRecompilation() = 0;
    };

    // ============================================================================
    // Hot-Reload and File Watching
    // ============================================================================

    class IHotReloadCallback {
    public:
        virtual ~IHotReloadCallback() = default;
        virtual void OnShaderChanged(const std::filesystem::path& shaderPath) = 0;
        virtual void OnShaderRecompiled(const std::string& shaderName, bool success, const std::string& errorMessage) = 0;
        virtual void OnShaderDependencyChanged(const std::filesystem::path& dependencyPath,
            const std::vector<std::string>& affectedShaders) = 0;
    };

    // ============================================================================
    // Main Shader Manager Interface
    // ============================================================================

    class IShaderManager {
    public:
        virtual ~IShaderManager() = default;

        // ========================================================================
        // Initialization and Configuration
        // ========================================================================

        virtual Core::Result<void*, Core::ErrorInfo> Initialize(
            std::shared_ptr<RHI::IRenderDevice> renderDevice,
            const Configuration::ShaderConfig& config
        ) = 0;

        virtual void Shutdown() = 0;
        virtual bool IsInitialized() const = 0;

        virtual void UpdateConfiguration(const Configuration::ShaderConfig& config) = 0;
        virtual const Configuration::ShaderConfig& GetConfiguration() const = 0;

        // ========================================================================
        // Shader Compilation
        // ========================================================================

        // Compile a single shader
        virtual Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo> CompileShader(
            const ShaderCompileRequest& request
        ) = 0;

        // Compile shader asynchronously (returns immediately, calls callback when done)
        virtual void CompileShaderAsync(
            const ShaderCompileRequest& request,
            std::function<void(Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo>)> callback
        ) = 0;

        // Create shader from precompiled bytecode
        virtual Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo> CreateShaderFromBytecode(
            const std::string& name,
            ShaderStage stage,
            const std::vector<std::uint8_t>& bytecode,
            const ShaderVariant& variant = ShaderVariant{}
            ) = 0;

            // ========================================================================
            // Shader Program Management
            // ========================================================================

            // Create a shader program from individual shaders
            virtual Core::Result<std::shared_ptr<IShaderProgram>, Core::ErrorInfo> CreateShaderProgram(
                const std::string& name,
                const std::vector<std::shared_ptr<IShader>>& shaders
            ) = 0;

            // Create a complete graphics pipeline shader program
            virtual Core::Result<std::shared_ptr<IShaderProgram>, Core::ErrorInfo> CreateGraphicsProgram(
                const std::string& name,
                const std::filesystem::path& vertexShaderPath,
                const std::filesystem::path& pixelShaderPath,
                const std::filesystem::path& geometryShaderPath = std::filesystem::path{},
                const std::filesystem::path& hullShaderPath = std::filesystem::path{},
                const std::filesystem::path& domainShaderPath = std::filesystem::path{},
                const ShaderVariant& variant = ShaderVariant{}
            ) = 0;

            // Create a compute shader program
            virtual Core::Result<std::shared_ptr<IShaderProgram>, Core::ErrorInfo> CreateComputeProgram(
                const std::string& name,
                const std::filesystem::path& computeShaderPath,
                const ShaderVariant& variant = ShaderVariant{}
            ) = 0;

            // ========================================================================
            // Shader Retrieval and Management
            // ========================================================================

            virtual std::shared_ptr<IShader> GetShader(const std::string& name, const ShaderVariant& variant = ShaderVariant{}) const = 0;
            virtual std::shared_ptr<IShaderProgram> GetShaderProgram(const std::string& name) const = 0;

            virtual std::vector<std::string> GetLoadedShaderNames() const = 0;
            virtual std::vector<std::string> GetLoadedProgramNames() const = 0;

            virtual bool HasShader(const std::string& name, const ShaderVariant& variant = {}) const = 0;
            virtual bool HasShaderProgram(const std::string& name) const = 0;

            virtual void UnloadShader(const std::string& name, const ShaderVariant& variant = {}) = 0;
            virtual void UnloadShaderProgram(const std::string& name) = 0;
            virtual void UnloadAllShaders() = 0;

            // ========================================================================
            // Hot-Reload System
            // ========================================================================

            virtual void EnableHotReload(std::shared_ptr<IHotReloadCallback> callback) = 0;
            virtual void DisableHotReload() = 0;
            virtual bool IsHotReloadEnabled() const = 0;

            virtual void ForceRecompileShader(const std::string& name, const ShaderVariant& variant = ShaderVariant{}) = 0;
            virtual void ForceRecompileProgram(const std::string& name) = 0;
            virtual void ForceRecompileAll() = 0;

            // Manual file watch management
            virtual void AddFileWatch(const std::filesystem::path& filePath) = 0;
            virtual void RemoveFileWatch(const std::filesystem::path& filePath) = 0;

            // ========================================================================
            // Cache Management
            // ========================================================================

            virtual void ClearCache() = 0;
            virtual void ValidateCache() = 0;
            virtual std::uint64_t GetCacheSize() const = 0;
            virtual std::uint32_t GetCachedShaderCount() const = 0;

            virtual void SaveCacheToDisk() = 0;
            virtual void LoadCacheFromDisk() = 0;

            // ========================================================================
            // Statistics and Debugging
            // ========================================================================

            struct Statistics {
                std::uint32_t totalShadersLoaded = 0;
                std::uint32_t totalProgramsLoaded = 0;
                std::uint32_t compilationRequests = 0;
                std::uint32_t compilationSuccesses = 0;
                std::uint32_t compilationFailures = 0;
                std::uint32_t cacheHits = 0;
                std::uint32_t cacheMisses = 0;
                std::uint32_t hotReloadEvents = 0;
                std::chrono::milliseconds totalCompilationTime{ 0 };
                std::chrono::milliseconds averageCompilationTime{ 0 };
                std::uint64_t totalMemoryUsed = 0;
                std::uint64_t cacheMemoryUsed = 0;
            };

            virtual const Statistics& GetStatistics() const = 0;
            virtual void ResetStatistics() = 0;

            virtual void DumpShaderInfo(const std::string& shaderName, std::ostream& output) const = 0;
            virtual void DumpAllShadersInfo(std::ostream& output) const = 0;

            // ========================================================================
            // Utility Functions
            // ========================================================================

            // Generate shader variant hash for caching
            virtual std::uint64_t GenerateVariantHash(const ShaderVariant& variant) const = 0;

            // Validate shader source for common errors before compilation
            virtual Core::Result<void*, Core::ErrorInfo> ValidateShaderSource(
                const std::filesystem::path& sourcePath,
                ShaderStage stage
            ) const = 0;

            // Get supported shader models for current device
            virtual std::vector<Configuration::ShaderModel> GetSupportedShaderModels() const = 0;

            // Check if specific shader features are supported
            virtual bool SupportsShaderModel(Configuration::ShaderModel model) const = 0;
            virtual bool SupportsRayTracing() const = 0;
            virtual bool SupportsMeshShaders() const = 0;
            virtual bool SupportsVariableRateShading() const = 0;

            // ========================================================================
            // Update and Maintenance
            // ========================================================================

            // Called every frame to handle hot-reload, async compilation, etc.
            virtual void Update() = 0;

            // Flush all pending async operations (blocking)
            virtual void FlushAsyncOperations() = 0;

            // Get number of pending async compilation requests
            virtual std::uint32_t GetPendingCompilationCount() const = 0;

            // ========================================================================
            // Enhanced Multiple Entry Point Support
            // ========================================================================

            // Get all available entry points in a shader file
            virtual std::vector<std::string> GetAvailableEntryPoints(
                const std::filesystem::path& shaderFile
            ) const = 0;

            // Compile specific entry point from a multi-entry shader file
            virtual Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo> CompileShaderEntryPoint(
                const std::filesystem::path& sourceFile,
                const std::string& entryPoint,
                ShaderStage stage,
                const ShaderVariant& variant = ShaderVariant{},
                Configuration::ShaderOptimization optimization = Configuration::ShaderOptimization::Debug
            ) = 0;

            // Compile all standard entry points for a shader file
            virtual Core::Result<std::vector<std::shared_ptr<IShader>>, Core::ErrorInfo> CompileAllEntryPoints(
                const std::filesystem::path& shaderFile,
                ShaderStage primaryStage,
                const ShaderVariant& variant = ShaderVariant{}
            ) = 0;

            // Compile common entry point combinations for graphics shaders
            virtual Core::Result<std::shared_ptr<IShaderProgram>, Core::ErrorInfo> CreateGraphicsProgramFromFile(
                const std::string& name,
                const std::filesystem::path& shaderFile,
                const std::string& vertexEntryPoint = "main",
                const std::string& pixelEntryPoint = "main",
                const std::string& geometryEntryPoint = "",
                const std::string& hullEntryPoint = "",
                const std::string& domainEntryPoint = "",
                const ShaderVariant& variant = ShaderVariant{}
            ) = 0;

            // ========================================================================
            // Constant Buffer Binding and Layout Validation
            // ========================================================================

            // Validate that shader's constant buffer layout matches expected engine layout
            virtual Core::Result<void*, Core::ErrorInfo> ValidateConstantBufferLayout(
                const std::shared_ptr<IShader>& shader,
                const std::string& expectedLayoutName = "AkhandaStandard"
            ) = 0;

            // Get constant buffer binding information for a shader
            virtual std::vector<std::pair<std::string, std::uint32_t>> GetConstantBufferBindings(
                const std::shared_ptr<IShader>& shader
            ) const = 0;

            // Verify that a shader program has all required constant buffers
            virtual Core::Result<void*, Core::ErrorInfo> ValidateProgramConstantBuffers(
                const std::shared_ptr<IShaderProgram>& program,
                const std::vector<std::string>& requiredBuffers
            ) = 0;

            // Check if shader uses expected register slots (b0-b31 layout from Constants.hlsli)
            virtual bool ValidateRegisterLayout(
                const std::shared_ptr<IShader>& shader
            ) const = 0;

            // ========================================================================
            // Enhanced Include Dependency Management
            // ========================================================================

            // Get all include dependencies for a shader file
            virtual std::vector<std::filesystem::path> GetShaderDependencies(
                const std::filesystem::path& shaderFile
            ) const = 0;

            // Find all shaders that depend on a specific include file
            virtual std::vector<std::string> FindShadersUsingInclude(
                const std::filesystem::path& includeFile
            ) const = 0;

            // Add custom include search path (beyond the configured ones)
            virtual void AddIncludePath(const std::filesystem::path& includePath) = 0;

            // Remove custom include search path
            virtual void RemoveIncludePath(const std::filesystem::path& includePath) = 0;

            // Get all current include search paths
            virtual std::vector<std::filesystem::path> GetIncludePaths() const = 0;

            // Refresh dependency tracking for all loaded shaders
            virtual void RefreshDependencyTracking() = 0;

            // ========================================================================
            // Shader Preprocessing and Macro Management
            // ========================================================================

            // Preprocess shader source and return processed HLSL (useful for debugging)
            virtual Core::Result<std::string, Core::ErrorInfo> PreprocessShader(
                const std::filesystem::path& sourceFile,
                const ShaderVariant& variant = ShaderVariant{}
            ) const = 0;

            // Validate shader macros and defines before compilation
            virtual Core::Result<void*, Core::ErrorInfo> ValidateShaderDefines(
                const ShaderVariant& variant
            ) const = 0;

            // Get list of all macros used in a shader file
            virtual std::vector<std::string> GetShaderMacros(
                const std::filesystem::path& shaderFile
            ) const = 0;

            // Check if specific feature macros are defined (e.g., AKHANDA_HAS_RAYTRACING)
            virtual bool HasFeatureMacro(
                const std::shared_ptr<IShader>& shader,
                const std::string& macroName
            ) const = 0;

            // ========================================================================
            // Shader Permutation and Variant Management
            // ========================================================================

            // Generate all possible shader variants based on available defines
            virtual std::vector<ShaderVariant> GenerateShaderVariants(
                const std::filesystem::path& shaderFile,
                const std::vector<std::string>& conditionalDefines = std::vector<std::string>{}
            ) const = 0;

            // Compile shader with automatic variant detection based on feature usage
            virtual Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo> CompileShaderWithAutoVariants(
                const std::filesystem::path& sourceFile,
                const std::string& entryPoint,
                ShaderStage stage,
                const std::vector<std::string>& enabledFeatures = std::vector<std::string>{}
            ) = 0;

            // Get optimal shader variant for current hardware/settings
            virtual ShaderVariant GetOptimalVariant(
                const std::filesystem::path& shaderFile,
                ShaderStage stage
            ) const = 0;

            // ========================================================================
            // Advanced Debugging and Profiling Support
            // ========================================================================

            // Compile shader with debug information and symbols
            virtual Core::Result<std::shared_ptr<IShader>, Core::ErrorInfo> CompileShaderWithDebugInfo(
                const ShaderCompileRequest& request,
                bool generateAssembly = false,
                bool preserveDebugInfo = true
            ) = 0;

            // Get shader assembly/disassembly for debugging
            virtual Core::Result<std::string, Core::ErrorInfo> GetShaderAssembly(
                const std::shared_ptr<IShader>& shader
            ) const = 0;

            // Profile shader compilation performance
            struct CompilationProfile {
                std::chrono::milliseconds preprocessingTime = std::chrono::milliseconds(0);
                std::chrono::milliseconds compilationTime = std::chrono::milliseconds(0);
                std::chrono::milliseconds reflectionTime = std::chrono::milliseconds(0);
                std::chrono::milliseconds totalTime = std::chrono::milliseconds(0);
                std::uint64_t sourceSize = 0;
                std::uint64_t bytecodeSize = 0;
                std::uint32_t instructionCount = 0;
                std::string compilerVersion;
                std::vector<std::string> warnings;
            };

            virtual CompilationProfile GetCompilationProfile(
                const std::shared_ptr<IShader>& shader
            ) const = 0;

            // Enable/disable detailed compilation profiling
            virtual void SetProfilingEnabled(bool enabled) = 0;
            virtual bool IsProfilingEnabled() const = 0;
    };

    // ============================================================================
    // Shader Manager Factory
    // ============================================================================

    class ShaderManagerFactory {
    public:
        static Core::Result<std::unique_ptr<IShaderManager>, Core::ErrorInfo> CreateShaderManager(
            Configuration::RenderingAPI api,
            Memory::IAllocator& allocator
        );
    };

} // namespace Akhanda::Shaders
