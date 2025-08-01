// RHI.Interface.ixx
// Akhanda Game Engine - Render Hardware Interface Module
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>
#include <string>
#include <format>

#undef DeviceCapabilities // Avoid macro conflicts

export module Akhanda.Engine.RHI;

import Akhanda.Core.Math;
import Akhanda.Core.Memory;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Platform.Interfaces;

using Microsoft::WRL::ComPtr;

export namespace Akhanda::RHI {

    // ========================================================================
    // Forward Declarations
    // ========================================================================

    class RenderDevice;
    class CommandList;
    class RenderBuffer;
    class RenderTexture;
    class RenderPipeline;
    class RenderPass;
    class DescriptorHeap;
    class SwapChain;

    // ========================================================================
    // Resource Types and Enumerations
    // ========================================================================

    enum class ResourceType : uint32_t {
        Buffer = 0,
        Texture1D,
        Texture2D,
        Texture3D,
        TextureCube
    };

    enum class ResourceUsage : uint32_t {
        None = 0,
        VertexBuffer = 1 << 0,
        IndexBuffer = 1 << 1,
        ConstantBuffer = 1 << 2,
        ShaderResource = 1 << 3,
        RenderTarget = 1 << 4,
        DepthStencil = 1 << 5,
        UnorderedAccess = 1 << 6,
        CopySource = 1 << 7,
        CopyDestination = 1 << 8,
    };

    // Overload the bitwise OR operator for ResourceUsage
    inline constexpr ResourceUsage operator|(ResourceUsage lhs, ResourceUsage rhs) {
        return static_cast<ResourceUsage>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
    }

    // Overload the bitwise AND operator for ResourceUsage
    inline constexpr ResourceUsage operator&(ResourceUsage lhs, ResourceUsage rhs) {
        return static_cast<ResourceUsage>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
    }

    // Overload the bitwise NOT operator for ResourceUsage
    inline constexpr ResourceUsage operator~(ResourceUsage p) {
        return static_cast<ResourceUsage>(~static_cast<uint32_t>(p));
    }

    enum class ResourceState : uint32_t {
        Common = 0,
        VertexAndConstantBuffer,
        IndexBuffer,
        RenderTarget,
        UnorderedAccess,
        DepthWrite,
        DepthRead,
        ShaderResource,
        StreamOut,
        IndirectArgument,
        CopyDestination,
        CopySource,
        ResolveDest,
        ResolveSource,
        Present,
        Predication
    };

    // Overload the bitwise OR operator for ResourceState
    inline constexpr ResourceState operator|(ResourceState lhs, ResourceState rhs) {
        return static_cast<ResourceState>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
    }

    // Overload the bitwise AND operator for ResourceState
    inline constexpr ResourceState operator&(ResourceState lhs, ResourceState rhs) {
        return static_cast<ResourceState>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
    }

    // Overload the bitwise NOT operator for ResourceState
    inline constexpr ResourceState operator~(ResourceState p) {
        return static_cast<ResourceState>(~static_cast<uint32_t>(p));
    }

    enum class CommandListType : uint32_t {
        Direct = 0,      // Graphics + Compute + Copy
        Bundle,          // Graphics commands recorded for reuse
        Compute,         // Compute + Copy
        Copy             // Copy only
    };

    enum class PrimitiveTopology : uint32_t {
        PointList = 0,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,
        TriangleFan
    };

    enum class Format : uint32_t {
        // Special/Unknown
        Undefined = 0,
        Unknown = 0,    // Alias for Undefined

        // 32-bit float formats
        R32G32B32A32_Float,
        R32G32B32_Float,
        R32G32_Float,
        R32_Float,

        // 32-bit unsigned integer formats
        R32G32B32A32_UInt,
        R32G32B32_UInt,
        R32G32_UInt,
        R32_UInt,

        // 32-bit signed integer formats
        R32G32B32A32_SInt,
        R32G32B32_SInt,
        R32G32_SInt,
        R32_SInt,

        // 16-bit float formats
        R16G16B16A16_Float,
        R16G16_Float,
        R16_Float,

        // 16-bit unsigned normalized formats
        R16G16B16A16_UNorm,
        R16G16_UNorm,
        R16_UNorm,

