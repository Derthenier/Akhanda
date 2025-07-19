// D3D12Device.cpp
// Akhanda Game Engine - D3D12 Device Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#include "D3D12Device.hpp"
#include "D3D12Buffer.hpp"
#include "D3D12Texture.hpp"
#include "D3D12Pipeline.hpp"
#include "D3D12SwapChain.hpp"
#include "D3D12CommandList.hpp"
#include "D3D12DescriptorHeap.hpp"

#include "Core/Logging/Core.Logging.hpp"

#include <dxgidebug.h>
#include <comdef.h>


import Akhanda.Engine.RHI;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Platform.Interfaces;
import Akhanda.Platform.RendererIntegration;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Core.Logging;
import Akhanda.Core.Memory;

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

using namespace Akhanda::RHI;
using namespace Akhanda::RHI::D3D12;
using namespace Akhanda::Logging;

namespace {
    DXGI_FORMAT ConvertFormat(Format format) {
        switch (format) {
        case Format::R32G32B32A32_Float: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case Format::R32G32B32_Float: return DXGI_FORMAT_R32G32B32_FLOAT;
        case Format::R32G32_Float: return DXGI_FORMAT_R32G32_FLOAT;
        case Format::R32_Float: return DXGI_FORMAT_R32_FLOAT;
        case Format::R8G8B8A8_UNorm: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Format::B8G8R8A8_UNorm: return DXGI_FORMAT_B8G8R8A8_UNORM;
        case Format::D32_Float: return DXGI_FORMAT_D32_FLOAT;
        case Format::D24_UNorm_S8_UInt: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        default: return DXGI_FORMAT_UNKNOWN;
        }
    }

    D3D12_RESOURCE_STATES ConvertResourceState(ResourceState state) {
        switch (state) {
        case ResourceState::Common: return D3D12_RESOURCE_STATE_COMMON;
        case ResourceState::VertexAndConstantBuffer: return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case ResourceState::IndexBuffer: return D3D12_RESOURCE_STATE_INDEX_BUFFER;
        case ResourceState::RenderTarget: return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case ResourceState::UnorderedAccess: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case ResourceState::DepthWrite: return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case ResourceState::DepthRead: return D3D12_RESOURCE_STATE_DEPTH_READ;
        case ResourceState::ShaderResource: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case ResourceState::CopyDestination: return D3D12_RESOURCE_STATE_COPY_DEST;
        case ResourceState::CopySource: return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case ResourceState::Present: return D3D12_RESOURCE_STATE_PRESENT;
        default: return D3D12_RESOURCE_STATE_COMMON;
        }
    }
}

// ============================================================================
// Helper Macros and Utilities
// ============================================================================

#define CHECK_HR(hr, msg, logChannel) \
    do { \
        if (FAILED(hr)) { \
            _com_error err(hr); \
            LOG_ERROR_FORMAT(logChannel, "{}: HRESULT 0x{:08X} - {}", msg, static_cast<uint32_t>(hr), err.ErrorMessage()); \
            return false; \
        } \
    } while(0)

#define CHECK_HR_RETURN(hr, msg, retval, logChannel) \
    do { \
        if (FAILED(hr)) { \
            _com_error err(hr); \
            LOG_ERROR_FORMAT(logChannel, "{}: HRESULT 0x{:08X} - {}", msg, static_cast<uint32_t>(hr), err.ErrorMessage()); \
            return retval; \
        } \
    } while(0)

