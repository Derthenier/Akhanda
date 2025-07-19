// D3D12Texture.hpp
// Akhanda Game Engine - D3D12 Texture Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <D3D12MemAlloc.h>
#include <memory>
#include <mutex>
#include <atomic>

import Akhanda.Engine.RHI;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Core.Logging;
import Akhanda.Core.Memory;
import std;

using Microsoft::WRL::ComPtr;

namespace Akhanda::RHI::D3D12 {

    // Forward declarations
    class D3D12Device;
    class D3D12DescriptorHeap;

    // ========================================================================
    // D3D12 Texture Implementation
    // ========================================================================

    class D3D12Texture : public IRenderTexture {
    private:
        // Core texture objects
        ComPtr<ID3D12Resource> texture_;
        D3D12MA::Allocation* allocation_ = nullptr;

        // Texture properties
        TextureDesc desc_;
        ResourceState currentState_;
        D3D12_RESOURCE_STATES d3d12State_;

        // Descriptor handles
        struct DescriptorHandles {
            uint32_t srv = UINT32_MAX;      // Shader Resource View
            uint32_t uav = UINT32_MAX;      // Unordered Access View
            uint32_t rtv = UINT32_MAX;      // Render Target View
            uint32_t dsv = UINT32_MAX;      // Depth Stencil View
            bool hasValidHandles = false;
        };
        DescriptorHandles descriptorHandles_;

        // Subresource tracking
        std::vector<ResourceState> subresourceStates_;
        std::mutex subresourceMutex_;

        // Clear values
        D3D12_CLEAR_VALUE clearValue_ = {};
        bool hasClearValue_ = false;

        // Memory footprint
        uint64_t memorySize_ = 0;
        D3D12_RESOURCE_ALLOCATION_INFO allocationInfo_ = {};

        // Debug information
        std::string debugName_;

        // Validity
        std::atomic<bool> isValid_{ false };

        // Logging
        Logging::LogChannel& logChannel_;

    public:
        D3D12Texture();
        virtual ~D3D12Texture();

        // IRenderTexture implementation
        bool Initialize(const TextureDesc& desc) override;
        void Shutdown() override;

        void UpdateData(const void* data, uint32_t mipLevel = 0,
            uint32_t arraySlice = 0) override;

        uint32_t GetWidth() const override { return desc_.width; }
        uint32_t GetHeight() const override { return desc_.height; }
        uint32_t GetDepth() const override { return desc_.depth; }
        uint32_t GetMipLevels() const override { return desc_.mipLevels; }
        uint32_t GetArraySize() const override { return desc_.arraySize; }
        Format GetFormat() const override { return desc_.format; }
        uint32_t GetSampleCount() const override { return desc_.sampleCount; }
        uint32_t GetSampleQuality() const override { return desc_.sampleQuality; }
        ResourceUsage GetUsage() const override { return desc_.usage; }
        ResourceState GetCurrentState() const override { return currentState_; }

        // IRenderResource implementation
        uint64_t GetGPUAddress() const override { return 0; } // Textures don't have GPU addresses
        void SetDebugName(const char* name) override;
        const char* GetDebugName() const override { return debugName_.c_str(); }
        bool IsValid() const override { return isValid_.load(); }

        // D3D12-specific methods
        bool InitializeWithDevice(const TextureDesc& desc, ID3D12Device* device,
            D3D12MA::Allocator* allocator);

        ID3D12Resource* GetD3D12Resource() const { return texture_.Get(); }
        D3D12MA::Allocation* GetAllocation() const { return allocation_; }

        // Resource state management
        void SetCurrentState(ResourceState state);
        void SetSubresourceState(uint32_t subresource, ResourceState state);
        ResourceState GetSubresourceState(uint32_t subresource) const;
        D3D12_RESOURCE_STATES GetD3D12State() const { return d3d12State_; }

        // Descriptor management
        const DescriptorHandles& GetDescriptorHandles() const { return descriptorHandles_; }
        void SetDescriptorHandles(const DescriptorHandles& handles) { descriptorHandles_ = handles; }

        // Clear value
        const D3D12_CLEAR_VALUE& GetClearValue() const { return clearValue_; }
        bool HasClearValue() const { return hasClearValue_; }

        // Subresource utilities
        uint32_t GetSubresourceIndex(uint32_t mipLevel, uint32_t arraySlice, uint32_t planeSlice = 0) const;
        uint32_t GetSubresourceCount() const;

        // Memory information
        uint64_t GetMemorySize() const { return memorySize_; }
        const D3D12_RESOURCE_ALLOCATION_INFO& GetAllocationInfo() const { return allocationInfo_; }

        // Texture type queries
        bool IsRenderTarget() const { return (desc_.usage & ResourceUsage::RenderTarget) != ResourceUsage::None; }
        bool IsDepthStencil() const { return (desc_.usage & ResourceUsage::DepthStencil) != ResourceUsage::None; }
        bool IsShaderResource() const { return (desc_.usage & ResourceUsage::ShaderResource) != ResourceUsage::None; }
        bool IsUnorderedAccess() const { return (desc_.usage & ResourceUsage::UnorderedAccess) != ResourceUsage::None; }
        bool IsMultisampled() const { return desc_.sampleCount > 1; }
        bool IsArray() const { return desc_.arraySize > 1; }
        bool IsCube() const { return desc_.type == ResourceType::TextureCube; }
        bool Is3D() const { return desc_.type == ResourceType::Texture3D; }