        // 16-bit signed normalized formats
        R16G16B16A16_SNorm,
        R16G16_SNorm,
        R16_SNorm,

        // 16-bit unsigned integer formats
        R16G16B16A16_UInt,
        R16G16_UInt,
        R16_UInt,

        // 16-bit signed integer formats
        R16G16B16A16_SInt,
        R16G16_SInt,
        R16_SInt,

        // 8-bit unsigned normalized formats
        R8G8B8A8_UNorm,
        R8G8B8A8_UNorm_sRGB,   // Added sRGB variant
        R8G8_UNorm,
        R8_UNorm,

        // 8-bit signed normalized formats
        R8G8B8A8_SNorm,
        R8G8_SNorm,
        R8_SNorm,

        // 8-bit unsigned integer formats
        R8G8B8A8_UInt,
        R8G8_UInt,
        R8_UInt,

        // 8-bit signed integer formats
        R8G8B8A8_SInt,
        R8G8_SInt,
        R8_SInt,

        // BGRA formats
        B8G8R8A8_UNorm,
        B8G8R8A8_UNorm_sRGB,   // Added sRGB variant
        B8G8R8X8_UNorm,

        // Special formats
        R11G11B10_Float,        // Added
        R10G10B10A2_UNorm,      // Added
        R10G10B10A2_UInt,       // Added

        // Depth formats
        D32_Float,
        D24_UNorm_S8_UInt,
        D16_UNorm,
        D32_Float_S8X24_UInt,   // Added

        // Block compression formats
        BC1_UNorm,
        BC1_UNorm_sRGB,
        BC2_UNorm,
        BC2_UNorm_sRGB,
        BC3_UNorm,
        BC3_UNorm_sRGB,
        BC4_UNorm,
        BC4_SNorm,
        BC5_UNorm,
        BC5_SNorm,
        BC6H_UF16,
        BC6H_SF16,
        BC7_UNorm,
        BC7_UNorm_sRGB
    };

    enum class Filter : uint32_t {
        // Basic point/linear/anisotropic (legacy simple values)
        Point = 0,
        Linear,
        Anisotropic,

        // Detailed filter modes - Min/Mag/Mip combinations
        MinMagMipPoint,
        MinMagPointMipLinear,
        MinPointMagLinearMipPoint,
        MinPointMagMipLinear,
        MinLinearMagMipPoint,
        MinLinearMagPointMipLinear,
        MinMagLinearMipPoint,
        MinMagMipLinear,

        // Comparison filters (for shadow mapping)
        ComparisonMinMagMipPoint,
        ComparisonMinMagPointMipLinear,
        ComparisonMinPointMagLinearMipPoint,
        ComparisonMinPointMagMipLinear,
        ComparisonMinLinearMagMipPoint,
        ComparisonMinLinearMagPointMipLinear,
        ComparisonMinMagLinearMipPoint,
        ComparisonMinMagMipLinear,
        ComparisonAnisotropic,

        // Minimum filters (for special effects)
        MinimumMinMagMipPoint,
        MinimumMinMagPointMipLinear,
        MinimumMinPointMagLinearMipPoint,
        MinimumMinPointMagMipLinear,
        MinimumMinLinearMagMipPoint,
        MinimumMinLinearMagPointMipLinear,
        MinimumMinMagLinearMipPoint,
        MinimumMinMagMipLinear,
        MinimumAnisotropic,

        // Maximum filters (for special effects)
        MaximumMinMagMipPoint,
        MaximumMinMagPointMipLinear,
        MaximumMinPointMagLinearMipPoint,
        MaximumMinPointMagMipLinear,
        MaximumMinLinearMagMipPoint,
        MaximumMinLinearMagPointMipLinear,
        MaximumMinMagLinearMipPoint,
        MaximumMinMagMipLinear,
        MaximumAnisotropic
    };

    enum class AddressMode : uint32_t {
        Wrap = 0,
        Mirror,
        Clamp,
        Border,
        MirrorOnce
    };

    enum class CompareFunction : uint32_t {
        Never = 0,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always
    };

    // ========================================================================
    // Resource Descriptors
    // ========================================================================

    struct BufferDesc {
        uint64_t size = 0;
        uint32_t stride = 0;
        ResourceUsage usage = ResourceUsage::None;
        bool cpuAccessible = false;
        const void* initialData = nullptr;
        const char* debugName = nullptr;
    };