namespace {
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

BufferHandle D3D12ResourceManager::CreateBuffer(const BufferDesc& desc, ID3D12Device* device) {
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

TextureHandle D3D12ResourceManager::CreateTexture(const TextureDesc& desc, ID3D12Device* device) {
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
    resourceDesc.DepthOrArraySize = (desc.type == ResourceType::Texture3D) ? desc.depth : desc.arraySize;
    resourceDesc.MipLevels = desc.mipLevels;
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
// D3D12CommandQueue Implementation
// ============================================================================

D3D12CommandQueue::D3D12CommandQueue(D3D12_COMMAND_LIST_TYPE type)
    : type_(type), debugName_("D3D12CommandQueue") {
    fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

D3D12CommandQueue::~D3D12CommandQueue() {
    if (fenceEvent_) {
        CloseHandle(fenceEvent_);
    }
}

bool D3D12CommandQueue::Initialize(ID3D12Device* device) {
    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = type_;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_));
    if (FAILED(hr)) {
        return false;
    }

    // Create fence
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

void D3D12CommandQueue::Shutdown() {
    WaitForIdle();

    // Clean up allocators
    while (!availableAllocators_.empty()) {
        availableAllocators_.pop();
    }
    while (!inFlightAllocators_.empty()) {
        inFlightAllocators_.pop();
    }

    fence_.Reset();
    commandQueue_.Reset();
}

uint64_t D3D12CommandQueue::ExecuteCommandList(ID3D12CommandList* commandList) {
    ID3D12CommandList* commandLists[] = { commandList };
    return ExecuteCommandLists(1, commandLists);
}

uint64_t D3D12CommandQueue::ExecuteCommandLists(uint32_t count, ID3D12CommandList* const* commandLists) {
    if (count == 0 || !commandLists) {
        return GetCompletedFenceValue();
    }

    // Execute command lists
    commandQueue_->ExecuteCommandLists(count, commandLists);

    // Signal fence
    uint64_t currentFenceValue = fenceValue_.fetch_add(1) + 1;
    HRESULT hr = commandQueue_->Signal(fence_.Get(), currentFenceValue);
    if (FAILED(hr)) {
        return 0;
    }

    return currentFenceValue;
}

void D3D12CommandQueue::WaitForFence(uint64_t fenceValue) {
    if (IsFenceComplete(fenceValue)) {
        return;
    }

    HRESULT hr = fence_->SetEventOnCompletion(fenceValue, fenceEvent_);
    if (SUCCEEDED(hr)) {
        WaitForSingleObject(fenceEvent_, INFINITE);
    }
}

void D3D12CommandQueue::WaitForIdle() {
    uint64_t currentFenceValue = fenceValue_.fetch_add(1) + 1;
    HRESULT hr = commandQueue_->Signal(fence_.Get(), currentFenceValue);
    if (SUCCEEDED(hr)) {
        WaitForFence(currentFenceValue);
    }
}

bool D3D12CommandQueue::IsFenceComplete(uint64_t fenceValue) const {
    return fence_->GetCompletedValue() >= fenceValue;
}

uint64_t D3D12CommandQueue::GetCompletedFenceValue() const {
    return fence_->GetCompletedValue();
}

ComPtr<ID3D12CommandAllocator> D3D12CommandQueue::GetCommandAllocator() {
    std::lock_guard<std::mutex> lock(allocatorMutex_);

    ProcessCompletedAllocators();

    ComPtr<ID3D12CommandAllocator> allocator;
    if (!availableAllocators_.empty()) {
        allocator = availableAllocators_.front();
        availableAllocators_.pop();
        allocator->Reset();
    }

    return allocator;
}

void D3D12CommandQueue::ReturnCommandAllocator(ComPtr<ID3D12CommandAllocator> allocator, uint64_t fenceValue) {
    std::lock_guard<std::mutex> lock(allocatorMutex_);
    inFlightAllocators_.push({ allocator, fenceValue });
}

void D3D12CommandQueue::ProcessCompletedAllocators() {
    uint64_t completedValue = GetCompletedFenceValue();

    while (!inFlightAllocators_.empty()) {
        const auto& front = inFlightAllocators_.front();
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
            LOG_INFO(logChannel_, "GPU validation enabled");
        }

        hr = d3d12Debug_.As(&d3d12Debug5_);
        if (SUCCEEDED(hr) && config_.GetDebugLayerEnabled()) {
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

        // Skip software adapters unless explicitly requested
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }

        // Try to create a D3D12 device with this adapter
        ComPtr<ID3D12Device> testDevice;
        hr = D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice));

        if (SUCCEEDED(hr) && desc.DedicatedVideoMemory > maxVideoMemory) {
            maxVideoMemory = desc.DedicatedVideoMemory;
            hr = adapter1.As(&bestAdapter);
            if (FAILED(hr)) {
                continue;
            }
        }
    }