        // Format queries
        bool IsDepthFormat() const;
        bool IsStencilFormat() const;
        bool IsCompressedFormat() const;
        bool IsSRGBFormat() const;
        uint32_t GetBytesPerPixel() const;

    private:
        // Initialization helpers
        bool CreateTexture(ID3D12Device* device, D3D12MA::Allocator* allocator);
        bool CreateDescriptorViews(ID3D12Device* device, D3D12DescriptorHeap* rtvHeap,
            D3D12DescriptorHeap* dsvHeap, D3D12DescriptorHeap* srvHeap);
        void SetupResourceDesc(D3D12_RESOURCE_DESC& resourceDesc) const;
        void SetupClearValue(D3D12_CLEAR_VALUE& clearValue) const;
        void SetupInitialResourceState(D3D12_RESOURCE_STATES& initialState) const;

        // View creation
        bool CreateShaderResourceView(ID3D12Device* device, D3D12DescriptorHeap* srvHeap);
        bool CreateUnorderedAccessView(ID3D12Device* device, D3D12DescriptorHeap* srvHeap);
        bool CreateRenderTargetView(ID3D12Device* device, D3D12DescriptorHeap* rtvHeap);
        bool CreateDepthStencilView(ID3D12Device* device, D3D12DescriptorHeap* dsvHeap);

        // Validation helpers
        bool ValidateTextureDesc(const TextureDesc& desc) const;
        bool ValidateUpdateParameters(const void* data, uint32_t mipLevel, uint32_t arraySlice) const;

        // Subresource helpers
        void InitializeSubresourceStates();
        void UpdateSubresourceStates(ResourceState newState);

        // Debug helpers
        void LogTextureInfo() const;
        void LogMemoryInfo() const;

        // Utility functions
        D3D12_RESOURCE_DIMENSION GetResourceDimension() const;
        D3D12_RESOURCE_FLAGS GetResourceFlags() const;
        DXGI_FORMAT GetDXGIFormat() const;
        DXGI_FORMAT GetTypelessFormat() const;
        DXGI_FORMAT GetSRVFormat() const;
        DXGI_FORMAT GetRTVFormat() const;
        DXGI_FORMAT GetDSVFormat() const;

        // Cleanup
        void CleanupResources();
        void CleanupDescriptorViews();
    };

    // ========================================================================
    // D3D12 Texture Utilities
    // ========================================================================

    namespace TextureUtils {

        // Texture creation helpers
        std::unique_ptr<D3D12Texture> CreateTexture2D(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint32_t width, uint32_t height,
            Format format, uint32_t mipLevels = 1,
            ResourceUsage usage = ResourceUsage::ShaderResource,
            const void* initialData = nullptr,
            const char* debugName = nullptr);

        std::unique_ptr<D3D12Texture> CreateRenderTarget(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint32_t width, uint32_t height,
            Format format, uint32_t sampleCount = 1,
            const float clearColor[4] = nullptr,
            const char* debugName = nullptr);

        std::unique_ptr<D3D12Texture> CreateDepthStencil(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint32_t width, uint32_t height,
            Format format = Format::D32_Float,
            uint32_t sampleCount = 1,
            float clearDepth = 1.0f,
            uint8_t clearStencil = 0,
            const char* debugName = nullptr);

        std::unique_ptr<D3D12Texture> CreateTextureCube(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint32_t size, Format format,
            uint32_t mipLevels = 1,
            ResourceUsage usage = ResourceUsage::ShaderResource,
            const void* initialData = nullptr,
            const char* debugName = nullptr);

        std::unique_ptr<D3D12Texture> CreateTexture3D(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint32_t width, uint32_t height, uint32_t depth,
            Format format, uint32_t mipLevels = 1,
            ResourceUsage usage = ResourceUsage::ShaderResource,
            const void* initialData = nullptr,
            const char* debugName = nullptr);

        // Texture array creation
        std::unique_ptr<D3D12Texture> CreateTextureArray(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint32_t width, uint32_t height,
            uint32_t arraySize, Format format,
            uint32_t mipLevels = 1,
            ResourceUsage usage = ResourceUsage::ShaderResource,
            const void* initialData = nullptr,
            const char* debugName = nullptr);

        // Size and alignment calculations
        uint64_t CalculateTextureSize(uint32_t width, uint32_t height, uint32_t depth,
            uint32_t mipLevels, uint32_t arraySize, Format format);
        uint64_t CalculateMipSize(uint32_t width, uint32_t height, uint32_t depth,
            uint32_t mipLevel, Format format);
        uint32_t CalculateMipLevels(uint32_t width, uint32_t height, uint32_t depth = 1);