    struct TextureDesc {
        ResourceType type = ResourceType::Texture2D;
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t depth = 1;
        uint32_t mipLevels = 1;
        uint32_t arraySize = 1;
        Format format = Format::Unknown;
        uint32_t sampleCount = 1;
        uint32_t sampleQuality = 0;
        ResourceUsage usage = ResourceUsage::None;
        ResourceState initialState = ResourceState::Common;
        const void* initialData = nullptr;
        const char* debugName = nullptr;
    };

    struct SamplerDesc {
        Filter filter = Filter::Linear;
        AddressMode addressU = AddressMode::Wrap;
        AddressMode addressV = AddressMode::Wrap;
        AddressMode addressW = AddressMode::Wrap;
        float mipLODBias = 0.0f;
        uint32_t maxAnisotropy = 1;
        CompareFunction compareFunction = CompareFunction::Never;
        float borderColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float minLOD = 0.0f;
        float maxLOD = 3.402823466e+38f; // FLT_MAX
        const char* debugName = nullptr;
    };

    struct RenderTargetDesc {
        Format format = Format::Unknown;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t sampleCount = 1;
        uint32_t sampleQuality = 0;
        bool clearOnBind = true;
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        const char* debugName = nullptr;
    };

    struct DepthStencilDesc {
        Format format = Format::D32_Float;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t sampleCount = 1;
        uint32_t sampleQuality = 0;
        bool clearOnBind = true;
        float clearDepth = 1.0f;
        uint8_t clearStencil = 0;
        const char* debugName = nullptr;
    };

    // ========================================================================
    // Vertex Input Layout
    // ========================================================================

    struct VertexElement {
        const char* semanticName = nullptr;
        uint32_t semanticIndex = 0;
        Format format = Format::Unknown;
        uint32_t inputSlot = 0;
        uint32_t alignedByteOffset = 0;
        bool instanceData = false;
        uint32_t instanceDataStepRate = 0;
    };

    struct VertexInputLayout {
        std::vector<VertexElement> elements;
        uint32_t inputSlotCount = 0;
    };

    // ========================================================================
    // Shader and Pipeline Descriptors
    // ========================================================================

    struct ShaderDesc {
        const void* bytecode = nullptr;
        size_t bytecodeLength = 0;
        const char* entryPoint = nullptr;
        const char* debugName = nullptr;
    };

    struct BlendDesc {
        bool blendEnable = false;
        uint32_t srcBlend = 0;
        uint32_t destBlend = 0;
        uint32_t blendOp = 0;
        uint32_t srcBlendAlpha = 0;
        uint32_t destBlendAlpha = 0;
        uint32_t blendOpAlpha = 0;
        uint8_t renderTargetWriteMask = 0xFF;
    };

    struct RasterizerDesc {
        uint32_t fillMode = 0;
        uint32_t cullMode = 0;
        bool frontCounterClockwise = false;
        int32_t depthBias = 0;
        float depthBiasClamp = 0.0f;
        float slopeScaledDepthBias = 0.0f;
        bool depthClipEnable = true;
        bool scissorEnable = false;
        bool multisampleEnable = false;
        bool antialiasedLineEnable = false;
    };

    struct DepthStencilStateDesc {
        bool depthEnable = true;
        bool depthWriteEnable = true;
        CompareFunction depthFunc = CompareFunction::Less;
        bool stencilEnable = false;
        uint8_t stencilReadMask = 0xFF;
        uint8_t stencilWriteMask = 0xFF;
        // Stencil operations can be added here as needed
    };

    struct GraphicsPipelineDesc {
        ShaderDesc vertexShader;
        ShaderDesc pixelShader;
        ShaderDesc geometryShader;
        ShaderDesc hullShader;
        ShaderDesc domainShader;
        VertexInputLayout inputLayout;
        PrimitiveTopology topology = PrimitiveTopology::TriangleList;
        BlendDesc blendState;
        RasterizerDesc rasterizerState;
        DepthStencilStateDesc depthStencilState;
        std::vector<Format> renderTargetFormats;
        Format depthStencilFormat = Format::Unknown;
        uint32_t sampleCount = 1;
        uint32_t sampleQuality = 0;
        const char* debugName = nullptr;
    };