    if (!bestAdapter) {
        LOG_ERROR(logChannel_, "No suitable D3D12 adapter found");
        return false;
    }

    adapter_ = bestAdapter;

    // Log adapter information
    DXGI_ADAPTER_DESC1 adapterDesc;
    adapter_->GetDesc1(&adapterDesc);

    std::string adapterName(adapterDesc.Description, adapterDesc.Description + wcslen(adapterDesc.Description));
    LOG_INFO_FORMAT(logChannel_, "Selected adapter: {} ({})", adapterName, GetAdapterTypeString(adapterDesc));
    LOG_INFO_FORMAT(logChannel_, "Video memory: {} MB", adapterDesc.DedicatedVideoMemory / (1024 * 1024));

    // Create D3D12 device
    hr = D3D12CreateDevice(adapter_.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_));
    CHECK_HR(hr, "Failed to create D3D12 device", logChannel_);

    // Setup debug device if available
    if (debugLayerEnabled_) {
        hr = device_.As(&debugDevice_);
        if (SUCCEEDED(hr)) {
            hr = device_.As(&infoQueue_);
            if (SUCCEEDED(hr)) {
                // Filter out unwanted messages
                D3D12_MESSAGE_CATEGORY denyCategories[] = {
                    D3D12_MESSAGE_CATEGORY_STATE_GETTING
                };

                D3D12_MESSAGE_SEVERITY denySeverities[] = {
                    D3D12_MESSAGE_SEVERITY_INFO
                };

                D3D12_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumCategories = _countof(denyCategories);
                filter.DenyList.pCategoryList = denyCategories;
                filter.DenyList.NumSeverities = _countof(denySeverities);
                filter.DenyList.pSeverityList = denySeverities;

                infoQueue_->PushStorageFilter(&filter);
                infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

                LOG_INFO(logChannel_, "Debug info queue configured");
            }
        }
    }

    return true;
}

bool D3D12Device::InitializeCommandQueues() {
    // Create graphics queue
    graphicsQueue_ = std::make_unique<D3D12CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);
    if (!graphicsQueue_->Initialize(device_.Get())) {
        LOG_ERROR(logChannel_, "Failed to create graphics command queue");
        return false;
    }
    graphicsQueue_->SetDebugName("GraphicsQueue");

    // Create compute queue
    computeQueue_ = std::make_unique<D3D12CommandQueue>(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    if (!computeQueue_->Initialize(device_.Get())) {
        LOG_ERROR(logChannel_, "Failed to create compute command queue");
        return false;
    }
    computeQueue_->SetDebugName("ComputeQueue");

    // Create copy queue
    copyQueue_ = std::make_unique<D3D12CommandQueue>(D3D12_COMMAND_LIST_TYPE_COPY);
    if (!copyQueue_->Initialize(device_.Get())) {
        LOG_ERROR(logChannel_, "Failed to create copy command queue");
        return false;
    }
    copyQueue_->SetDebugName("CopyQueue");

    LOG_INFO(logChannel_, "Command queues initialized");
    return true;
}

