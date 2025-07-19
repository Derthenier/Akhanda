// D3D12Pipeline.hpp
// Akhanda Game Engine - D3D12 Pipeline Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Core.Logging;
import Akhanda.Core.Memory;

using Microsoft::WRL::ComPtr;

namespace Akhanda::RHI::D3D12 {

    // Forward declarations
    class D3D12Device;

    // ========================================================================
    // Shader Reflection Data
    // ========================================================================

    struct ShaderReflection {
        struct Parameter {
            std::string name;
            uint32_t bindPoint = 0;
            uint32_t space = 0;
            uint32_t count = 1;
            D3D_SHADER_INPUT_TYPE type = D3D_SIT_CBUFFER;
        };

        struct ConstantBuffer {
            std::string name;
            uint32_t bindPoint = 0;
            uint32_t space = 0;
            uint32_t size = 0;
            std::vector<D3D12_SHADER_VARIABLE_DESC> variables;
        };

        std::vector<Parameter> cbvParameters;
        std::vector<Parameter> srvParameters;
        std::vector<Parameter> uavParameters;
        std::vector<Parameter> samplerParameters;
        std::vector<ConstantBuffer> constantBuffers;

        D3D12_SHADER_VERSION_TYPE shaderType = D3D12_SHVER_VERTEX_SHADER;
        uint32_t inputParameterCount = 0;
        uint32_t outputParameterCount = 0;
        uint32_t instructionCount = 0;
        uint32_t tempRegisterCount = 0;
    };

    // ========================================================================
    // Root Signature Builder
    // ========================================================================

    class RootSignatureBuilder {
    private:
        std::vector<D3D12_ROOT_PARAMETER1> rootParameters_;
        std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers_;
        std::vector<D3D12_DESCRIPTOR_RANGE1> descriptorRanges_;
        D3D12_ROOT_SIGNATURE_FLAGS flags_ = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    public:
        RootSignatureBuilder();
        ~RootSignatureBuilder();

