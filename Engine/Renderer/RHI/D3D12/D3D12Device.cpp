// Engine/Renderer/RHI/D3D12/D3D12Device.cpp
// Akhanda Game Engine - D3D12 Device Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#include "D3D12Core.hpp"
#include "D3D12Device.hpp"
#include "D3D12Buffer.hpp"
#include "D3D12Texture.hpp"
#include "D3D12Pipeline.hpp"
#include "D3D12SwapChain.hpp"
#include "D3D12CommandList.hpp"
#include "D3D12DescriptorHeap.hpp"

#include "Core/Logging/Core.Logging.hpp"

#include <chrono>
#include <dxgidebug.h>
#include <comdef.h>
#include <d3d12sdklayers.h>

// D3D12 Agility SDK
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 616; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

import Akhanda.Engine.RHI;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Platform.Interfaces;
import Akhanda.Platform.Windows;
import Akhanda.Platform.RendererIntegration;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Core.Logging;
import Akhanda.Core.Memory;

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

using namespace Akhanda::RHI;

using namespace Akhanda::Logging;

namespace Akhanda::RHI::D3D12 {

namespace {

    // ============================================================================
    // Helper Macros and Utilities
    // ============================================================================

    // Error checking macro
#define CHECK_HR(hr, msg, channel) \
    if (FAILED(hr)) { \
        _com_error err(hr); \
        std::string errorMsg = ""; \
        if (err.ErrorMessage()) { \
            std::wstring wstr(err.ErrorMessage()); \
            errorMsg = Platform::Windows::WideToUtf8(wstr); \
        } \
        channel.LogFormat(Akhanda::Logging::LogLevel::Error, "{}: {} (HRESULT: 0x{:08X})", \
            msg, errorMsg, static_cast<uint32_t>(hr)); \
        return false; \
    }

#define CHECK_HR_VOID(hr, msg, channel) \
    if (FAILED(hr)) { \
        _com_error err(hr); \
        std::string errorMsg = ""; \
        if (err.ErrorMessage()) { \
            std::wstring wstr(err.ErrorMessage()); \
            errorMsg = Platform::Windows::WideToUtf8(wstr); \
        } \
        channel.LogFormat(Akhanda::Logging::LogLevel::Error, "{}: {} (HRESULT: 0x{:08X})", \
            msg, errorMsg, static_cast<uint32_t>(hr)); \
        return; \
    }

    // Helper function to get feature level string
    const char* GetFeatureLevelString(D3D_FEATURE_LEVEL level) {
        switch (level) {
        case D3D_FEATURE_LEVEL_12_2: return "12.2";
        case D3D_FEATURE_LEVEL_12_1: return "12.1";
        case D3D_FEATURE_LEVEL_12_0: return "12.0";
        case D3D_FEATURE_LEVEL_11_1: return "11.1";
        case D3D_FEATURE_LEVEL_11_0: return "11.0";
        default: return "Unknown";
        }
    }