bool D3D12Device::InitializeDescriptorHeaps() {
    // Create RTV heap
    rtvHeap_ = std::make_unique<D3D12DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
    if (!rtvHeap_->InitializeWithDevice(device_.Get(), 1000, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false)) {
        LOG_ERROR(logChannel_, "Failed to create RTV descriptor heap");
        return false;
    }
    rtvHeap_->SetDebugName("RTVHeap");

    // Create DSV heap
    dsvHeap_ = std::make_unique<D3D12DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);
    if (!dsvHeap_->InitializeWithDevice(device_.Get(), 100, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false)) {
        LOG_ERROR(logChannel_, "Failed to create DSV descriptor heap");
        return false;
    }
    dsvHeap_->SetDebugName("DSVHeap");

    // Create CBV/SRV/UAV heap
    srvHeap_ = std::make_unique<D3D12DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
    if (!srvHeap_->InitializeWithDevice(device_.Get(), 10000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true)) {
        LOG_ERROR(logChannel_, "Failed to create SRV descriptor heap");
        return false;
    }
    srvHeap_->SetDebugName("SRVHeap");

    // Create Sampler heap
    samplerHeap_ = std::make_unique<D3D12DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, true);
    if (!samplerHeap_->InitializeWithDevice(device_.Get(), 100, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, true)) {
        LOG_ERROR(logChannel_, "Failed to create Sampler descriptor heap");
        return false;
    }
    samplerHeap_->SetDebugName("SamplerHeap");

    LOG_INFO(logChannel_, "Descriptor heaps initialized");
    return true;
}

bool D3D12Device::InitializeSwapChain() {
    swapChain_ = std::make_unique<D3D12SwapChain>();
    if (!swapChain_->InitializeWithDevice(this, graphicsQueue_.get(), surfaceInfo_, config_)) {
        LOG_ERROR(logChannel_, "Failed to initialize swap chain");
        return false;
    }

    LOG_INFO(logChannel_, "Swap chain initialized");
    return true;
}

bool D3D12Device::InitializeResourceManager() {
    resourceManager_ = std::make_unique<D3D12ResourceManager>();
    if (!resourceManager_->Initialize(device_.Get(), adapter_.Get())) {
        LOG_ERROR(logChannel_, "Failed to initialize resource manager");
        return false;
    }

    LOG_INFO(logChannel_, "Resource manager initialized");
    return true;
}

void D3D12Device::DetectCapabilities() {
    capabilities_ = {};

    // Get adapter description
    DXGI_ADAPTER_DESC1 adapterDesc;
    adapter_->GetDesc1(&adapterDesc);

    std::wstring wAdapterDesc(adapterDesc.Description);
    capabilities_.adapterDescription = std::string(wAdapterDesc.begin(), wAdapterDesc.end());
    capabilities_.vendorId = adapterDesc.VendorId;
    capabilities_.deviceId = adapterDesc.DeviceId;
    capabilities_.dedicatedVideoMemory = adapterDesc.DedicatedVideoMemory;
    capabilities_.sharedSystemMemory = adapterDesc.SharedSystemMemory;

    // Check feature levels
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelInfo = {};
    featureLevelInfo.NumFeatureLevels = _countof(featureLevels);
    featureLevelInfo.pFeatureLevelsRequested = featureLevels;

    if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelInfo, sizeof(featureLevelInfo)))) {
        capabilities_.maxFeatureLevel = static_cast<uint32_t>(featureLevelInfo.MaxSupportedFeatureLevel);
    }

    // Check for D3D12 specific features
    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options)))) {
        capabilities_.supportsRayTracing = (options.ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_3);
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1 = {};
    if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options1, sizeof(options1)))) {
        // Check for additional features
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)))) {
        capabilities_.supportsRayTracing = (options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0);
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
    if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6)))) {
        capabilities_.supportsVariableRateShading = (options6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1);
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
    if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7)))) {
        capabilities_.supportsMeshShaders = (options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1);
    }

    // Set texture limits (these are D3D12 maximums)
    capabilities_.maxTexture2DSize = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    capabilities_.maxTexture3DSize = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    capabilities_.maxTextureCubeSize = D3D12_REQ_TEXTURECUBE_DIMENSION;
    capabilities_.maxTextureArraySize = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    capabilities_.maxRenderTargets = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
    capabilities_.maxVertexInputElements = D3D12_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT;
    capabilities_.maxConstantBufferSize = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;

    // Check debug layer availability
    capabilities_.debugLayerAvailable = debugLayerEnabled_;
    capabilities_.gpuValidationAvailable = gpuValidationEnabled_;

    // Check for supported sample counts
    for (uint32_t sampleCount = 1; sampleCount <= 16; sampleCount *= 2) {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaInfo = {};
        msaaInfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        msaaInfo.SampleCount = sampleCount;

        if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaInfo, sizeof(msaaInfo)))) {
            if (msaaInfo.NumQualityLevels > 0) {
                capabilities_.supportedSampleCounts.push_back(sampleCount);
            }
        }
    }
}