    struct ComputePipelineDesc {
        ShaderDesc computeShader;
        const char* debugName = nullptr;
    };

    // ========================================================================
    // Resource Binding
    // ========================================================================

    struct DescriptorRange {
        uint32_t descriptorType = 0;
        uint32_t numDescriptors = 0;
        uint32_t baseShaderRegister = 0;
        uint32_t registerSpace = 0;
        uint32_t offsetInDescriptorsFromTableStart = 0;
    };

    struct RootParameter {
        enum Type {
            DescriptorTable = 0,
            Constants32Bit,
            ConstantBufferView,
            ShaderResourceView,
            UnorderedAccessView
        };

        Type type = DescriptorTable;
        uint32_t shaderVisibility = 0;

        union {
            struct {
                uint32_t numDescriptorRanges = 0;
                const DescriptorRange* descriptorRanges = nullptr;
            } descriptorTable;

            struct {
                uint32_t shaderRegister = 0;
                uint32_t registerSpace = 0;
                uint32_t num32BitValues = 0;
            } constants;

            struct {
                uint32_t shaderRegister = 0;
                uint32_t registerSpace = 0;
            } descriptor;
        };
    };

    struct RootSignatureDesc {
        std::vector<RootParameter> parameters;
        std::vector<SamplerDesc> staticSamplers;
        uint32_t flags = 0;
        const char* debugName = nullptr;
    };

    // ========================================================================
    // Command Recording
    // ========================================================================

    struct Viewport {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float minDepth = 0.0f;
        float maxDepth = 1.0f;
    };

    struct Scissor {
        int32_t left = 0;
        int32_t top = 0;
        int32_t right = 0;
        int32_t bottom = 0;
    };

    struct DrawIndexedIndirectCommand {
        uint32_t indexCountPerInstance = 0;
        uint32_t instanceCount = 0;
        uint32_t startIndexLocation = 0;
        int32_t baseVertexLocation = 0;
        uint32_t startInstanceLocation = 0;
    };

    struct DispatchIndirectCommand {
        uint32_t threadGroupCountX = 0;
        uint32_t threadGroupCountY = 0;
        uint32_t threadGroupCountZ = 0;
    };

    // ========================================================================
    // Performance and Debug Information
    // ========================================================================

    struct RenderStats {
        uint64_t frameNumber = 0;
        uint32_t drawCalls = 0;
        uint32_t instanceCount = 0;
        uint32_t triangleCount = 0;
        uint32_t vertexCount = 0;
        uint32_t commandListCount = 0;
        uint32_t bufferCount = 0;
        uint32_t textureCount = 0;
        uint32_t pipelineCount = 0;
        uint64_t memoryUsage = 0;
        uint64_t videoMemoryUsage = 0;
        float cpuFrameTime = 0.0f;
        float gpuFrameTime = 0.0f;
    };

    struct DebugMessage {
        enum Severity {
            Info = 0,
            Warning,
            Error,
            Corruption
        };

        Severity severity = Info;
        std::string category;
        std::string message;
        uint32_t id = 0;
        uint64_t timestamp = 0;
    };

    // ========================================================================
    // Resource Handle System
    // ========================================================================

    template<typename T>
    struct ResourceHandle {
        static constexpr uint32_t InvalidIndex = UINT32_MAX;

        uint32_t index = InvalidIndex;
        uint32_t generation = 0;

        constexpr bool IsValid() const { return index != InvalidIndex; }
        constexpr void Invalidate() { index = InvalidIndex; generation = 0; }

        constexpr bool operator==(const ResourceHandle& other) const {
            return index == other.index && generation == other.generation;
        }
        constexpr bool operator!=(const ResourceHandle& other) const {
            return !(*this == other);
        }
    };

    // Resource handle type aliases
    using BufferHandle = ResourceHandle<RenderBuffer>;
    using TextureHandle = ResourceHandle<RenderTexture>;
    using PipelineHandle = ResourceHandle<RenderPipeline>;
    using RenderPassHandle = ResourceHandle<RenderPass>;

    // ========================================================================
    // Base Resource Interface
    // ========================================================================