    // Helper function to get adapter type string
    const char* GetAdapterTypeString(const DXGI_ADAPTER_DESC1& desc) {
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) return "Software";
        if (desc.VendorId == 0x1002) return "AMD";
        if (desc.VendorId == 0x10DE) return "NVIDIA";
        if (desc.VendorId == 0x8086) return "Intel";
        return "Unknown";
    }
}

    // ============================================================================
    // D3D12CommandQueue Implementation
    // ============================================================================

    D3D12CommandQueue::D3D12CommandQueue(CommandListType type)
        : type_(type)
        , logChannel_(LogManager::Instance().GetChannel("D3D12CommandQueue"))
        , nextFenceValue_(1) {
    }

    D3D12CommandQueue::~D3D12CommandQueue() {
        Shutdown();
    }

    bool D3D12CommandQueue::Initialize(ID3D12Device* device) {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};

        switch (type_) {
        case CommandListType::Direct:
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            break;
        case CommandListType::Compute:
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            break;
        case CommandListType::Copy:
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            break;
        }

        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;

        HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_));
        CHECK_HR(hr, "Failed to create command queue", logChannel_);

        // Create fence
        hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
        CHECK_HR(hr, "Failed to create fence", logChannel_);

        // Create fence event
        fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!fenceEvent_) {
            logChannel_.Log(LogLevel::Error, "Failed to create fence event");
            return false;
        }

        // Create command allocator pool
        for (int i = 0; i < 3; ++i) {
            ComPtr<ID3D12CommandAllocator> allocator;
            hr = device->CreateCommandAllocator(queueDesc.Type, IID_PPV_ARGS(&allocator));
            CHECK_HR(hr, "Failed to create command allocator", logChannel_);
            availableAllocators_.push(allocator);
        }

        return true;
    }

    void D3D12CommandQueue::Shutdown() {
        WaitForIdle();

        if (fenceEvent_) {
            CloseHandle(fenceEvent_);
            fenceEvent_ = nullptr;
        }

        while (!availableAllocators_.empty()) {
            availableAllocators_.pop();
        }
        while (!inFlightAllocators_.empty()) {
            inFlightAllocators_.pop();
        }

        fence_.Reset();
        commandQueue_.Reset();
    }

    uint64_t D3D12CommandQueue::ExecuteCommandLists(uint32_t count, ID3D12CommandList* const* commandLists) {
        if (count == 0 || !commandLists) return 0;

        commandQueue_->ExecuteCommandLists(count, commandLists);

        // Signal fence
        uint64_t fenceValue = nextFenceValue_++;
        HRESULT hr = commandQueue_->Signal(fence_.Get(), fenceValue);

        if (FAILED(hr)) {
            logChannel_.LogFormat(LogLevel::Error, "Failed to signal fence: {}", hr);
            return 0;
        }

        return fenceValue;
    }

    void D3D12CommandQueue::WaitForFenceValue(uint64_t fenceValue) {
        if (fence_->GetCompletedValue() >= fenceValue) {
            return;
        }

        HRESULT hr = fence_->SetEventOnCompletion(fenceValue, fenceEvent_);
        if (FAILED(hr)) {
            logChannel_.LogFormat(LogLevel::Error, "Failed to set fence event: {}", hr);
            return;
        }

        WaitForSingleObject(fenceEvent_, INFINITE);
    }

    void D3D12CommandQueue::WaitForIdle() {
        uint64_t fenceValue = Signal();
        WaitForFenceValue(fenceValue);
    }

    uint64_t D3D12CommandQueue::Signal() {
        uint64_t fenceValue = nextFenceValue_++;
        HRESULT hr = commandQueue_->Signal(fence_.Get(), fenceValue);
        if (FAILED(hr)) {
            logChannel_.LogFormat(LogLevel::Error, "Failed to signal fence: {}", hr);
            return 0;
        }
        return fenceValue;
    }

    ComPtr<ID3D12CommandAllocator> D3D12CommandQueue::GetCommandAllocator() {
        // Reclaim completed allocators
        ProcessCompletedAllocators();

        if (availableAllocators_.empty()) {
            // Create new allocator if none available
            ComPtr<ID3D12CommandAllocator> allocator;
            D3D12_COMMAND_LIST_TYPE type = (type_ == CommandListType::Direct) ? D3D12_COMMAND_LIST_TYPE_DIRECT :
                (type_ == CommandListType::Compute) ? D3D12_COMMAND_LIST_TYPE_COMPUTE :
                D3D12_COMMAND_LIST_TYPE_COPY;

            // This would need a device reference - for now return nullptr
            logChannel_.LogFormat(LogLevel::Warning, "No available command allocators, need to create new one. Type = {}", static_cast<uint32_t>(type));
            return nullptr;
        }

        ComPtr<ID3D12CommandAllocator> allocator = availableAllocators_.front();
        availableAllocators_.pop();

        HRESULT hr = allocator->Reset();
        if (FAILED(hr)) {
            logChannel_.LogFormat(LogLevel::Error, "Failed to reset command allocator: {}", hr);
            return nullptr;
        }

        return allocator;
    }

    void D3D12CommandQueue::ReturnCommandAllocator(ComPtr<ID3D12CommandAllocator> allocator, uint64_t fenceValue) {
        inFlightAllocators_.emplace(allocator, fenceValue);
    }

    void D3D12CommandQueue::ProcessCompletedAllocators() {
        uint64_t completedValue = fence_->GetCompletedValue();

        while (!inFlightAllocators_.empty()) {
            auto& front = inFlightAllocators_.front();
            if (front.second <= completedValue) {
                availableAllocators_.push(front.first);
                inFlightAllocators_.pop();
            }
            else {
                break;
            }
        }
    }

    void D3D12CommandQueue::SetDebugName(const char* name) {
        if (name) {
            debugName_ = name;
            std::wstring wname(debugName_.begin(), debugName_.end());
            if (commandQueue_) {
                commandQueue_->SetName(wname.c_str());
            }
            if (fence_) {
                std::wstring fenceName = wname + L"_Fence";
                fence_->SetName(fenceName.c_str());
            }
        }
    }


    // ============================================================================
    // D3D12ResourceManager Implementation
    // ============================================================================

    bool D3D12ResourceManager::Initialize(ID3D12Device* device, IDXGIAdapter* adapter) {
        // Create D3D12MemAlloc allocator
        D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
        allocatorDesc.pDevice = device;
        allocatorDesc.pAdapter = adapter;
        allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED; // We handle threading

        HRESULT hr = D3D12MA::CreateAllocator(&allocatorDesc, &allocator_);
        if (FAILED(hr)) {
            return false;
        }

        // Pre-allocate resource containers
        buffers_.reserve(1000);
        textures_.reserve(1000);
        pipelines_.reserve(100);

        return true;
    }

    void D3D12ResourceManager::Shutdown() {
        // Clean up all resources
        buffers_.clear();
        textures_.clear();
        pipelines_.clear();

        // Clear free indices
        std::queue<uint32_t> empty;
        freeBufferIndices_.swap(empty);
        freeTextureIndices_.swap(empty);
        freePipelineIndices_.swap(empty);

        // Release allocator
        if (allocator_) {
            allocator_->Release();
            allocator_ = nullptr;
        }
    }

    BufferHandle D3D12ResourceManager::CreateBuffer(const BufferDesc& desc, [[maybe_unused]] ID3D12Device* device) {
        std::lock_guard<std::mutex> lock(resourceMutex_);

        uint32_t index;
        if (!freeBufferIndices_.empty()) {
            index = freeBufferIndices_.front();
            freeBufferIndices_.pop();
        }
        else {
            index = static_cast<uint32_t>(buffers_.size());
            buffers_.emplace_back();
        }

        auto& entry = buffers_[index];
        entry.generation = nextGeneration_.fetch_add(1);
        entry.isValid = true;
        entry.debugName = desc.debugName ? desc.debugName : "";

        // Create D3D12 resource
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = desc.size;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // Set resource flags based on usage
        if ((desc.usage & ResourceUsage::UnorderedAccess) != ResourceUsage::None) {
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        // Determine heap type
        D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST;

        if (desc.cpuAccessible) {
            heapType = D3D12_HEAP_TYPE_UPLOAD;
            initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        }

        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = heapType;

        HRESULT hr = allocator_->CreateResource(
            &allocationDesc,
            &resourceDesc,
            initialState,
            nullptr,
            &entry.allocation,
            IID_PPV_ARGS(&entry.resource));

        if (FAILED(hr)) {
            entry.isValid = false;
            freeBufferIndices_.push(index);
            return BufferHandle{};
        }

        entry.currentState = ResourceState::CopyDestination;
        if (desc.cpuAccessible) {
            entry.currentState = ResourceState::Common;
        }

        // Set debug name
        if (!entry.debugName.empty()) {
            std::wstring wname(entry.debugName.begin(), entry.debugName.end());
            entry.resource->SetName(wname.c_str());
        }

        return BufferHandle{ index, entry.generation };
    }

    void D3D12ResourceManager::DestroyBuffer(BufferHandle handle) {
        std::lock_guard<std::mutex> lock(resourceMutex_);

        if (!IsValidHandle(handle)) {
            return;
        }

        auto& entry = buffers_[handle.index];
        entry.isValid = false;
        entry.resource.Reset();
        if (entry.allocation) {
            entry.allocation->Release();
            entry.allocation = nullptr;
        }
        entry.debugName.clear();

        freeBufferIndices_.push(handle.index);
    }

    D3D12Buffer* D3D12ResourceManager::GetBuffer(BufferHandle handle) {
        if (!IsValidHandle(handle)) {
            return nullptr;
        }
        // This would return a wrapper object in the full implementation
        return nullptr; // Placeholder
    }

    TextureHandle D3D12ResourceManager::CreateTexture(const TextureDesc& desc, [[maybe_unused]] ID3D12Device* device) {
        std::lock_guard<std::mutex> lock(resourceMutex_);

        uint32_t index;
        if (!freeTextureIndices_.empty()) {
            index = freeTextureIndices_.front();
            freeTextureIndices_.pop();
        }
        else {
            index = static_cast<uint32_t>(textures_.size());
            textures_.emplace_back();
        }

        auto& entry = textures_[index];
        entry.generation = nextGeneration_.fetch_add(1);
        entry.isValid = true;
        entry.debugName = desc.debugName ? desc.debugName : "";

        // Create D3D12 resource description
        D3D12_RESOURCE_DESC resourceDesc = {};

        switch (desc.type) {
        case ResourceType::Texture1D:
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            break;
        case ResourceType::Texture2D:
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;
        case ResourceType::Texture3D:
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            break;
        case ResourceType::TextureCube:
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;
        }

        resourceDesc.Width = desc.width;
        resourceDesc.Height = desc.height;
        resourceDesc.DepthOrArraySize = static_cast<uint16_t>((desc.type == ResourceType::Texture3D) ? desc.depth : desc.arraySize);
        resourceDesc.MipLevels = static_cast<uint16_t>(desc.mipLevels);
        resourceDesc.Format = ConvertFormat(desc.format);
        resourceDesc.SampleDesc.Count = desc.sampleCount;
        resourceDesc.SampleDesc.Quality = desc.sampleQuality;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // Set resource flags based on usage
        if ((desc.usage & ResourceUsage::RenderTarget) != ResourceUsage::None) {
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }
        if ((desc.usage & ResourceUsage::DepthStencil) != ResourceUsage::None) {
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }
        if ((desc.usage & ResourceUsage::UnorderedAccess) != ResourceUsage::None) {
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_CLEAR_VALUE* clearValue = nullptr;
        D3D12_CLEAR_VALUE clearValueData = {};

        if ((desc.usage & (ResourceUsage::RenderTarget | ResourceUsage::DepthStencil)) != ResourceUsage::None) {
            clearValueData.Format = resourceDesc.Format;
            if ((desc.usage & ResourceUsage::RenderTarget) != ResourceUsage::None) {
                clearValueData.Color[0] = 0.0f;
                clearValueData.Color[1] = 0.0f;
                clearValueData.Color[2] = 0.0f;
                clearValueData.Color[3] = 1.0f;
            }
            else {
                clearValueData.DepthStencil.Depth = 1.0f;
                clearValueData.DepthStencil.Stencil = 0;
            }
            clearValue = &clearValueData;
        }

        HRESULT hr = allocator_->CreateResource(
            &allocationDesc,
            &resourceDesc,
            ConvertResourceState(desc.initialState),
            clearValue,
            &entry.allocation,
            IID_PPV_ARGS(&entry.resource));

        if (FAILED(hr)) {
            entry.isValid = false;
            freeTextureIndices_.push(index);
            return TextureHandle{};
        }

        entry.currentState = desc.initialState;

        // Set debug name
        if (!entry.debugName.empty()) {
            std::wstring wname(entry.debugName.begin(), entry.debugName.end());
            entry.resource->SetName(wname.c_str());
        }

        return TextureHandle{ index, entry.generation };
    }

    void D3D12ResourceManager::DestroyTexture(TextureHandle handle) {
        std::lock_guard<std::mutex> lock(resourceMutex_);

        if (!IsValidHandle(handle)) {
            return;
        }

        auto& entry = textures_[handle.index];
        entry.isValid = false;
        entry.resource.Reset();
        if (entry.allocation) {
            entry.allocation->Release();
            entry.allocation = nullptr;
        }
        entry.debugName.clear();

        freeTextureIndices_.push(handle.index);
    }

    D3D12Texture* D3D12ResourceManager::GetTexture(TextureHandle handle) {
        if (!IsValidHandle(handle)) {
            return nullptr;
        }
        // This would return a wrapper object in the full implementation
        return nullptr; // Placeholder
    }

    bool D3D12ResourceManager::IsValidHandle(BufferHandle handle) const {
        if (handle.index >= buffers_.size()) {
            return false;
        }
        const auto& entry = buffers_[handle.index];
        return entry.isValid && entry.generation == handle.generation;
    }

    bool D3D12ResourceManager::IsValidHandle(TextureHandle handle) const {
        if (handle.index >= textures_.size()) {
            return false;
        }
        const auto& entry = textures_[handle.index];
        return entry.isValid && entry.generation == handle.generation;
    }

    bool D3D12ResourceManager::IsValidHandle(PipelineHandle handle) const {
        if (handle.index >= pipelines_.size()) {
            return false;
        }
        const auto& entry = pipelines_[handle.index];
        return entry->IsValid();
    }

    uint64_t D3D12ResourceManager::GetUsedMemory() const {
        if (!allocator_) {
            return 0;
        }

        D3D12MA::Budget localBudget, nonLocalBudget;
        allocator_->GetBudget(&localBudget, &nonLocalBudget);
        return localBudget.UsageBytes + nonLocalBudget.UsageBytes;
    }

    uint64_t D3D12ResourceManager::GetTotalMemory() const {
        if (!allocator_) {
            return 0;
        }

        D3D12MA::Budget localBudget, nonLocalBudget;
        allocator_->GetBudget(&localBudget, &nonLocalBudget);
        return localBudget.BudgetBytes + nonLocalBudget.BudgetBytes;
    }

    // ============================================================================
    // D3D12Device Implementation
    // ============================================================================

    D3D12Device::D3D12Device()
        : logChannel_(LogManager::Instance().GetChannel("D3D12Device")) {
        frameNumber_.store(0);
    }

    D3D12Device::~D3D12Device() {
        Shutdown();
    }

    bool D3D12Device::Initialize(const Configuration::RenderingConfig& config,
        const Platform::SurfaceInfo& surfaceInfo) {
        LOG_INFO(logChannel_, "Initializing D3D12 device...");

        config_ = config;
        const auto* rendererSurfaceInfo = static_cast<const Platform::RendererSurfaceInfo*>(&surfaceInfo);
        if (rendererSurfaceInfo) {
            surfaceInfo_ = *rendererSurfaceInfo;
        }

        // Initialize debug layer first
        if (config.GetDebugLayerEnabled() && !InitializeDebugLayer()) {
            LOG_WARNING(logChannel_, "Failed to initialize debug layer, continuing without debug support");
        }

        // Initialize core device
        if (!InitializeDevice()) {
            LOG_ERROR(logChannel_, "Failed to initialize D3D12 device");
            return false;
        }

        // Initialize command queues
        if (!InitializeCommandQueues()) {
            LOG_ERROR(logChannel_, "Failed to initialize command queues");
            return false;
        }

        // Initialize descriptor heaps
        if (!InitializeDescriptorHeaps()) {
            LOG_ERROR(logChannel_, "Failed to initialize descriptor heaps");
            return false;
        }

        // Initialize resource manager
        if (!InitializeResourceManager()) {
            LOG_ERROR(logChannel_, "Failed to initialize resource manager");
            return false;
        }

        // Initialize swap chain
        if (!InitializeSwapChain()) {
            LOG_ERROR(logChannel_, "Failed to initialize swap chain");
            return false;
        }

        // Detect and log device capabilities
        DetectCapabilities();
        LogDeviceInfo();

        isInitialized_.store(true);

        LOG_INFO(logChannel_, "D3D12 device initialized successfully");
        return true;
    }

    void D3D12Device::Shutdown() {
        if (!isInitialized_.load()) {
            return;
        }

        LOG_INFO(logChannel_, "Shutting down D3D12 device...");

        // Wait for all GPU work to complete
        WaitForIdle();

        // Clean up in reverse order
        CleanupResources();
        CleanupDebugLayer();

        isInitialized_.store(false);

        LOG_INFO(logChannel_, "D3D12 device shutdown complete");
    }

    void D3D12Device::WaitForIdle() {
        if (graphicsQueue_) graphicsQueue_->WaitForIdle();
        if (computeQueue_) computeQueue_->WaitForIdle();
        if (copyQueue_) copyQueue_->WaitForIdle();
    }

    void D3D12Device::BeginFrame() {
        frameNumber_.fetch_add(1);
        ResetFrameStats();

        // Process debug messages
        if (debugLayerEnabled_) {
            ProcessDebugMessages();
        }
    }

    void D3D12Device::EndFrame() {
        UpdateRenderStats();
    }

    void D3D12Device::Present() {
        if (swapChain_) {
            swapChain_->Present(config_.GetVsync());
        }
    }

    bool D3D12Device::InitializeDebugLayer() {
        debugLayerEnabled_ = config_.GetDebugLayerEnabled();
        gpuValidationEnabled_ = config_.GetGpuValidationEnabled();

        if (!debugLayerEnabled_) {
            return true;
        }

        // Enable D3D12 debug layer
        HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12Debug_));
        if (SUCCEEDED(hr)) {
            d3d12Debug_->EnableDebugLayer();

            // Try to get newer debug interfaces
            hr = d3d12Debug_.As(&d3d12Debug1_);
            if (SUCCEEDED(hr) && gpuValidationEnabled_) {
                d3d12Debug1_->SetEnableGPUBasedValidation(TRUE);
                LOG_INFO(logChannel_, "GPU-based validation enabled");
            }

            hr = d3d12Debug_.As(&d3d12Debug5_);
            if (SUCCEEDED(hr)) {
                d3d12Debug5_->SetEnableAutoName(TRUE);
                LOG_INFO(logChannel_, "Debug auto-naming enabled");
            }

            LOG_INFO(logChannel_, "D3D12 debug layer enabled");
        }
        else {
            LOG_WARNING(logChannel_, "Failed to enable D3D12 debug layer");
            debugLayerEnabled_ = false;
            return false;
        }

        // Enable DXGI debug layer
        typedef HRESULT(WINAPI* DXGIGetDebugInterface1Proc)(UINT, REFIID, void**);
        HMODULE dxgiDebugModule = LoadLibraryEx(L"dxgidebug.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (dxgiDebugModule) {
            auto dxgiGetDebugInterface1 = reinterpret_cast<DXGIGetDebugInterface1Proc>(
                GetProcAddress(dxgiDebugModule, "DXGIGetDebugInterface1"));

            if (dxgiGetDebugInterface1) {
                ComPtr<IDXGIDebug1> dxgiDebug;
                hr = dxgiGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
                if (SUCCEEDED(hr)) {
                    dxgiDebug->EnableLeakTrackingForThread();
                    LOG_INFO(logChannel_, "DXGI debug layer enabled");
                }
            }
        }

        return true;
    }

    bool D3D12Device::InitializeDevice() {
        // Create DXGI factory
        UINT factoryFlags = 0;
        if (debugLayerEnabled_) {
            factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }

        HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&dxgiFactory_));
        CHECK_HR(hr, "Failed to create DXGI factory", logChannel_);

        // Enumerate adapters and find the best one
        ComPtr<IDXGIAdapter1> adapter1;
        ComPtr<IDXGIAdapter4> bestAdapter;
        SIZE_T maxVideoMemory = 0;

        for (UINT i = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory_->EnumAdapters1(i, &adapter1); ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter1->GetDesc1(&desc);

            // Skip software adapters
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            // Test D3D12 compatibility
            if (SUCCEEDED(D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
                if (desc.DedicatedVideoMemory > maxVideoMemory) {
                    maxVideoMemory = desc.DedicatedVideoMemory;
                    adapter1.As(&bestAdapter);
                }
            }
        }

        if (!bestAdapter) {
            LOG_ERROR(logChannel_, "No compatible D3D12 adapter found");
            return false;
        }

        adapter_ = bestAdapter;

        // Create D3D12 device
        hr = D3D12CreateDevice(adapter_.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_));
        CHECK_HR(hr, "Failed to create D3D12 device", logChannel_);

        // Set device name
        device_->SetName(L"Akhanda D3D12 Device");

        // Initialize debug device for validation
        if (debugLayerEnabled_) {
            hr = device_.As(&debugDevice_);
            if (SUCCEEDED(hr)) {
                hr = device_.As(&infoQueue_);
                if (SUCCEEDED(hr)) {
                    // Configure debug message filtering
                    D3D12_MESSAGE_SEVERITY severities[] = {
                        D3D12_MESSAGE_SEVERITY_INFO
                    };

                    D3D12_MESSAGE_ID denyIds[] = {
                        D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                        D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                        D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                    };

                    D3D12_INFO_QUEUE_FILTER filter = {};
                    filter.DenyList.NumSeverities = _countof(severities);
                    filter.DenyList.pSeverityList = severities;
                    filter.DenyList.NumIDs = _countof(denyIds);
                    filter.DenyList.pIDList = denyIds;

                    infoQueue_->PushStorageFilter(&filter);
                    infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                    infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
                }
            }
        }

        return true;
    }

    bool D3D12Device::InitializeCommandQueues() {
        // Create graphics queue
        graphicsQueue_ = std::make_unique<D3D12CommandQueue>(CommandListType::Direct);
        if (!graphicsQueue_->Initialize(device_.Get())) {
            LOG_ERROR(logChannel_, "Failed to initialize graphics command queue");
            return false;
        }
        graphicsQueue_->SetDebugName("GraphicsQueue");

        // Create compute queue
        computeQueue_ = std::make_unique<D3D12CommandQueue>(CommandListType::Compute);
        if (!computeQueue_->Initialize(device_.Get())) {
            LOG_ERROR(logChannel_, "Failed to initialize compute command queue");
            return false;
        }
        computeQueue_->SetDebugName("ComputeQueue");

        // Create copy queue
        copyQueue_ = std::make_unique<D3D12CommandQueue>(CommandListType::Copy);
        if (!copyQueue_->Initialize(device_.Get())) {
            LOG_ERROR(logChannel_, "Failed to initialize copy command queue");
            return false;
        }
        copyQueue_->SetDebugName("CopyQueue");

        return true;
    }

    bool D3D12Device::InitializeDescriptorHeaps() {
        // Create RTV heap
        rtvHeap_ = std::make_unique<D3D12DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
        if (!rtvHeap_->InitializeWithDevice(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256, false)) {
            LOG_ERROR(logChannel_, "Failed to create RTV descriptor heap");
            return false;
        }

        // Create DSV heap
        dsvHeap_ = std::make_unique<D3D12DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);
        if (!dsvHeap_->InitializeWithDevice(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256, false)) {
            LOG_ERROR(logChannel_, "Failed to create DSV descriptor heap");
            return false;
        }

        // Create SRV heap (shader visible)
        srvHeap_ = std::make_unique<D3D12DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
        if (!srvHeap_->InitializeWithDevice(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16384, true)) {
            LOG_ERROR(logChannel_, "Failed to create SRV descriptor heap");
            return false;
        }

        // Create sampler heap (shader visible)
        samplerHeap_ = std::make_unique<D3D12DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, true);
        if (!samplerHeap_->InitializeWithDevice(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048, true)) {
            LOG_ERROR(logChannel_, "Failed to create sampler descriptor heap");
            return false;
        }

        return true;
    }

    bool D3D12Device::InitializeResourceManager() {
        // TODO: Implement D3D12ResourceManager
        // For now, just log that it needs implementation
        LOG_INFO(logChannel_, "Resource manager initialization - TODO: Implement D3D12ResourceManager");
        return true;
    }

    bool D3D12Device::InitializeSwapChain() {
        // TODO: Implement D3D12SwapChain
        // For now, just log that it needs implementation
        LOG_INFO(logChannel_, "Swap chain initialization - TODO: Implement D3D12SwapChain");
        return true;
    }

    void D3D12Device::DetectCapabilities() {
        if (!device_) return;

        // Query basic feature levels
        D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels = {};
        D3D_FEATURE_LEVEL levels[] = {
            D3D_FEATURE_LEVEL_12_2,
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };
        featureLevels.NumFeatureLevels = _countof(levels);
        featureLevels.pFeatureLevelsRequested = levels;

        HRESULT hr = device_->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof(featureLevels));
        if (SUCCEEDED(hr)) {
            deviceCapabilities_.maxFeatureLevel = featureLevels.MaxSupportedFeatureLevel;
        }

        // Query raytracing support
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        hr = device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
        if (SUCCEEDED(hr)) {
            deviceCapabilities_.supportsRayTracing = (options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED);
            deviceCapabilities_.raytracingTier = static_cast<uint32_t>(options5.RaytracingTier);
        }

        // Query variable rate shading support
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
        hr = device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6));
        if (SUCCEEDED(hr)) {
            deviceCapabilities_.supportsVariableRateShading = (options6.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED);
            deviceCapabilities_.variableRateShadingTier = static_cast<uint32_t>(options6.VariableShadingRateTier);
        }

        // Query mesh shader support
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
        hr = device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));
        if (SUCCEEDED(hr)) {
            deviceCapabilities_.supportsMeshShaders = (options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED);
            deviceCapabilities_.meshShaderTier = static_cast<uint32_t>(options7.MeshShaderTier);
        }

        // Query DirectStorage support
        D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12 = {};
        hr = device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12));
        if (SUCCEEDED(hr)) {
            deviceCapabilities_.supportsDirectStorageGPU = options12.MSPrimitivesPipelineStatisticIncludesCulledPrimitives;
        }

        // Query architecture info
        D3D12_FEATURE_DATA_ARCHITECTURE architecture = {};
        architecture.NodeIndex = 0;
        hr = device_->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &architecture, sizeof(architecture));
        if (SUCCEEDED(hr)) {
            deviceCapabilities_.isTileBasedRenderer = architecture.TileBasedRenderer;
            deviceCapabilities_.isUMA = architecture.UMA;
            deviceCapabilities_.isCacheCoherentUMA = architecture.CacheCoherentUMA;
        }

        // Query resource binding tier
        D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
        hr = device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
        if (SUCCEEDED(hr)) {
            deviceCapabilities_.resourceBindingTier = static_cast<uint32_t>(options.ResourceBindingTier);
        }
    }

    void D3D12Device::LogDeviceInfo() {
        if (!adapter_) return;

        DXGI_ADAPTER_DESC1 desc;
        adapter_->GetDesc1(&desc);

        LOG_INFO(logChannel_, "=== D3D12 Device Information ===");

        // Convert wide string to narrow string for logging
        std::wstring wstr(desc.Description);
        std::string description = Platform::Windows::WideToUtf8(wstr);

        logChannel_.LogFormat(LogLevel::Info, "Adapter: {}", description);
        logChannel_.LogFormat(LogLevel::Info, "Video Memory: {} MB", desc.DedicatedVideoMemory / (1024 * 1024));
        logChannel_.LogFormat(LogLevel::Info, "System Memory: {} MB", desc.DedicatedSystemMemory / (1024 * 1024));
        logChannel_.LogFormat(LogLevel::Info, "Shared Memory: {} MB", desc.SharedSystemMemory / (1024 * 1024));

        // Log capabilities
        logChannel_.LogFormat(LogLevel::Info, "Max Feature Level: {}", static_cast<uint32_t>(deviceCapabilities_.maxFeatureLevel));
        logChannel_.LogFormat(LogLevel::Info, "Raytracing Support: {} (Tier {})",
            deviceCapabilities_.supportsRayTracing ? "Yes" : "No", deviceCapabilities_.raytracingTier);
        logChannel_.LogFormat(LogLevel::Info, "Variable Rate Shading: {} (Tier {})",
            deviceCapabilities_.supportsVariableRateShading ? "Yes" : "No", deviceCapabilities_.variableRateShadingTier);
        logChannel_.LogFormat(LogLevel::Info, "Mesh Shaders: {} (Tier {})",
            deviceCapabilities_.supportsMeshShaders ? "Yes" : "No", deviceCapabilities_.meshShaderTier);
        logChannel_.LogFormat(LogLevel::Info, "DirectStorage: {}",
            deviceCapabilities_.supportsDirectStorageGPU ? "Yes" : "No");
        logChannel_.LogFormat(LogLevel::Info, "Resource Binding Tier: {}", deviceCapabilities_.resourceBindingTier);
        logChannel_.LogFormat(LogLevel::Info, "Is UMA: {}", deviceCapabilities_.isUMA ? "Yes" : "No");
        logChannel_.LogFormat(LogLevel::Info, "Is Tile Based: {}", deviceCapabilities_.isTileBasedRenderer ? "Yes" : "No");
    }

    void D3D12Device::ProcessDebugMessages() {
        if (!infoQueue_ || !debugLayerEnabled_) return;

        uint64_t messageCount = infoQueue_->GetNumStoredMessages();
        for (uint64_t i = 0; i < messageCount; ++i) {
            SIZE_T messageLength = 0;
            infoQueue_->GetMessage(i, nullptr, &messageLength);

            if (messageLength > 0) {
                std::vector<uint8_t> messageData(messageLength);
                D3D12_MESSAGE* message = reinterpret_cast<D3D12_MESSAGE*>(messageData.data());

                HRESULT hr = infoQueue_->GetMessage(i, message, &messageLength);
                if (SUCCEEDED(hr)) {
                    DebugMessage debugMsg;
                    debugMsg.severity = ConvertD3D12Severity(message->Severity);
                    debugMsg.category = std::to_string(static_cast<uint32_t>(message->Category));
                    debugMsg.id = static_cast<uint32_t>(message->ID);
                    debugMsg.message = std::string(message->pDescription, message->DescriptionByteLength);
                    debugMsg.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

                    debugMessages_.push_back(debugMsg);

                    // Log critical messages immediately
                    if (debugMsg.severity == DebugMessage::Severity::Error ||
                        debugMsg.severity == DebugMessage::Severity::Corruption) {
                        logChannel_.LogFormat(LogLevel::Error, "D3D12 Debug: {}", debugMsg.message);
                    }
                }
            }
        }

        infoQueue_->ClearStoredMessages();
    }

    DebugMessage::Severity D3D12Device::ConvertD3D12Severity(D3D12_MESSAGE_SEVERITY severity) {
        switch (severity) {
        case D3D12_MESSAGE_SEVERITY_CORRUPTION: return DebugMessage::Severity::Corruption;
        case D3D12_MESSAGE_SEVERITY_ERROR: return DebugMessage::Severity::Error;
        case D3D12_MESSAGE_SEVERITY_WARNING: return DebugMessage::Severity::Warning;
        case D3D12_MESSAGE_SEVERITY_INFO: return DebugMessage::Severity::Info;
        case D3D12_MESSAGE_SEVERITY_MESSAGE: return DebugMessage::Severity::Info;
        default: return DebugMessage::Severity::Info;
        }
    }

    void D3D12Device::UpdateRenderStats() {
        // Update memory usage
        renderStats_.memoryUsage = resourceManager_ ? resourceManager_->GetUsedMemory() : 0;
        renderStats_.videoMemoryUsage = resourceManager_ ? resourceManager_->GetTotalMemory() : 0;
    }

    void D3D12Device::ResetFrameStats() {
        renderStats_.drawCalls = 0;
    }

    void D3D12Device::CleanupResources() {
        swapChain_.reset();
        resourceManager_.reset();

        samplerHeap_.reset();
        srvHeap_.reset();
        dsvHeap_.reset();
        rtvHeap_.reset();

        copyQueue_.reset();
        computeQueue_.reset();
        graphicsQueue_.reset();

        device_.Reset();
        adapter_.Reset();
        dxgiFactory_.Reset();
    }

    void D3D12Device::CleanupDebugLayer() {
        if (debugLayerEnabled_) {
            debugMessages_.clear();
            infoQueue_.Reset();
            debugDevice_.Reset();
            d3d12Debug5_.Reset();
            d3d12Debug1_.Reset();
            d3d12Debug_.Reset();
        }
    }

    // ============================================================================
    // IRenderDevice Interface Implementation
    // ============================================================================

    BufferHandle D3D12Device::CreateBuffer(const BufferDesc& desc) {
        if (!ValidateBufferDesc(desc)) return {};
        if (!resourceManager_) return {};
        return resourceManager_->CreateBuffer(desc, device_.Get());
    }

    TextureHandle D3D12Device::CreateTexture(const TextureDesc& desc) {
        if (!ValidateTextureDesc(desc)) return {};
        if (!resourceManager_) return {};
        return resourceManager_->CreateTexture(desc, device_.Get());
    }

    PipelineHandle D3D12Device::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) {
        if (!ValidatePipelineDesc(desc)) return {};

        auto pipeline = std::make_unique<D3D12Pipeline>();
        if (!pipeline->InitializeWithDevice(desc, device_.Get())) {
            return {};
        }

        // TODO: Store pipeline in pipeline manager and return handle
        return {};
    }

    PipelineHandle D3D12Device::CreateComputePipeline(const ComputePipelineDesc& desc) {
        if (!ValidatePipelineDesc(desc)) return {};

        auto pipeline = std::make_unique<D3D12Pipeline>();
        if (!pipeline->InitializeWithDevice(desc, device_.Get())) {
            return {};
        }

        // TODO: Store pipeline in pipeline manager and return handle
        return {};
    }

    void D3D12Device::DestroyBuffer(BufferHandle handle) {
        if (resourceManager_) {
            resourceManager_->DestroyBuffer(handle);
        }
    }

    void D3D12Device::DestroyTexture(TextureHandle handle) {
        if (resourceManager_) {
            resourceManager_->DestroyTexture(handle);
        }
    }

    void D3D12Device::DestroyPipeline([[maybe_unused]] PipelineHandle handle) {
        // TODO: Implement pipeline destruction
    }

    IRenderBuffer* D3D12Device::GetBuffer(BufferHandle handle) {
        return resourceManager_ ? resourceManager_->GetBuffer(handle) : nullptr;
    }

    IRenderTexture* D3D12Device::GetTexture(TextureHandle handle) {
        return resourceManager_ ? resourceManager_->GetTexture(handle) : nullptr;
    }

    IRenderPipeline* D3D12Device::GetPipeline([[maybe_unused]] PipelineHandle handle) {
        // TODO: Implement pipeline retrieval
        return nullptr;
    }

    std::unique_ptr<ICommandList> D3D12Device::CreateCommandList(CommandListType type) {
        auto commandList = std::make_unique<D3D12CommandList>(type);
        if (!commandList->Initialize(type)) {
            return nullptr;
        }
        return std::move(commandList);
    }

    uint64_t D3D12Device::ExecuteCommandLists(CommandListType queueType, uint32_t count, ICommandList* const* commandLists) {
        D3D12CommandQueue* queue = nullptr;
        switch (queueType) {
        case CommandListType::Direct: queue = graphicsQueue_.get(); break;
        case CommandListType::Compute: queue = computeQueue_.get(); break;
        case CommandListType::Copy: queue = copyQueue_.get(); break;
        }

        if (!queue || count == 0 || !commandLists) return 0;

        // Convert to D3D12 command lists
        std::vector<ID3D12CommandList*> d3d12CommandLists;
        d3d12CommandLists.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            if (auto* d3d12CmdList = static_cast<D3D12CommandList*>(commandLists[i])) {
                d3d12CommandLists.push_back(d3d12CmdList->GetD3D12CommandList());
            }
        }

        return queue->ExecuteCommandLists(static_cast<uint32_t>(d3d12CommandLists.size()), d3d12CommandLists.data());
    }

    void D3D12Device::WaitForFence(uint64_t fenceValue, CommandListType queueType) {
        D3D12CommandQueue* queue = nullptr;
        switch (queueType) {
        case CommandListType::Direct: queue = graphicsQueue_.get(); break;
        case CommandListType::Compute: queue = computeQueue_.get(); break;
        case CommandListType::Copy: queue = copyQueue_.get(); break;
        }

        if (queue) {
            queue->WaitForFenceValue(fenceValue);
        }
    }

    const DeviceCapabilities& D3D12Device::GetCapabilities() const {
        return deviceCapabilities_;
    }

    const RenderStats& D3D12Device::GetStats() const {
        return renderStats_;
    }

    std::vector<DebugMessage> D3D12Device::GetDebugMessages() {
        std::vector<DebugMessage> messages;
        messages.swap(debugMessages_);
        return messages;
    }

    void D3D12Device::SetDebugName(const char* name) {
        if (name && device_) {
            debugName_ = name;
            std::wstring wname(debugName_.begin(), debugName_.end());
            device_->SetName(wname.c_str());
        }
    }

    const char* D3D12Device::GetDebugName() const {
        return debugName_.c_str();
    }

    // ============================================================================
    // Validation Helpers
    // ============================================================================

    bool D3D12Device::ValidateBufferDesc(const BufferDesc& desc) const {
        if (desc.size == 0) {
            logChannel_.Log(LogLevel::Error, "Buffer size cannot be zero");
            return false;
        }

        if (desc.size > deviceCapabilities_.maxConstantBufferSize) {
            logChannel_.LogFormat(LogLevel::Error, "Buffer size {} exceeds maximum {}",
                desc.size, deviceCapabilities_.maxConstantBufferSize);
            return false;
        }

        return true;
    }

    bool D3D12Device::ValidateTextureDesc(const TextureDesc& desc) const {
        if (desc.width == 0 || desc.height == 0) {
            logChannel_.Log(LogLevel::Error, "Texture dimensions cannot be zero");
            return false;
        }

        if (desc.width > deviceCapabilities_.maxTexture2DSize || desc.height > deviceCapabilities_.maxTexture2DSize) {
            logChannel_.LogFormat(LogLevel::Error, "Texture size {}x{} exceeds maximum {}x{}",
                desc.width, desc.height, deviceCapabilities_.maxTexture2DSize, deviceCapabilities_.maxTexture2DSize);
            return false;
        }

        return true;
    }

    bool D3D12Device::ValidatePipelineDesc(const GraphicsPipelineDesc& desc) const {
        if (!desc.vertexShader.bytecode) {
            logChannel_.Log(LogLevel::Error, "Graphics pipeline requires vertex shader");
            return false;
        }

        return true;
    }

    bool D3D12Device::ValidatePipelineDesc(const ComputePipelineDesc& desc) const {
        if (!desc.computeShader.bytecode) {
            logChannel_.Log(LogLevel::Error, "Compute pipeline requires compute shader");
            return false;
        }

        return true;
    }

    std::unique_ptr<IDescriptorHeap> D3D12Device::CreateDescriptorHeap([[maybe_unused]] uint32_t descriptorCount, [[maybe_unused]] bool shaderVisible) {
        // Implementation from the artifact
        return {};
    }

    ISwapChain* D3D12Device::GetSwapChain() {
        return swapChain_.get();
    }

    void D3D12Device::BeginDebugEvent([[maybe_unused]] const char* name) {
        // Implementation from the artifact
    }

    void D3D12Device::EndDebugEvent() {
        // Implementation from the artifact  
    }

    void D3D12Device::InsertDebugMarker([[maybe_unused]] const char* name) {
        // Implementation from the artifact
    }

    uint64_t D3D12Device::GetUsedVideoMemory() const {
        return resourceManager_ ? resourceManager_->GetUsedMemory() : 0;
    }

    uint64_t D3D12Device::GetAvailableVideoMemory() const {
        return resourceManager_ ? resourceManager_->GetTotalMemory() - resourceManager_->GetUsedMemory() : 0;
    }

    void D3D12Device::FlushMemory() {
        // Implementation from the artifact
    }

    // ============================================================================
    // D3D12Factory Implementation
    // ============================================================================

    D3D12Factory::D3D12Factory()
        : logChannel_(LogManager::Instance().GetChannel("D3D12Factory")) {
        Initialize();
    }

    D3D12Factory::~D3D12Factory() {
        Shutdown();
    }

    std::unique_ptr<IRenderDevice> D3D12Factory::CreateDevice(
        const Configuration::RenderingConfig& config,
        const Platform::SurfaceInfo& surfaceInfo) {

        auto device = std::make_unique<D3D12Device>();
        if (!device->Initialize(config, surfaceInfo)) {
            logChannel_.Log(Akhanda::Logging::LogLevel::Error, "Failed to initialize D3D12 device");
            return nullptr;
        }
        return std::unique_ptr<IRenderDevice>(device.release());
    }

    bool D3D12Factory::IsSupported() const {
        ComPtr<IDXGIFactory7> factory;
        CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));

        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter); ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
                return true;
            }
        }

        return false;
    }

    std::vector<std::string> D3D12Factory::GetSupportedDevices() const {
        std::vector<std::string> devices;

        for (const auto& adapter : adapters_) {
            std::string name = Platform::Windows::WideToUtf8(adapter.description);
            devices.push_back(name);
        }

        return devices;
    }

    DeviceCapabilities D3D12Factory::GetDeviceCapabilities(uint32_t deviceIndex) const {
        if (deviceIndex >= adapters_.size()) {
            return {};
        }

        return GetAdapterCapabilities(adapters_[deviceIndex]);
    }

    void D3D12Factory::SetDebugName(const char* name) {
        if (name) {
            debugName_ = name;
        }
    }

    const char* D3D12Factory::GetDebugName() const {
        return debugName_.c_str();
    }

    bool D3D12Factory::Initialize() {
        HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory_));
        if (FAILED(hr)) {
            LOG_ERROR(logChannel_, "Failed to create DXGI factory");
            return false;
        }

        EnumerateAdapters();

        platformIntegration_ = Platform::CreateRendererPlatformIntegration();

        return true;
    }

    void D3D12Factory::Shutdown() {
        adapters_.clear();
        platformIntegration_.reset();
        dxgiFactory_.Reset();
    }

    void D3D12Factory::EnumerateAdapters() {
        adapters_.clear();

        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory_->EnumAdapters1(i, &adapter); ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
                Platform::AdapterInfo info;
                info.description = desc.Description;
                info.vendorId = desc.VendorId;
                info.deviceId = desc.DeviceId;
                info.dedicatedVideoMemory = desc.DedicatedVideoMemory;
                info.dedicatedSystemMemory = desc.DedicatedSystemMemory;
                info.sharedSystemMemory = desc.SharedSystemMemory;

                adapters_.push_back(info);
            }
        }

        LOG_INFO_FORMAT(logChannel_, "Found {} compatible D3D12 adapters", adapters_.size());
    }

    DeviceCapabilities D3D12Factory::GetAdapterCapabilities([[maybe_unused]] const Platform::AdapterInfo& adapter) const {

        // Create temporary device to query capabilities
        ComPtr<IDXGIAdapter1> dxgiAdapter;
        if (FAILED(dxgiFactory_->EnumAdapters1(0, &dxgiAdapter))) {
            return {};
        }

        ComPtr<ID3D12Device> device;
        if (FAILED(D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) {
            return {};
        }

        DeviceCapabilities caps;

        // Query capabilities using the temporary device
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)))) {
            caps.supportsRayTracing = (options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED);
            caps.raytracingTier = static_cast<uint32_t>(options5.RaytracingTier);
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6)))) {
            caps.supportsVariableRateShading = (options6.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED);
            caps.variableRateShadingTier = static_cast<uint32_t>(options6.VariableShadingRateTier);
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7)))) {
            caps.supportsMeshShaders = (options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED);
            caps.meshShaderTier = static_cast<uint32_t>(options7.MeshShaderTier);
        }

        // Set reasonable limits
        caps.maxConstantBufferSize = 1024 * 1024 * 1024; // 1GB
        caps.maxTexture2DSize = 16384;
        caps.maxTexture2DSize = 1000000; // D3D12 supports essentially unlimited

        return caps;
    }
}