void D3D12Device::LogDeviceInfo() {
    LOG_INFO(logChannel_, "=== D3D12 Device Information ===");
    LOG_INFO_FORMAT(logChannel_, "Adapter: {}", capabilities_.adapterDescription);
    LOG_INFO_FORMAT(logChannel_, "Vendor ID: 0x{:04X}", capabilities_.vendorId);
    LOG_INFO_FORMAT(logChannel_, "Device ID: 0x{:04X}", capabilities_.deviceId);
    LOG_INFO_FORMAT(logChannel_, "Video Memory: {} MB", capabilities_.dedicatedVideoMemory / (1024 * 1024));
    LOG_INFO_FORMAT(logChannel_, "Feature Level: {}", GetFeatureLevelString(static_cast<D3D_FEATURE_LEVEL>(capabilities_.maxFeatureLevel)));
    LOG_INFO_FORMAT(logChannel_, "Ray Tracing: {}", capabilities_.supportsRayTracing ? "Supported" : "Not Supported");
    LOG_INFO_FORMAT(logChannel_, "Variable Rate Shading: {}", capabilities_.supportsVariableRateShading ? "Supported" : "Not Supported");
    LOG_INFO_FORMAT(logChannel_, "Mesh Shaders: {}", capabilities_.supportsMeshShaders ? "Supported" : "Not Supported");

    std::string sampleCounts;
    for (uint32_t count : capabilities_.supportedSampleCounts) {
        if (!sampleCounts.empty()) sampleCounts += ", ";
        sampleCounts += std::to_string(count) + "x";
    }
    LOG_INFO_FORMAT(logChannel_, "MSAA Support: {}", sampleCounts);
    LOG_INFO(logChannel_, "================================");
}