        // Format utilities
        DXGI_FORMAT GetDXGIFormat(Format format);
        Format GetEngineFormat(DXGI_FORMAT format);
        DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT format);
        DXGI_FORMAT GetSRVFormat(DXGI_FORMAT format);
        DXGI_FORMAT GetRTVFormat(DXGI_FORMAT format);
        DXGI_FORMAT GetDSVFormat(DXGI_FORMAT format);

        uint32_t GetFormatBytesPerPixel(Format format);
        uint32_t GetBlockSize(Format format);
        bool IsBlockCompressedFormat(Format format);
        bool IsDepthFormat(Format format);
        bool IsStencilFormat(Format format);
        bool IsDepthStencilFormat(Format format);
        bool IsSRGBFormat(Format format);
        bool IsIntegerFormat(Format format);

        // Resource usage conversion
        D3D12_RESOURCE_FLAGS ConvertResourceUsageToFlags(ResourceUsage usage);
        D3D12_RESOURCE_STATES ConvertResourceUsageToState(ResourceUsage usage);

        // Resource state conversion
        D3D12_RESOURCE_STATES ConvertResourceState(ResourceState state);
        ResourceState ConvertD3D12ResourceState(D3D12_RESOURCE_STATES state);

        // Subresource utilities
        uint32_t CalcSubresource(uint32_t mipSlice, uint32_t arraySlice, uint32_t planeSlice,
            uint32_t mipLevels, uint32_t arraySize);
        void DecomposeSubresource(uint32_t subresource, uint32_t mipLevels, uint32_t arraySize,
            uint32_t& mipSlice, uint32_t& arraySlice, uint32_t& planeSlice);

        // Validation
        bool IsValidTextureSize(uint32_t width, uint32_t height, uint32_t depth = 1);
        bool IsValidMipLevels(uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels);
        bool IsValidArraySize(uint32_t arraySize, ResourceType type);
        bool IsValidSampleCount(uint32_t sampleCount);
        bool IsCompatibleUsage(ResourceUsage usage, Format format);

        // Debug utilities
        const char* ResourceTypeToString(ResourceType type);
        const char* FormatToString(Format format);
        const char* ResourceUsageToString(ResourceUsage usage);
        const char* ResourceStateToString(ResourceState state);

        // Mip generation
        struct MipGenerationDesc {
            uint32_t sourceMip = 0;
            uint32_t mipLevels = 1;
            uint32_t arraySlice = 0;
            bool generateForAllSlices = false;
        };

        bool GenerateMips(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
            D3D12Texture* texture, const MipGenerationDesc& desc);

    } // namespace TextureUtils

    // ========================================================================
    // D3D12 Texture Manager
    // ========================================================================

    class D3D12TextureManager {
    private:
        ID3D12Device* device_ = nullptr;
        D3D12MA::Allocator* allocator_ = nullptr;

        // Texture caching
        std::unordered_map<std::string, std::weak_ptr<D3D12Texture>> textureCache_;
        std::mutex cacheMutex_;

        // Common textures
        std::shared_ptr<D3D12Texture> whiteTexture_;
        std::shared_ptr<D3D12Texture> blackTexture_;
        std::shared_ptr<D3D12Texture> defaultNormalTexture_;
        std::shared_ptr<D3D12Texture> defaultSpecularTexture_;

        // Statistics
        std::atomic<uint32_t> totalTextures_{ 0 };
        std::atomic<uint64_t> totalMemory_{ 0 };

        Logging::LogChannel& logChannel_;

    public:
        D3D12TextureManager();
        ~D3D12TextureManager();

        bool Initialize(ID3D12Device* device, D3D12MA::Allocator* allocator);
        void Shutdown();

        // Texture creation
        std::shared_ptr<D3D12Texture> CreateTexture(const TextureDesc& desc,
            const char* debugName = nullptr);
        std::shared_ptr<D3D12Texture> LoadTexture(const std::string& filepath);

        // Common textures
        std::shared_ptr<D3D12Texture> GetWhiteTexture() const { return whiteTexture_; }
        std::shared_ptr<D3D12Texture> GetBlackTexture() const { return blackTexture_; }
        std::shared_ptr<D3D12Texture> GetDefaultNormalTexture() const { return defaultNormalTexture_; }
        std::shared_ptr<D3D12Texture> GetDefaultSpecularTexture() const { return defaultSpecularTexture_; }

        // Cache management
        void FlushCache();
        void TrimCache();

        // Statistics
        uint32_t GetTotalTextures() const { return totalTextures_.load(); }
        uint64_t GetTotalMemory() const { return totalMemory_.load(); }
        uint32_t GetCacheSize() const;

    private:
        // Cache helpers
        std::shared_ptr<D3D12Texture> GetFromCache(const std::string& key);
        void AddToCache(const std::string& key, std::shared_ptr<D3D12Texture> texture);

        // Common texture creation
        bool CreateCommonTextures();
        std::shared_ptr<D3D12Texture> CreateSolidColorTexture(uint32_t color, const char* debugName);

        // Cleanup
        void CleanupCommonTextures();
        void CleanupCache();
    };

} // namespace Akhanda::RHI::D3D12