    class IRenderResource {
    public:
        virtual ~IRenderResource() = default;
        virtual ResourceType GetType() const = 0;
        virtual uint64_t GetGPUAddress() const = 0;
        virtual void SetDebugName(const char* name) = 0;
        virtual const char* GetDebugName() const = 0;
        virtual bool IsValid() const = 0;
    };

    // ========================================================================
    // Device Capabilities
    // ========================================================================

    struct DeviceCapabilities {
        // ====================================================================
        // Basic Device Information
        // ====================================================================

        // Adapter identification (existing)
        std::string adapterDescription;        // Existing: Wide string description converted to UTF-8
        std::string adapterName;              // NEW: Friendly name for display/logging
        uint32_t vendorId = 0;                // Existing
        uint32_t deviceId = 0;                // Existing  
        uint32_t subSysId = 0;                // Existing
        uint32_t revision = 0;                // Existing
        uint64_t driverVersion = 0;           // Existing

        // NEW: Enhanced identification properties
        uint64_t adapterLuid = 0;             // NEW: Adapter LUID for multi-GPU scenarios
        uint32_t nodeIndex = 0;               // NEW: GPU node index
        bool isDiscrete = false;              // NEW: True for discrete GPUs vs integrated
        bool isWarpAdapter = false;           // NEW: True for software WARP adapter

        // ====================================================================
        // Feature Level and Shader Model Support
        // ====================================================================

        // Feature levels (existing)
        uint32_t maxFeatureLevel = 0;         // Existing: D3D_FEATURE_LEVEL value

        // Shader model support (existing and enhanced)
        bool supportsShaderModel6 = false;    // Existing: SM 6.0+ support
        bool supportsShaderModel6_6 = false;  // NEW: SM 6.6+ support (enhanced features)
        bool supportsShaderModel6_7 = false;  // NEW: SM 6.7+ support (latest features)
        uint32_t highestShaderModel = 0;      // NEW: Highest supported shader model as uint

        // ====================================================================
        // Graphics Pipeline Capabilities
        // ====================================================================

        // Modern graphics features (existing)
        bool supportsRayTracing = false;              // Existing
        bool supportsVariableRateShading = false;     // Existing  
        bool supportsMeshShaders = false;             // Existing
        bool supportsDirectStorageGPU = false;        // Existing

        // Tier information (existing)
        uint32_t raytracingTier = 0;                   // Existing: D3D12_RAYTRACING_TIER value
        uint32_t variableRateShadingTier = 0;          // Existing: D3D12_VARIABLE_SHADING_RATE_TIER value
        uint32_t meshShaderTier = 0;                   // Existing: D3D12_MESH_SHADER_TIER value
        uint32_t resourceBindingTier = 0;              // Existing: D3D12_RESOURCE_BINDING_TIER value

        // NEW: Additional pipeline capabilities
        bool supportsSamplerfeedback = false;          // NEW: Sampler feedback support
        bool supportsWorkGraphs = false;               // NEW: Work graphs support
        bool supportsExecuteIndirect = false;          // NEW: Enhanced ExecuteIndirect
        bool supportsBarrierExtensions = false;        // NEW: Enhanced barriers (D3D12_OPTIONS12)
        bool supportsDirectML = false;                 // NEW: DirectML meta-commands
        uint32_t directMLFeatureLevel = 0;             // NEW: DirectML feature level

        // ====================================================================
        // Architecture and Memory Properties
        // ====================================================================

        // Architecture (existing)
        bool isUMA = false;                    // Existing: Unified Memory Architecture
        bool isCacheCoherentUMA = false;       // Existing: Cache-coherent UMA
        bool isTileBasedRenderer = false;      // Existing: Tile-based deferred renderer

        // Memory information (existing and enhanced)
        uint64_t dedicatedVideoMemory = 0;     // Existing: Dedicated VRAM
        uint64_t sharedSystemMemory = 0;       // Existing: Shared system memory
        uint64_t availableVideoMemory = 0;     // Existing: Currently available VRAM

        // NEW: Enhanced memory information
        uint64_t dedicatedSystemMemory = 0;    // NEW: Dedicated system memory
        uint64_t totalPhysicalMemory = 0;      // NEW: Total physical memory
        uint64_t currentUsage = 0;             // NEW: Current memory usage
        uint64_t currentAvailable = 0;         // NEW: Current available memory
        uint64_t currentReservation = 0;       // NEW: Current reservation
        uint32_t memorySegmentCount = 0;       // NEW: Number of memory segments