void D3D12Device::ProcessDebugMessages() {
    if (!infoQueue_) {
        return;
    }

    uint64_t messageCount = infoQueue_->GetNumStoredMessages();
    for (uint64_t i = 0; i < messageCount; ++i) {
        SIZE_T messageSize = 0;
        infoQueue_->GetMessage(i, nullptr, &messageSize);

        if (messageSize > 0) {
            std::vector<uint8_t> messageBuffer(messageSize);
            D3D12_MESSAGE* message = reinterpret_cast<D3D12_MESSAGE*>(messageBuffer.data());

            if (SUCCEEDED(infoQueue_->GetMessage(i, message, &messageSize))) {
                DebugMessage debugMsg;
                debugMsg.severity = ConvertD3D12Severity(message->Severity);
                debugMsg.category = "D3D12";
                debugMsg.message = std::string(message->pDescription, message->DescriptionByteLength - 1);
                debugMsg.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now().time_since_epoch()).count();

                debugMessages_.push_back(debugMsg);

                // Also log to our logging system
                switch (debugMsg.severity) {
                case DebugMessage::Severity::Info:
                    LOG_INFO_FORMAT(logChannel_, "[D3D12] {}", debugMsg.message);
                    break;
                case DebugMessage::Severity::Warning:
                    LOG_WARNING_FORMAT(logChannel_, "[D3D12] {}", debugMsg.message);
                    break;
                case DebugMessage::Severity::Error:
                case DebugMessage::Severity::Corruption:
                    LOG_ERROR_FORMAT(logChannel_, "[D3D12] {}", debugMsg.message);
                    break;
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
    case D3D12_MESSAGE_SEVERITY_INFO:
    case D3D12_MESSAGE_SEVERITY_MESSAGE:
    default: return DebugMessage::Severity::Info;
    }
}

void D3D12Device::UpdateRenderStats() {
    renderStats_.frameNumber = frameNumber_.load();
    renderStats_.cpuMemoryUsed = 0; // Would be populated from memory tracker
    renderStats_.gpuMemoryUsed = resourceManager_ ? resourceManager_->GetUsedMemory() : 0;
    renderStats_.totalMemoryAvailable = resourceManager_ ? resourceManager_->GetTotalMemory() : 0;
}

void D3D12Device::ResetFrameStats() {
    renderStats_.drawCalls = 0;
    renderStats_.triangles = 0;
    renderStats_.vertices = 0;
    renderStats_.instances = 0;
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
// IRenderDevice Interface Implementation (Stubs for now)
// ============================================================================

BufferHandle D3D12Device::CreateBuffer(const BufferDesc& desc) {
    if (!resourceManager_) return {};
    return resourceManager_->CreateBuffer(desc, device_.Get());
}

TextureHandle D3D12Device::CreateTexture(const TextureDesc& desc) {
    if (!resourceManager_) return {};
    return resourceManager_->CreateTexture(desc, device_.Get());
}

PipelineHandle D3D12Device::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) {
    // Implementation would create D3D12Pipeline
    return {};
}

PipelineHandle D3D12Device::CreateComputePipeline(const ComputePipelineDesc& desc) {
    // Implementation would create D3D12Pipeline
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

void D3D12Device::DestroyPipeline(PipelineHandle handle) {
    // Implementation would destroy D3D12Pipeline
}

IRenderBuffer* D3D12Device::GetBuffer(BufferHandle handle) {
    return resourceManager_ ? resourceManager_->GetBuffer(handle) : nullptr;
}

IRenderTexture* D3D12Device::GetTexture(TextureHandle handle) {
    return resourceManager_ ? resourceManager_->GetTexture(handle) : nullptr;
}

IRenderPipeline* D3D12Device::GetPipeline(PipelineHandle handle) {
    // Implementation would return D3D12Pipeline
    return nullptr;
}

std::unique_ptr<ICommandList> D3D12Device::CreateCommandList(CommandListType type) {
    // Implementation would create D3D12CommandList
    return {};
}

void D3D12Device::ExecuteCommandList(ICommandList* commandList) {
    // Implementation would execute on appropriate queue
}

void D3D12Device::ExecuteCommandLists(uint32_t count, ICommandList* const* commandLists) {
    // Implementation would execute on appropriate queue
}

ISwapChain* D3D12Device::GetSwapChain() {
    return swapChain_.get();
}

std::unique_ptr<IDescriptorHeap> D3D12Device::CreateDescriptorHeap(uint32_t descriptorCount, bool shaderVisible) {
    auto heap = std::make_unique<D3D12DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, shaderVisible);
    if (heap->InitializeWithDevice(device_.Get(), descriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, shaderVisible)) {
        return heap;
    }
    return {};
}

std::vector<DebugMessage> D3D12Device::GetDebugMessages() {
    std::vector<DebugMessage> messages;
    messages.swap(debugMessages_);
    return messages;
}

void D3D12Device::SetDebugName(const char* name) {
    if (name && device_) {
        std::string debugName = name;
        std::wstring wname(debugName.begin(), debugName.end());
        device_->SetName(wname.c_str());
    }
}

const char* D3D12Device::GetDebugName() const {
    return "D3D12Device";
}

void D3D12Device::BeginDebugEvent(const char* name) {
    // PIX event implementation would go here
}

void D3D12Device::EndDebugEvent() {
    // PIX event implementation would go here
}

void D3D12Device::InsertDebugMarker(const char* name) {
    // PIX marker implementation would go here
}

uint64_t D3D12Device::GetUsedVideoMemory() const {
    return resourceManager_ ? resourceManager_->GetUsedMemory() : 0;
}

uint64_t D3D12Device::GetAvailableVideoMemory() const {
    return resourceManager_ ? resourceManager_->GetTotalMemory() - resourceManager_->GetUsedMemory() : 0;
}

void D3D12Device::FlushMemory() {
    // Implementation would flush memory allocations
}