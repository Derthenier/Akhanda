// RHI.Interface.ixx
// Akhanda Game Engine - Render Hardware Interface Module
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>
#include <string>
#include <type_traits>

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

        constexpr ResourceHandle() = default;
        constexpr ResourceHandle(uint32_t idx, uint32_t gen) : index(idx), generation(gen) {}

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
        // Feature levels
        uint32_t maxFeatureLevel = 0;

        bool supportsShaderModel6 = false;
        bool supportsRayTracing = false;
        bool supportsVariableRateShading = false;
        bool supportsMeshShaders = false;
        bool supportsDirectStorageGPU = false;

        bool isUMA = false; // Unified Memory Architecture
        bool isCacheCoherentUMA = false;
        bool isTileBasedRenderer = false;

        uint32_t raytracingTier = 0;
        uint32_t variableRateShadingTier = 0;
        uint32_t meshShaderTier = 0;
        uint32_t resourceBindingTier = 0;

        // Resource limits
        uint32_t maxTexture2DSize = 0;
        uint32_t maxTexture3DSize = 0;
        uint32_t maxTextureCubeSize = 0;
        uint32_t maxTextureArraySize = 0;
        uint32_t maxRenderTargets = 0;
        uint32_t maxVertexInputElements = 0;
        uint32_t maxConstantBufferSize = 0;

        // Memory information
        uint64_t dedicatedVideoMemory = 0;
        uint64_t sharedSystemMemory = 0;
        uint64_t availableVideoMemory = 0;

        // Multi-sampling
        std::vector<uint32_t> supportedSampleCounts;

        // Debug features
        bool debugLayerAvailable = false;
        bool gpuValidationAvailable = false;

        // Adapter information
        std::string adapterDescription;
        uint32_t vendorId = 0;
        uint32_t deviceId = 0;
        uint32_t subSysId = 0;
        uint32_t revision = 0;
        uint64_t driverVersion = 0;
    };

} // namespace Akhanda::RHI