        // ====================================================================
        // Resource Limits and Constraints
        // ====================================================================

        // Texture limits (existing)
        uint32_t maxTexture2DSize = 16384;         // Existing: Maximum 2D texture dimension
        uint32_t maxTexture3DSize = 2048;          // Existing: Maximum 3D texture dimension  
        uint32_t maxTextureCubeSize = 16384;       // Existing: Maximum cube texture dimension
        uint32_t maxTextureArraySize = 2048;       // Existing: Maximum texture array size

        // Render target limits (existing)
        uint32_t maxRenderTargets = 8;             // Existing: Maximum simultaneous render targets
        uint32_t maxViewports = 16;                // NEW: Maximum simultaneous viewports

        // Buffer and vertex limits (existing and enhanced)
        uint32_t maxVertexInputElements = 32;      // Existing: Maximum vertex input elements
        uint32_t maxConstantBufferSize = 65536;    // Existing: Maximum constant buffer size in bytes

        // NEW: Additional resource limits
        uint32_t maxStructuredBufferSize = 0;      // NEW: Maximum structured buffer size
        uint32_t maxRawBufferSize = 0;              // NEW: Maximum raw buffer size
        uint32_t maxTexture1DSize = 16384;          // NEW: Maximum 1D texture dimension
        uint32_t maxIndices = UINT32_MAX;           // NEW: Maximum index count
        uint32_t maxVertices = UINT32_MAX;          // NEW: Maximum vertex count
        uint32_t maxDrawIndirectCount = 0;          // NEW: Maximum indirect draw count
        uint32_t maxComputeSharedMemory = 0;        // NEW: Maximum compute shared memory

        // ====================================================================
        // Sampling and Multisampling Support
        // ====================================================================

        // Multi-sampling (existing and enhanced)
        std::vector<uint32_t> supportedSampleCounts;  // Existing: Supported MSAA sample counts

        // NEW: Enhanced sampling capabilities
        uint32_t maxAnisotropy = 16;                   // NEW: Maximum anisotropic filtering
        bool supportsMinMaxFiltering = false;          // NEW: Min/Max filtering support  
        bool supportsBorderColorSamplers = false;      // NEW: Border color sampler support
        bool supportsProgrammableSamplePositions = false; // NEW: Programmable sample positions

        // ====================================================================
        // Compute and Async Capabilities
        // ====================================================================

        // NEW: Compute shader capabilities
        uint32_t maxComputeThreadsPerGroup[3] = { 1024, 1024, 64 }; // NEW: Max threads per group (X,Y,Z)
        uint32_t maxComputeThreadGroupsPerDispatch[3] = { 65535, 65535, 65535 }; // NEW: Max groups per dispatch
        uint32_t maxComputeSMMemory = 32768;           // NEW: Max shared memory per thread group
        bool supportsWaveIntrinsics = false;           // NEW: Wave intrinsics support
        uint32_t waveSize = 32;                        // NEW: Wave size (typically 32 or 64)

        // NEW: Async compute capabilities  
        bool supportsAsyncCompute = false;             // NEW: Async compute queue support
        uint32_t numAsyncComputeQueues = 0;            // NEW: Number of async compute queues
        bool supportsCopyQueues = false;               // NEW: Dedicated copy queue support
        uint32_t numCopyQueues = 0;                    // NEW: Number of copy queues

        // ====================================================================
        // Debug and Development Features
        // ====================================================================

        // Debug features (existing and enhanced)
        bool debugLayerAvailable = false;             // Existing: D3D12 debug layer available
        bool gpuValidationAvailable = false;          // Existing: GPU-based validation available

        // NEW: Enhanced debug capabilities
        bool dredAvailable = false;                    // NEW: Device Removed Extended Data available
        bool pixSupportAvailable = false;             // NEW: PIX support available
        bool aftermathAvailable = false;               // NEW: NVIDIA Nsight Aftermath available
        bool supportsDebugMarkers = false;            // NEW: Debug marker/annotation support
        bool supportsTimestampQueries = false;        // NEW: Timestamp query support
        bool supportsPipelineStatistics = false;      // NEW: Pipeline statistics queries