        // Parameter addition
        uint32_t AddConstantBufferView(uint32_t shaderRegister, uint32_t space = 0,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
        uint32_t AddShaderResourceView(uint32_t shaderRegister, uint32_t space = 0,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
        uint32_t AddUnorderedAccessView(uint32_t shaderRegister, uint32_t space = 0,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
        uint32_t Add32BitConstants(uint32_t shaderRegister, uint32_t num32BitValues, uint32_t space = 0,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

        // Descriptor table
        uint32_t BeginDescriptorTable(D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
        void AddDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t numDescriptors,
            uint32_t baseShaderRegister, uint32_t space = 0);
        void EndDescriptorTable();

        // Static samplers
        void AddStaticSampler(uint32_t shaderRegister, const D3D12_SAMPLER_DESC& samplerDesc,
            uint32_t space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

        // Flags
        void SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags) { flags_ = flags; }
        void AddFlags(D3D12_ROOT_SIGNATURE_FLAGS flags) { flags_ |= flags; }

        // Build
        ComPtr<ID3D12RootSignature> Build(ID3D12Device* device, const char* debugName = nullptr);

        // Reset
        void Reset();

    private:
        uint32_t currentDescriptorTableIndex_ = UINT32_MAX;
        std::vector<D3D12_DESCRIPTOR_RANGE1> currentTableRanges_;
    };

    // ========================================================================
    // D3D12 Pipeline Implementation
    // ========================================================================

    class D3D12Pipeline : public IRenderPipeline {
    private:
        // Core pipeline objects
        ComPtr<ID3D12PipelineState> pipelineState_;
        ComPtr<ID3D12RootSignature> rootSignature_;

        // Pipeline type
        bool isGraphicsPipeline_ = false;
        bool isComputePipeline_ = false;

        // Pipeline descriptors
        GraphicsPipelineDesc graphicsDesc_;
        ComputePipelineDesc computeDesc_;
        RootSignatureDesc rootSignatureDesc_;

        // Compiled shaders
        ComPtr<ID3DBlob> vertexShader_;
        ComPtr<ID3DBlob> pixelShader_;
        ComPtr<ID3DBlob> geometryShader_;
        ComPtr<ID3DBlob> hullShader_;
        ComPtr<ID3DBlob> domainShader_;
        ComPtr<ID3DBlob> computeShader_;

        // Shader reflection
        std::unordered_map<std::string, ShaderReflection> shaderReflections_;

        // Input layout
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements_;

        // Render state
        D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12GraphicsDesc_ = {};
        D3D12_COMPUTE_PIPELINE_STATE_DESC d3d12ComputeDesc_ = {};

        // Debug information
        std::string debugName_;

        // Validity
        std::atomic<bool> isValid_{ false };

        // Logging
        Logging::LogChannel& logChannel_;

    public:
        D3D12Pipeline();
        virtual ~D3D12Pipeline();

        // IRenderPipeline implementation
        bool Initialize(const GraphicsPipelineDesc& desc) override;
        bool Initialize(const ComputePipelineDesc& desc) override;
        void Shutdown() override;

        bool IsGraphicsPipeline() const override { return isGraphicsPipeline_; }
        bool IsComputePipeline() const override { return isComputePipeline_; }
        const RootSignatureDesc& GetRootSignature() const override { return rootSignatureDesc_; }

        void SetDebugName(const char* name) override;
        const char* GetDebugName() const override { return debugName_.c_str(); }

        // D3D12-specific methods
        bool InitializeWithDevice(const GraphicsPipelineDesc& desc, ID3D12Device* device);
        bool InitializeWithDevice(const ComputePipelineDesc& desc, ID3D12Device* device);

        ID3D12PipelineState* GetD3D12PipelineState() const { return pipelineState_.Get(); }
        ID3D12RootSignature* GetD3D12RootSignature() const { return rootSignature_.Get(); }

        // Shader access
        const ShaderReflection* GetShaderReflection(const std::string& shaderName) const;
        bool HasShaderReflection(const std::string& shaderName) const;

        // Pipeline state queries
        const GraphicsPipelineDesc& GetGraphicsDesc() const { return graphicsDesc_; }
        const ComputePipelineDesc& GetComputeDesc() const { return computeDesc_; }

        // Input layout
        const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetInputElements() const { return inputElements_; }

        // Validation
        bool IsValid() const { return isValid_.load(); }

    private:
        // Graphics pipeline creation
        bool CreateGraphicsPipeline(const GraphicsPipelineDesc& desc, ID3D12Device* device);
        bool CompileGraphicsShaders(const GraphicsPipelineDesc& desc);
        bool CreateGraphicsRootSignature(ID3D12Device* device);
        bool CreateGraphicsPipelineState(ID3D12Device* device);

        // Compute pipeline creation
        bool CreateComputePipeline(const ComputePipelineDesc& desc, ID3D12Device* device);
        bool CompileComputeShader(const ComputePipelineDesc& desc);
        bool CreateComputeRootSignature(ID3D12Device* device);
        bool CreateComputePipelineState(ID3D12Device* device);

        // Shader compilation
        ComPtr<ID3DBlob> CompileShader(const ShaderDesc& shaderDesc, const char* target);
        bool ReflectShader(ID3DBlob* shaderBlob, const char* shaderName);

        // Root signature creation
        ComPtr<ID3D12RootSignature> CreateRootSignatureFromReflection(ID3D12Device* device);
        void AddShaderParametersToRootSignature(const ShaderReflection& reflection, RootSignatureBuilder& builder);

        // Input layout conversion
        bool ConvertInputLayout(const VertexInputLayout& inputLayout);
        D3D12_INPUT_ELEMENT_DESC ConvertVertexElement(const VertexElement& element);

        // Render state conversion
        void ConvertBlendState(const BlendDesc& blendDesc, D3D12_BLEND_DESC& d3d12BlendDesc);
        void ConvertRasterizerState(const RasterizerDesc& rasterizerDesc, D3D12_RASTERIZER_DESC& d3d12RasterizerDesc);
        void ConvertDepthStencilState(const DepthStencilStateDesc& depthStencilDesc, D3D12_DEPTH_STENCIL_DESC& d3d12DepthStencilDesc);

        // Format conversion
        DXGI_FORMAT ConvertFormat(Format format);
        D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertPrimitiveTopologyType(PrimitiveTopology topology);

        // Validation helpers
        bool ValidateGraphicsPipelineDesc(const GraphicsPipelineDesc& desc);
        bool ValidateComputePipelineDesc(const ComputePipelineDesc& desc);
        bool ValidateShaderDesc(const ShaderDesc& desc);

        // Debug helpers
        void LogPipelineInfo();
        void LogShaderReflection(const ShaderReflection& reflection, const char* shaderName);

        // Cleanup helpers
        void CleanupResources();
        void CleanupShaders();
    };

    // ========================================================================
    // Pipeline Utilities
    // ========================================================================

    namespace PipelineUtils {

        // Shader compilation utilities
        ComPtr<ID3DBlob> CompileShaderFromFile(const std::string& filename, const char* entryPoint,
            const char* target, const D3D_SHADER_MACRO* defines = nullptr);
        ComPtr<ID3DBlob> CompileShaderFromSource(const std::string& source, const char* entryPoint,
            const char* target, const D3D_SHADER_MACRO* defines = nullptr);

        // Shader reflection utilities
        ShaderReflection ReflectShader(ID3DBlob* shaderBlob);
        bool GetShaderInputSignature(ID3DBlob* shaderBlob, std::vector<D3D12_SIGNATURE_PARAMETER_DESC>& inputParams);
        bool GetShaderOutputSignature(ID3DBlob* shaderBlob, std::vector<D3D12_SIGNATURE_PARAMETER_DESC>& outputParams);

        // Input layout utilities
        std::vector<D3D12_INPUT_ELEMENT_DESC> CreateInputLayoutFromReflection(const ShaderReflection& vertexShader);
        bool ValidateInputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout);
        uint32_t CalculateInputLayoutStride(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout, uint32_t inputSlot);

        // Format conversion
        DXGI_FORMAT ConvertFormat(Format format);
        Format ConvertDXGIFormat(DXGI_FORMAT format);
        const char* GetShaderTarget(D3D12_SHADER_VERSION_TYPE shaderType);

        // Blend state utilities
        D3D12_BLEND ConvertBlendFactor(uint32_t blendFactor);
        D3D12_BLEND_OP ConvertBlendOp(uint32_t blendOp);
        D3D12_LOGIC_OP ConvertLogicOp(uint32_t logicOp);

        // Rasterizer state utilities
        D3D12_FILL_MODE ConvertFillMode(uint32_t fillMode);
        D3D12_CULL_MODE ConvertCullMode(uint32_t cullMode);

        // Depth stencil state utilities
        D3D12_COMPARISON_FUNC ConvertComparisonFunc(CompareFunction func);
        D3D12_STENCIL_OP ConvertStencilOp(uint32_t stencilOp);

        // Primitive topology utilities
        D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertPrimitiveTopologyType(PrimitiveTopology topology);
        D3D_PRIMITIVE_TOPOLOGY ConvertPrimitiveTopology(PrimitiveTopology topology);

        // Validation utilities
        bool IsValidShaderTarget(const char* target);
        bool IsValidEntryPoint(const char* entryPoint);
        bool IsValidInputElement(const D3D12_INPUT_ELEMENT_DESC& element);

        // Debug utilities
        const char* ShaderTypeToString(D3D12_SHADER_VERSION_TYPE type);
        const char* PrimitiveTopologyToString(PrimitiveTopology topology);
        const char* FormatToString(Format format);

        // Pipeline state caching
        std::string CreatePipelineStateHash(const GraphicsPipelineDesc& desc);
        std::string CreatePipelineStateHash(const ComputePipelineDesc& desc);

    } // namespace PipelineUtils

    // ========================================================================
    // Pipeline Cache
    // ========================================================================

    class D3D12PipelineCache {
    private:
        struct CacheEntry {
            std::unique_ptr<D3D12Pipeline> pipeline;
            std::string hash;
            uint64_t lastUsed = 0;
            uint32_t useCount = 0;
        };

        std::unordered_map<std::string, std::unique_ptr<CacheEntry>> cache_;
        std::mutex cacheMutex_;

        // Cache configuration
        uint32_t maxCacheSize_ = 1000;
        uint64_t maxCacheMemory_ = 100 * 1024 * 1024; // 100MB

        // Statistics
        std::atomic<uint32_t> cacheHits_{ 0 };
        std::atomic<uint32_t> cacheMisses_{ 0 };
        std::atomic<uint64_t> totalMemory_{ 0 };

        Logging::LogChannel& logChannel_;

    public:
        D3D12PipelineCache();
        ~D3D12PipelineCache();

        bool Initialize();
        void Shutdown();

        // Pipeline caching
        D3D12Pipeline* GetPipeline(const GraphicsPipelineDesc& desc, ID3D12Device* device);
        D3D12Pipeline* GetPipeline(const ComputePipelineDesc& desc, ID3D12Device* device);
        void AddPipeline(const std::string& hash, std::unique_ptr<D3D12Pipeline> pipeline);

        // Cache management
        void TrimCache();
        void FlushCache();
        void SaveCacheToDisk(const std::string& filename);
        bool LoadCacheFromDisk(const std::string& filename, ID3D12Device* device);

        // Statistics
        uint32_t GetCacheHits() const { return cacheHits_.load(); }
        uint32_t GetCacheMisses() const { return cacheMisses_.load(); }
        float GetCacheHitRatio() const;
        uint32_t GetCacheSize() const;
        uint64_t GetCacheMemory() const { return totalMemory_.load(); }

        // Configuration
        void SetMaxCacheSize(uint32_t size) { maxCacheSize_ = size; }
        void SetMaxCacheMemory(uint64_t memory) { maxCacheMemory_ = memory; }

    private:
        // Cache helpers
        CacheEntry* FindEntry(const std::string& hash);
        void UpdateCacheStatistics(bool hit);
        void EvictLeastRecentlyUsed();

        // Serialization
        bool SerializePipeline(const D3D12Pipeline* pipeline, std::vector<uint8_t>& data);
        std::unique_ptr<D3D12Pipeline> DeserializePipeline(const std::vector<uint8_t>& data, ID3D12Device* device);
    };

    // ========================================================================
    // Pipeline Manager
    // ========================================================================

    class D3D12PipelineManager {
    private:
        D3D12Device* device_ = nullptr;
        std::unique_ptr<D3D12PipelineCache> cache_;

        // Shader include handler
        class ShaderIncludeHandler : public ID3DInclude {
        private:
            std::vector<std::string> includePaths_;

        public:
            void AddIncludePath(const std::string& path);

            HRESULT Open(D3D_INCLUDE_TYPE includeType, LPCSTR fileName, LPCVOID parentData, LPCVOID* data, UINT* bytes) override;
            HRESULT Close(LPCVOID data) override;
        };

        std::unique_ptr<ShaderIncludeHandler> includeHandler_;

        // Statistics
        std::atomic<uint32_t> totalPipelines_{ 0 };
        std::atomic<uint32_t> activePipelines_{ 0 };

        Logging::LogChannel& logChannel_;

    public:
        D3D12PipelineManager();
        ~D3D12PipelineManager();

        bool Initialize(D3D12Device* device);
        void Shutdown();

        // Pipeline creation
        std::unique_ptr<D3D12Pipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc);
        std::unique_ptr<D3D12Pipeline> CreateComputePipeline(const ComputePipelineDesc& desc);

        // Shader include paths
        void AddShaderIncludePath(const std::string& path);
        void ClearShaderIncludePaths();

        // Cache management
        D3D12PipelineCache* GetCache() { return cache_.get(); }
        void FlushCache() { if (cache_) cache_->FlushCache(); }

        // Statistics
        uint32_t GetTotalPipelines() const { return totalPipelines_.load(); }
        uint32_t GetActivePipelines() const { return activePipelines_.load(); }

    private:
        // Pipeline creation helpers
        std::unique_ptr<D3D12Pipeline> CreatePipelineInternal(const GraphicsPipelineDesc& desc);
        std::unique_ptr<D3D12Pipeline> CreatePipelineInternal(const ComputePipelineDesc& desc);
    };

} // namespace Akhanda::RHI::D3D12