        // ====================================================================
        // Performance and Optimization Hints
        // ====================================================================

        // NEW: Performance characteristics
        uint32_t recommendedSwapChainBuffers = 3;     // NEW: Recommended swap chain buffer count
        bool prefersBundledCommandLists = false;       // NEW: Benefits from command list bundles
        bool prefersResourceBarrierBatching = false;   // NEW: Benefits from batched barriers
        bool supportsGPUUploadHeaps = false;          // NEW: GPU upload heap support
        uint32_t minResourceAlignment = 256;           // NEW: Minimum resource alignment
        uint64_t nonLocalMemoryBandwidth = 0;         // NEW: System memory bandwidth (bytes/sec)
        uint64_t localMemoryBandwidth = 0;            // NEW: Video memory bandwidth (bytes/sec)

        // NEW: Vendor-specific optimizations
        bool supportsNVIDIAExtensions = false;        // NEW: NVIDIA-specific extensions
        bool supportsAMDExtensions = false;           // NEW: AMD-specific extensions  
        bool supportsIntelExtensions = false;         // NEW: Intel-specific extensions

        // ====================================================================
        // Display and Output Capabilities
        // ====================================================================

        // NEW: Display and presentation features
        bool supportsTearing = false;                 // NEW: Variable refresh rate/tearing support
        bool supportsHDR10 = false;                   // NEW: HDR10 output support
        bool supportsHDR10Plus = false;               // NEW: HDR10+ support
        bool supportsDolbyVision = false;             // NEW: Dolby Vision support
        std::vector<uint32_t> supportedColorSpaces;   // NEW: Supported color spaces (DXGI_COLOR_SPACE_TYPE)
        float maxDisplayLuminance = 80.0f;            // NEW: Maximum display luminance (nits)
        float minDisplayLuminance = 0.1f;             // NEW: Minimum display luminance (nits)

        // ====================================================================
        // Helper Methods for Capability Checking
        // ====================================================================

        /// Check if a specific raytracing tier is supported
        bool SupportsRaytracingTier(uint32_t tier) const {
            return supportsRayTracing && raytracingTier >= tier;
        }

        /// Check if a specific variable rate shading tier is supported
        bool SupportsVRSTier(uint32_t tier) const {
            return supportsVariableRateShading && variableRateShadingTier >= tier;
        }

        /// Check if a specific mesh shader tier is supported
        bool SupportsMeshShaderTier(uint32_t tier) const {
            return supportsMeshShaders && meshShaderTier >= tier;
        }

        /// Check if a specific shader model is supported
        bool SupportsShaderModel(uint32_t major, uint32_t minor) const {
            uint32_t requestedSM = (major << 8) | minor; // e.g., 6.6 -> 0x0606
            return highestShaderModel >= requestedSM;
        }

        /// Check if a specific sample count is supported for MSAA
        bool SupportsSampleCount(uint32_t sampleCount) const {
            return std::find(supportedSampleCounts.begin(), supportedSampleCounts.end(), sampleCount)
                != supportedSampleCounts.end();
        }

        /// Get maximum supported sample count for MSAA
        uint32_t GetMaxSampleCount() const {
            if (supportedSampleCounts.empty()) return 1;
            return *std::max_element(supportedSampleCounts.begin(), supportedSampleCounts.end());
        }

        /// Check if sufficient memory is available for allocation
        bool HasSufficientMemory(uint64_t requiredBytes) const {
            return availableVideoMemory >= requiredBytes;
        }

        /// Get memory pressure level (0.0 = no pressure, 1.0 = fully utilized)
        float GetMemoryPressure() const {
            if (dedicatedVideoMemory == 0) return 0.0f;
            return static_cast<float>(currentUsage) / static_cast<float>(dedicatedVideoMemory);
        }

        /// Check if this is a high-end discrete GPU
        bool IsHighEndGPU() const {
            return isDiscrete &&
                dedicatedVideoMemory >= (4ULL * 1024 * 1024 * 1024) && // >= 4 GB VRAM
                supportsRayTracing &&
                SupportsShaderModel(6, 6);
        }

        /// Check if this is a low-power/mobile GPU
        bool IsLowPowerGPU() const {
            return isUMA ||
                dedicatedVideoMemory < (2ULL * 1024 * 1024 * 1024) || // < 2 GB VRAM
                (vendorId == 0x8086); // Intel integrated graphics
        }

        /// Get vendor name from vendor ID
        std::string GetVendorName() const {
            switch (vendorId) {
            case 0x10DE: return "NVIDIA";
            case 0x1002: return "AMD";
            case 0x8086: return "Intel";
            case 0x1414: return "Microsoft"; // WARP
            default: return "Unknown";
            }
        }

        /// Get a human-readable capability summary
        std::string GetCapabilitySummary() const {
            std::string summary = GetVendorName() + " " + adapterName;
            summary += std::format(" ({:.1f} GB VRAM)",
                static_cast<double>(dedicatedVideoMemory) / (1024.0 * 1024.0 * 1024.0));

            if (supportsRayTracing) {
                summary += ", RT Tier " + std::to_string(raytracingTier);
            }
            if (supportsVariableRateShading) {
                summary += ", VRS Tier " + std::to_string(variableRateShadingTier);
            }
            if (supportsMeshShaders) {
                summary += ", MS Tier " + std::to_string(meshShaderTier);
            }
            if (supportsDirectML) {
                summary += ", DirectML";
            }

            return summary;
        }

        /// Update capabilities from D3D12 device (similar to AdapterInfo::UpdateFromD3D12Capabilities)
        void UpdateFromD3D12Device(ID3D12Device* device) {
            if (!device) return;

            // Query feature levels
            D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels = {};
            D3D_FEATURE_LEVEL levels[] = {
                D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0,
                D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0
            };
            featureLevels.NumFeatureLevels = _countof(levels);
            featureLevels.pFeatureLevelsRequested = levels;

            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof(featureLevels)))) {
                maxFeatureLevel = static_cast<uint32_t>(featureLevels.MaxSupportedFeatureLevel);
            }

            // Query raytracing support  
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)))) {
                supportsRayTracing = (options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0);
                raytracingTier = static_cast<uint32_t>(options5.RaytracingTier);
            }

            // Query variable rate shading
            D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6)))) {
                supportsVariableRateShading = (options6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1);
                variableRateShadingTier = static_cast<uint32_t>(options6.VariableShadingRateTier);
            }

            // Query mesh shader support
            D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7)))) {
                supportsMeshShaders = (options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1);
                meshShaderTier = static_cast<uint32_t>(options7.MeshShaderTier);
            }

            // Query architecture
            D3D12_FEATURE_DATA_ARCHITECTURE architecture = {};
            architecture.NodeIndex = 0;
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &architecture, sizeof(architecture)))) {
                isTileBasedRenderer = architecture.TileBasedRenderer;
                isUMA = architecture.UMA;
                isCacheCoherentUMA = architecture.CacheCoherentUMA;
            }

            // Query resource binding tier
            D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options)))) {
                resourceBindingTier = static_cast<uint32_t>(options.ResourceBindingTier);
            }

            // Query shader model support
            D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_7 };
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))) {
                highestShaderModel = static_cast<uint32_t>(shaderModel.HighestShaderModel);
                supportsShaderModel6 = (shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_0);
                supportsShaderModel6_6 = (shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_6);
                supportsShaderModel6_7 = (shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_7);
            }

            // Additional capability queries can be added here as needed
        }

        // ====================================================================
        // Initialization and Default Values
        // ====================================================================

        /// Set reasonable default values for development/testing
        void SetDefaults() {
            // Basic limits
            maxTexture2DSize = 16384;
            maxTexture3DSize = 2048;
            maxTextureCubeSize = 16384;
            maxTextureArraySize = 2048;
            maxRenderTargets = 8;
            maxViewports = 16;
            maxVertexInputElements = 32;
            maxConstantBufferSize = 65536;

            // Default sample counts
            supportedSampleCounts = { 1, 2, 4, 8 };

            // Performance defaults
            recommendedSwapChainBuffers = 3;
            minResourceAlignment = 256;

            // Assume modern DirectX 12 baseline
            maxFeatureLevel = static_cast<uint32_t>(D3D_FEATURE_LEVEL_12_0);
            supportsShaderModel6 = true;
            highestShaderModel = static_cast<uint32_t>(D3D_SHADER_MODEL_6_0);
        }
    };

} // namespace Akhanda::RHI
