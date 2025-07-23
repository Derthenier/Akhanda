// Engine/Renderer/RHI/D3D12/D3D12Buffer.cpp
// Akhanda Game Engine - D3D12 Buffer Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#include "D3D12Buffer.hpp"
#include "D3D12Core.hpp"

#include <algorithm>
#include <format>
#include <cstring>

#undef min
#undef max

import Akhanda.Engine.RHI;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Core.Logging;
import Akhanda.Core.Memory;

namespace Akhanda::RHI::D3D12 {

    // ========================================================================
    // Constructor / Destructor
    // ========================================================================

    D3D12Buffer::D3D12Buffer()
        : logChannel_(Logging::LogManager::Instance().GetChannel("D3D12Buffer"))
        , currentState_(ResourceState::Common)
        , d3d12State_(D3D12_RESOURCE_STATE_COMMON)
    {
        // Initialize clear values
        vertexBufferView_ = {};
        indexBufferView_ = {};
        constantBufferViewDesc_ = {};

        logChannel_.Log(Logging::LogLevel::Trace, "D3D12Buffer created");
    }

    D3D12Buffer::~D3D12Buffer() {
        if (isValid_.load()) {
            Shutdown();
        }
        logChannel_.Log(Logging::LogLevel::Trace, "D3D12Buffer destroyed");
    }

    // ========================================================================
    // IRenderBuffer Implementation
    // ========================================================================

    bool D3D12Buffer::Initialize([[maybe_unused]] const BufferDesc& desc) {
        logChannel_.LogFormat(Logging::LogLevel::Error,
            "Initialize(BufferDesc) called but D3D12Buffer requires D3D12 device. Use InitializeWithDevice instead.");
        return false;
    }

    void D3D12Buffer::Shutdown() {
        if (!isValid_.load()) {
            return;
        }

        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Shutting Down D3D12Buffer '{}'",
            debugName_);

        // Unmap if currently mapped
        if (isMapped_.load()) {
            UnmapInternal();
        }

        // Clean up resources
        CleanupResources();

        // Reset state
        isValid_.store(false);
        currentState_ = ResourceState::Common;
        d3d12State_ = D3D12_RESOURCE_STATE_COMMON;
        gpuAddress_ = 0;

        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "D3D12Buffer '{}' shutdown completed", debugName_);
    }

    void* D3D12Buffer::Map(uint64_t offset, uint64_t size) {
        if (!isValid_.load()) {
            logChannel_.Log(Logging::LogLevel::Error, "Cannot map invalid buffer");
            return nullptr;
        }

        if (!desc_.cpuAccessible) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Cannot map buffer '{}' - not CPU accessible", debugName_);
            return nullptr;
        }

        if (isMapped_.load()) {
            logChannel_.LogFormat(Logging::LogLevel::Warning,
                "Buffer '{}' is already mapped", debugName_);
            return mappedData_;
        }

        if (!ValidateMapParameters(offset, size)) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(mapMutex_);

        if (!MapInternal(offset, size)) {
            return nullptr;
        }

        isMapped_.store(true);

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Mapped buffer '{}' at offset {}, size {}", debugName_, offset, size);

        return static_cast<uint8_t*>(mappedData_) + offset;
    }

    void D3D12Buffer::Unmap() {
        if (!isMapped_.load()) {
            logChannel_.LogFormat(Logging::LogLevel::Warning,
                "Attempting to unmap buffer '{}' that is not mapped", debugName_);
            return;
        }

        std::lock_guard<std::mutex> lock(mapMutex_);
        UnmapInternal();
        isMapped_.store(false);

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Unmapped buffer '{}'", debugName_);
    }

    void D3D12Buffer::UpdateData(const void* data, uint64_t size, uint64_t offset) {
        if (!isValid_.load()) {
            logChannel_.Log(Logging::LogLevel::Error, "Cannot update invalid buffer");
            return;
        }

        if (!ValidateUpdateParameters(data, size, offset)) {
            return;
        }

        if (desc_.cpuAccessible) {
            // Direct CPU update for upload buffers
            void* mappedPtr = Map(offset, size);
            if (mappedPtr) {
                std::memcpy(mappedPtr, data, static_cast<size_t>(size));
                Unmap();

                logChannel_.LogFormat(Logging::LogLevel::Trace,
                    "Updated buffer '{}' via CPU mapping, size: {}, offset: {}",
                    debugName_, size, offset);
            }
        }
        else {
            // TODO: Implement staging buffer upload for GPU-only buffers
            // This would require a command list and staging buffer
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "GPU-only buffer updates via staging buffer not yet implemented for '{}'",
                debugName_);
        }
    }

    // ========================================================================
    // Resource State Management
    // ========================================================================

    void D3D12Buffer::SetCurrentState(ResourceState state) {
        currentState_ = state;
        d3d12State_ = ConvertResourceState(state);

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Buffer '{}' state changed to {}", debugName_,
            static_cast<uint32_t>(state));
    }

    // ========================================================================
    // D3D12-Specific Methods
    // ========================================================================

    bool D3D12Buffer::InitializeWithDevice(const BufferDesc& desc, ID3D12Device* device,
        D3D12MA::Allocator* allocator) {
        if (!device || !allocator) {
            logChannel_.Log(Logging::LogLevel::Error,
                "Invalid device or allocator provided to InitializeWithDevice");
            return false;
        }

        if (!ValidateBufferDesc(desc)) {
            return false;
        }

        // Store parameters
        desc_ = desc;

        // Set debug name
        if (desc.debugName) {
            debugName_ = desc.debugName;
        }
        else {
            debugName_ = "Unnamed Buffer";
        }

        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Initializing D3D12Buffer '{}', size: {}, stride: {}",
            debugName_, desc_.size, desc_.stride);

        // Create the buffer resource
        if (!CreateBuffer(device, allocator)) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Failed to create buffer resource for '{}'", debugName_);
            return false;
        }

        // Create buffer views
        if (!CreateViews()) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Failed to create buffer views for '{}'", debugName_);
            CleanupResources();
            return false;
        }

        // Log buffer information
        LogBufferInfo();
        LogMemoryInfo();

        isValid_.store(true);

        logChannel_.LogFormat(Logging::LogLevel::Info,
            "Successfully initialized D3D12Buffer '{}'", debugName_);

        return true;
    }

    void D3D12Buffer::SetDebugName(const char* name) {
        if (name) {
            debugName_ = name;

            if (buffer_) {
                std::wstring wideName(name, name + std::strlen(name));
                buffer_->SetName(wideName.c_str());
            }

            logChannel_.LogFormat(Logging::LogLevel::Trace,
                "Set debug name for buffer: '{}'", name);
        }
    }

    bool D3D12Buffer::IsReadbackBuffer() const {
        // A readback buffer is typically a buffer in the READBACK heap
        // that allows the CPU to read data written by the GPU
        D3D12_HEAP_TYPE heapType = GetHeapType();
        return heapType == D3D12_HEAP_TYPE_READBACK;
    }

    uint64_t D3D12Buffer::GetMemorySize() const {
        return memorySize_;
    }

    uint64_t D3D12Buffer::GetAlignedSize() const {
        if (IsConstantBuffer()) {
            // Constant buffers must be aligned to 256 bytes
            return (desc_.size + 255) & ~255;
        }
        return desc_.size;
    }

    // ========================================================================
    // Private Implementation Methods
    // ========================================================================

    bool D3D12Buffer::CreateBuffer(ID3D12Device* device, D3D12MA::Allocator* allocator) {
        // Setup resource description
        D3D12_RESOURCE_DESC resourceDesc = {};
        SetupResourceDesc(resourceDesc);

        // Setup heap properties
        D3D12_HEAP_PROPERTIES heapProps = {};
        SetupHeapProperties(heapProps);

        // Setup initial resource state
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
        SetupInitialResourceState(initialState);

        // Create allocation info
        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = heapProps.Type;
        allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;

        // Create the resource through D3D12MemAlloc
        HRESULT hr = allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            initialState,
            nullptr,  // Clear value (not used for buffers)
            &allocation_,
            IID_PPV_ARGS(&buffer_)
        );

        if (FAILED(hr)) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Failed to create D3D12 buffer resource for '{}', HRESULT: 0x{:08X}",
                debugName_, static_cast<uint32_t>(hr));
            return false;
        }

        // Get GPU virtual address
        gpuAddress_ = buffer_->GetGPUVirtualAddress();

        // Store memory information
        allocationInfo_ = device->GetResourceAllocationInfo(0, 1, &resourceDesc);
        memorySize_ = allocationInfo_.SizeInBytes;

        // Set resource name for debugging
        if (!debugName_.empty()) {
            std::wstring wideName(debugName_.begin(), debugName_.end());
            buffer_->SetName(wideName.c_str());
        }

        // Set initial state
        currentState_ = ConvertD3D12ResourceState(initialState);
        d3d12State_ = initialState;

        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Created D3D12 buffer resource for '{}', GPU address: 0x{:016X}",
            debugName_, gpuAddress_);

        return true;
    }

    bool D3D12Buffer::CreateViews() {
        // Create appropriate views based on buffer usage
        bool success = true;

        if (IsVertexBuffer()) {
            CreateVertexBufferView();
            logChannel_.LogFormat(Logging::LogLevel::Trace,
                "Created vertex buffer view for '{}'", debugName_);
        }

        if (IsIndexBuffer()) {
            CreateIndexBufferView();
            logChannel_.LogFormat(Logging::LogLevel::Trace,
                "Created index buffer view for '{}'", debugName_);
        }

        if (IsConstantBuffer()) {
            CreateConstantBufferView();
            logChannel_.LogFormat(Logging::LogLevel::Trace,
                "Created constant buffer view for '{}'", debugName_);
        }

        // TODO: Create SRV/UAV for structured buffers when descriptor heap integration is complete

        return success;
    }

    void D3D12Buffer::SetupHeapProperties(D3D12_HEAP_PROPERTIES& heapProps) const {
        D3D12_HEAP_TYPE heapType = GetHeapType();

        heapProps.Type = heapType;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Setup heap properties for '{}': type = {}",
            debugName_, static_cast<int>(heapType));
    }

    void D3D12Buffer::SetupResourceDesc(D3D12_RESOURCE_DESC& resourceDesc) const {
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0; // Let D3D12 decide
        resourceDesc.Width = GetAlignedSize();
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN; // Buffers use UNKNOWN format
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = GetResourceFlags();

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Setup resource description for '{}': width = {}, flags = 0x{:X}",
            debugName_, resourceDesc.Width, static_cast<uint32_t>(resourceDesc.Flags));
    }

    void D3D12Buffer::SetupInitialResourceState(D3D12_RESOURCE_STATES& initialState) const {
        if (desc_.cpuAccessible) {
            // Upload buffers start in GENERIC_READ state
            initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        }
        else {
            // GPU-only buffers can start in COMMON state
            initialState = D3D12_RESOURCE_STATE_COMMON;
        }

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Setup initial resource state for '{}': state = 0x{:X}",
            debugName_, static_cast<uint32_t>(initialState));
    }

    bool D3D12Buffer::ValidateBufferDesc(const BufferDesc& desc) const {
        if (desc.size == 0) {
            logChannel_.Log(Logging::LogLevel::Error, "Buffer size cannot be zero");
            return false;
        }

        if (desc.size > static_cast<uint64_t>(D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM) * 1024 * 1024) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Buffer size {} exceeds D3D12 maximum", desc.size);
            return false;
        }

        if ((desc.usage & ResourceUsage::VertexBuffer) != ResourceUsage::None && desc.stride == 0) {
            logChannel_.Log(Logging::LogLevel::Error, "Vertex buffer must have non-zero stride");
            return false;
        }

        if ((desc.usage & ResourceUsage::IndexBuffer) != ResourceUsage::None) {
            if (desc.stride != 2 && desc.stride != 4) {
                logChannel_.Log(Logging::LogLevel::Error,
                    "Index buffer stride must be 2 or 4 bytes");
                return false;
            }
        }

        if ((desc.usage & ResourceUsage::ConstantBuffer) != ResourceUsage::None) {
            // Constant buffers have specific alignment requirements
            if (desc.size > D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16) {
                logChannel_.Log(Logging::LogLevel::Error,
                    "Constant buffer exceeds maximum size");
                return false;
            }
        }

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Buffer description validation passed for size: {}, stride: {}, usage: 0x{:X}",
            desc.size, desc.stride, static_cast<uint32_t>(desc.usage));

        return true;
    }

    bool D3D12Buffer::ValidateMapParameters(uint64_t offset, uint64_t size) const {
        if (offset >= desc_.size) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Map offset {} is beyond buffer size {}", offset, desc_.size);
            return false;
        }

        if (size > 0 && (offset + size) > desc_.size) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Map range (offset: {}, size: {}) exceeds buffer size {}",
                offset, size, desc_.size);
            return false;
        }

        return true;
    }

    bool D3D12Buffer::ValidateUpdateParameters(const void* data, uint64_t size, uint64_t offset) const {
        if (!data) {
            logChannel_.Log(Logging::LogLevel::Error, "Update data pointer is null");
            return false;
        }

        if (size == 0) {
            logChannel_.Log(Logging::LogLevel::Error, "Update size cannot be zero");
            return false;
        }

        if (offset >= desc_.size) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Update offset {} is beyond buffer size {}", offset, desc_.size);
            return false;
        }

        if ((offset + size) > desc_.size) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Update range (offset: {}, size: {}) exceeds buffer size {}",
                offset, size, desc_.size);
            return false;
        }

        return true;
    }

    void D3D12Buffer::CreateVertexBufferView() {
        vertexBufferView_.BufferLocation = gpuAddress_;
        vertexBufferView_.StrideInBytes = desc_.stride;
        vertexBufferView_.SizeInBytes = static_cast<UINT>(desc_.size);

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Created vertex buffer view: address=0x{:016X}, stride={}, size={}",
            gpuAddress_, desc_.stride, desc_.size);
    }

    void D3D12Buffer::CreateIndexBufferView() {
        indexBufferView_.BufferLocation = gpuAddress_;
        indexBufferView_.SizeInBytes = static_cast<UINT>(desc_.size);

        // Determine index format based on stride
        if (desc_.stride == 2) {
            indexBufferView_.Format = DXGI_FORMAT_R16_UINT;
        }
        else if (desc_.stride == 4) {
            indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
        }
        else {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Invalid index buffer stride: {}", desc_.stride);
            indexBufferView_.Format = DXGI_FORMAT_UNKNOWN;
        }

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Created index buffer view: address=0x{:016X}, format={}, size={}",
            gpuAddress_, static_cast<int>(indexBufferView_.Format), desc_.size);
    }

    void D3D12Buffer::CreateConstantBufferView() {
        constantBufferViewDesc_.BufferLocation = gpuAddress_;
        constantBufferViewDesc_.SizeInBytes = static_cast<UINT>(GetAlignedSize());

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Created constant buffer view: address=0x{:016X}, size={} (aligned from {})",
            gpuAddress_, constantBufferViewDesc_.SizeInBytes, desc_.size);
    }

    bool D3D12Buffer::MapInternal([[maybe_unused]] uint64_t offset, [[maybe_unused]] uint64_t size) {
        D3D12_RANGE readRange = { 0, 0 }; // We don't intend to read from this resource on CPU

        HRESULT hr = buffer_->Map(0, &readRange, &mappedData_);
        if (FAILED(hr)) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Failed to map buffer '{}', HRESULT: 0x{:08X}",
                debugName_, static_cast<uint32_t>(hr));
            return false;
        }

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Successfully mapped buffer '{}' at address 0x{:016X}",
            debugName_, reinterpret_cast<uintptr_t>(mappedData_));

        return true;
    }

    void D3D12Buffer::UnmapInternal() {
        if (mappedData_) {
            D3D12_RANGE writtenRange = { 0, desc_.size };
            buffer_->Unmap(0, &writtenRange);
            mappedData_ = nullptr;

            logChannel_.LogFormat(Logging::LogLevel::Trace,
                "Unmapped buffer '{}'", debugName_);
        }
    }

    void D3D12Buffer::LogBufferInfo() const {
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Buffer '{}' Info - Size: {}, Stride: {}, Usage: 0x{:X}, CPU Accessible: {}",
            debugName_, desc_.size, desc_.stride,
            static_cast<uint32_t>(desc_.usage), desc_.cpuAccessible);

        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Buffer '{}' GPU Address: 0x{:016X}", debugName_, gpuAddress_);
    }

    void D3D12Buffer::LogMemoryInfo() const {
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Buffer '{}' Memory - Allocated: {} bytes, Alignment: {}",
            debugName_, memorySize_, allocationInfo_.Alignment);

        if (allocation_) {
            logChannel_.LogFormat(Logging::LogLevel::Debug,
                "Buffer '{}' Heap Type: {}", debugName_,
                static_cast<int>(allocation_->GetHeap()->GetDesc().Properties.Type));
        }
    }

    D3D12_HEAP_TYPE D3D12Buffer::GetHeapType() const {
        if (desc_.cpuAccessible) {
            // Check if this is a readback buffer (GPU writes, CPU reads)
            if ((desc_.usage & ResourceUsage::CopyDestination) != ResourceUsage::None) {
                return D3D12_HEAP_TYPE_READBACK;
            }
            // Otherwise it's an upload buffer (CPU writes, GPU reads)
            return D3D12_HEAP_TYPE_UPLOAD;
        }

        // GPU-only buffer
        return D3D12_HEAP_TYPE_DEFAULT;
    }

    D3D12_RESOURCE_FLAGS D3D12Buffer::GetResourceFlags() const {
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

        if ((desc_.usage & ResourceUsage::UnorderedAccess) != ResourceUsage::None) {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        // Buffers cannot be render targets or depth stencils, so we don't need those flags

        return flags;
    }

    size_t D3D12Buffer::GetRequiredAlignment() const {
        if (IsConstantBuffer()) {
            return D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT; // 256 bytes
        }

        return D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT; // 64KB
    }

    void D3D12Buffer::CleanupResources() {
        // Release D3D12 resources
        if (buffer_) {
            logChannel_.LogFormat(Logging::LogLevel::Debug,
                "Releasing D3D12 buffer resource for '{}'", debugName_);
            buffer_.Reset();
        }

        if (allocation_) {
            logChannel_.LogFormat(Logging::LogLevel::Debug,
                "Releasing D3D12MemAlloc allocation for '{}'", debugName_);
            allocation_->Release();
            allocation_ = nullptr;
        }

        // Reset views
        vertexBufferView_ = {};
        indexBufferView_ = {};
        constantBufferViewDesc_ = {};

        // Reset memory info
        memorySize_ = 0;
        allocationInfo_ = {};
        mappedData_ = nullptr;

        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Resource cleanup completed for '{}'", debugName_);
    }

    namespace BufferUtils {
        // ========================================================================
        // Static Logging Channel
        // ========================================================================
        static Logging::LogChannel& GetLogChannel() {
            static Logging::LogChannel& logChannel = Logging::LogManager::Instance().GetChannel("D3D12BufferUtils");
            return logChannel;
        }

        // ========================================================================
        // Buffer Creation Helpers
        // ========================================================================

        std::unique_ptr<D3D12Buffer> CreateVertexBuffer(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint64_t size,
            uint32_t stride,
            const void* initialData,
            const char* debugName) {
            if (!device || !allocator) {
                GetLogChannel().Log(Logging::LogLevel::Error, "Invalid device or allocator for vertex buffer creation");
                return nullptr;
            }

            if (size == 0 || stride == 0) {
                GetLogChannel().LogFormat(Logging::LogLevel::Error,
                    "Invalid vertex buffer parameters: size={}, stride={}", size, stride);
                return nullptr;
            }

            BufferDesc desc = {};
            desc.size = size;
            desc.stride = stride;
            desc.usage = ResourceUsage::VertexBuffer;
            desc.cpuAccessible = initialData ? true : false;  // Make CPU accessible if we have initial data
            desc.initialData = initialData;
            desc.debugName = debugName ? debugName : "Vertex Buffer";

            auto buffer = std::make_unique<D3D12Buffer>();
            if (!buffer->InitializeWithDevice(desc, device, allocator)) {
                GetLogChannel().LogFormat(Logging::LogLevel::Error,
                    "Failed to create vertex buffer '{}'", desc.debugName);
                return nullptr;
            }

            GetLogChannel().LogFormat(Logging::LogLevel::Debug,
                "Created vertex buffer '{}': size={}, stride={}", desc.debugName, size, stride);

            return buffer;
        }

        std::unique_ptr<D3D12Buffer> CreateIndexBuffer(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint64_t size,
            Format indexFormat,
            const void* initialData,
            const char* debugName) {
            if (!device || !allocator) {
                GetLogChannel().Log(Logging::LogLevel::Error, "Invalid device or allocator for index buffer creation");
                return nullptr;
            }

            if (size == 0) {
                GetLogChannel().Log(Logging::LogLevel::Error, "Index buffer size cannot be zero");
                return nullptr;
            }

            if (!IsValidIndexFormat(indexFormat)) {
                GetLogChannel().LogFormat(Logging::LogLevel::Error,
                    "Invalid index format: {}", static_cast<uint32_t>(indexFormat));
                return nullptr;
            }

            uint32_t stride = GetFormatSize(indexFormat);

            BufferDesc desc = {};
            desc.size = size;
            desc.stride = stride;
            desc.usage = ResourceUsage::IndexBuffer;
            desc.cpuAccessible = initialData ? true : false;  // Make CPU accessible if we have initial data
            desc.initialData = initialData;
            desc.debugName = debugName ? debugName : "Index Buffer";

            auto buffer = std::make_unique<D3D12Buffer>();
            if (!buffer->InitializeWithDevice(desc, device, allocator)) {
                GetLogChannel().LogFormat(Logging::LogLevel::Error,
                    "Failed to create index buffer '{}'", desc.debugName);
                return nullptr;
            }

            GetLogChannel().LogFormat(Logging::LogLevel::Debug,
                "Created index buffer '{}': size={}, format={}", desc.debugName, size, static_cast<uint32_t>(indexFormat));

            return buffer;
        }

        std::unique_ptr<D3D12Buffer> CreateConstantBuffer(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint64_t size,
            const void* initialData,
            const char* debugName) {
            if (!device || !allocator) {
                GetLogChannel().Log(Logging::LogLevel::Error, "Invalid device or allocator for constant buffer creation");
                return nullptr;
            }

            if (size == 0) {
                GetLogChannel().Log(Logging::LogLevel::Error, "Constant buffer size cannot be zero");
                return nullptr;
            }

            // Align size to constant buffer alignment requirements
            uint64_t alignedSize = CalculateConstantBufferSize(size);

            BufferDesc desc = {};
            desc.size = alignedSize;
            desc.stride = 0;  // Not applicable for constant buffers
            desc.usage = ResourceUsage::ConstantBuffer;
            desc.cpuAccessible = true;  // Constant buffers are typically CPU accessible for frequent updates
            desc.initialData = initialData;
            desc.debugName = debugName ? debugName : "Constant Buffer";

            auto buffer = std::make_unique<D3D12Buffer>();
            if (!buffer->InitializeWithDevice(desc, device, allocator)) {
                GetLogChannel().LogFormat(Logging::LogLevel::Error,
                    "Failed to create constant buffer '{}'", desc.debugName);
                return nullptr;
            }

            GetLogChannel().LogFormat(Logging::LogLevel::Debug,
                "Created constant buffer '{}': size={} (aligned from {})", desc.debugName, alignedSize, size);

            return buffer;
        }

        std::unique_ptr<D3D12Buffer> CreateUploadBuffer(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint64_t size,
            const char* debugName) {
            if (!device || !allocator) {
                GetLogChannel().Log(Logging::LogLevel::Error, "Invalid device or allocator for upload buffer creation");
                return nullptr;
            }

            if (size == 0) {
                GetLogChannel().Log(Logging::LogLevel::Error, "Upload buffer size cannot be zero");
                return nullptr;
            }

            BufferDesc desc = {};
            desc.size = size;
            desc.stride = 0;  // Generic upload buffer
            desc.usage = ResourceUsage::CopySource;  // Upload buffers are typically copy sources
            desc.cpuAccessible = true;  // Upload buffers must be CPU accessible
            desc.initialData = nullptr;
            desc.debugName = debugName ? debugName : "Upload Buffer";

            auto buffer = std::make_unique<D3D12Buffer>();
            if (!buffer->InitializeWithDevice(desc, device, allocator)) {
                GetLogChannel().LogFormat(Logging::LogLevel::Error,
                    "Failed to create upload buffer '{}'", desc.debugName);
                return nullptr;
            }

            GetLogChannel().LogFormat(Logging::LogLevel::Debug,
                "Created upload buffer '{}': size={}", desc.debugName, size);

            return buffer;
        }

        std::unique_ptr<D3D12Buffer> CreateReadbackBuffer(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint64_t size,
            const char* debugName) {
            if (!device || !allocator) {
                GetLogChannel().Log(Logging::LogLevel::Error, "Invalid device or allocator for readback buffer creation");
                return nullptr;
            }

            if (size == 0) {
                GetLogChannel().Log(Logging::LogLevel::Error, "Readback buffer size cannot be zero");
                return nullptr;
            }

            BufferDesc desc = {};
            desc.size = size;
            desc.stride = 0;  // Generic readback buffer
            desc.usage = ResourceUsage::CopyDestination;  // Readback buffers are typically copy destinations
            desc.cpuAccessible = true;  // Readback buffers must be CPU accessible for reading
            desc.initialData = nullptr;
            desc.debugName = debugName ? debugName : "Readback Buffer";

            auto buffer = std::make_unique<D3D12Buffer>();
            if (!buffer->InitializeWithDevice(desc, device, allocator)) {
                GetLogChannel().LogFormat(Logging::LogLevel::Error,
                    "Failed to create readback buffer '{}'", desc.debugName);
                return nullptr;
            }

            GetLogChannel().LogFormat(Logging::LogLevel::Debug,
                "Created readback buffer '{}': size={}", desc.debugName, size);

            return buffer;
        }

        // ========================================================================
        // Buffer Size Calculations
        // ========================================================================

        uint64_t CalculateConstantBufferSize(uint64_t size) {
            // Constant buffers must be aligned to 256 bytes
            constexpr uint64_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT; // 256 bytes
            return AlignBufferSize(size, alignment);
        }

        uint64_t CalculateVertexBufferSize(uint64_t vertexCount, uint32_t stride) {
            if (vertexCount == 0 || stride == 0) {
                GetLogChannel().LogFormat(Logging::LogLevel::Error,
                    "Invalid vertex buffer parameters: count={}, stride={}", vertexCount, stride);
                return 0;
            }

            uint64_t size = vertexCount * stride;

            // Check for overflow
            if (size / stride != vertexCount) {
                GetLogChannel().LogFormat(Logging::LogLevel::Error,
                    "Vertex buffer size calculation overflow: count={}, stride={}", vertexCount, stride);
                return 0;
            }

            return size;
        }

        uint64_t CalculateIndexBufferSize(uint64_t indexCount, Format indexFormat) {
            if (indexCount == 0) {
                GetLogChannel().Log(Logging::LogLevel::Error, "Index count cannot be zero");
                return 0;
            }

            uint32_t indexSize = GetFormatSize(indexFormat);
            if (indexSize == 0) {
                GetLogChannel().LogFormat(Logging::LogLevel::Error,
                    "Invalid index format: {}", static_cast<uint32_t>(indexFormat));
                return 0;
            }

            uint64_t size = indexCount * indexSize;

            // Check for overflow
            if (size / indexSize != indexCount) {
                GetLogChannel().LogFormat(Logging::LogLevel::Error,
                    "Index buffer size calculation overflow: count={}, format={}",
                    indexCount, static_cast<uint32_t>(indexFormat));
                return 0;
            }

            return size;
        }

        // ========================================================================
        // Buffer Alignment
        // ========================================================================

        uint64_t AlignBufferSize(uint64_t size, uint64_t alignment) {
            if (alignment == 0) {
                GetLogChannel().Log(Logging::LogLevel::Error, "Alignment cannot be zero");
                return size;
            }

            // Check if alignment is power of 2
            if ((alignment & (alignment - 1)) != 0) {
                GetLogChannel().LogFormat(Logging::LogLevel::Warning,
                    "Alignment {} is not power of 2, using standard calculation", alignment);
            }

            return (size + alignment - 1) & ~(alignment - 1);
        }

        uint64_t GetConstantBufferAlignment() {
            return D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT; // 256 bytes
        }

        uint64_t GetVertexBufferAlignment() {
            return 1; // Vertex buffers don't have special alignment requirements
        }

        uint64_t GetIndexBufferAlignment() {
            return 1; // Index buffers don't have special alignment requirements
        }

        // ========================================================================
        // Resource Usage Conversion
        // ========================================================================

        D3D12_RESOURCE_FLAGS ConvertResourceUsageToFlags(ResourceUsage usage) {
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

            if ((usage & ResourceUsage::RenderTarget) != ResourceUsage::None) {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            }

            if ((usage & ResourceUsage::DepthStencil) != ResourceUsage::None) {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            }

            if ((usage & ResourceUsage::UnorderedAccess) != ResourceUsage::None) {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }

            return flags;
        }

        D3D12_HEAP_TYPE ConvertResourceUsageToHeapType(ResourceUsage usage, bool cpuAccessible) {
            if (cpuAccessible) {
                // Check if this is primarily for GPU writing, CPU reading (readback)
                if ((usage & ResourceUsage::CopyDestination) != ResourceUsage::None) {
                    return D3D12_HEAP_TYPE_READBACK;
                }
                // Otherwise it's for CPU writing, GPU reading (upload)
                return D3D12_HEAP_TYPE_UPLOAD;
            }

            // GPU-only resources
            return D3D12_HEAP_TYPE_DEFAULT;
        }

        D3D12_RESOURCE_STATES ConvertResourceUsageToState(ResourceUsage usage) {
            // Return the most appropriate initial state based on usage
            if ((usage & ResourceUsage::RenderTarget) != ResourceUsage::None) {
                return D3D12_RESOURCE_STATE_RENDER_TARGET;
            }

            if ((usage & ResourceUsage::DepthStencil) != ResourceUsage::None) {
                return D3D12_RESOURCE_STATE_DEPTH_WRITE;
            }

            if ((usage & ResourceUsage::UnorderedAccess) != ResourceUsage::None) {
                return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            }

            if ((usage & ResourceUsage::ShaderResource) != ResourceUsage::None) {
                return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            }

            if ((usage & ResourceUsage::VertexBuffer) != ResourceUsage::None) {
                return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            }

            if ((usage & ResourceUsage::IndexBuffer) != ResourceUsage::None) {
                return D3D12_RESOURCE_STATE_INDEX_BUFFER;
            }

            if ((usage & ResourceUsage::ConstantBuffer) != ResourceUsage::None) {
                return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            }

            if ((usage & ResourceUsage::CopySource) != ResourceUsage::None) {
                return D3D12_RESOURCE_STATE_COPY_SOURCE;
            }

            if ((usage & ResourceUsage::CopyDestination) != ResourceUsage::None) {
                return D3D12_RESOURCE_STATE_COPY_DEST;
            }

            return D3D12_RESOURCE_STATE_COMMON;
        }

        // ========================================================================
        // Validation Functions
        // ========================================================================

        bool IsValidBufferSize(uint64_t size) {
            if (size == 0) {
                return false;
            }

            // Check against D3D12 maximum resource size
            constexpr uint64_t maxSize = static_cast<uint64_t>(D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM) * 1024 * 1024;
            return size <= maxSize;
        }

        bool IsValidVertexStride(uint32_t stride) {
            if (stride == 0) {
                return false;
            }

            // Check against D3D12 maximum vertex stride
            return stride <= D3D12_SO_BUFFER_MAX_STRIDE_IN_BYTES;
        }

        bool IsValidIndexFormat(Format format) {
            return (format == Format::R16_UInt) || (format == Format::R32_UInt);
        }

        bool IsCompatibleUsage(ResourceUsage usage, bool cpuAccessible) {
            // Some usage combinations are not compatible with CPU accessibility

            if (cpuAccessible) {
                // CPU accessible buffers cannot be render targets or depth stencils
                if ((usage & ResourceUsage::RenderTarget) != ResourceUsage::None ||
                    (usage & ResourceUsage::DepthStencil) != ResourceUsage::None) {
                    return false;
                }

                // UAV buffers can be CPU accessible but it's not common
                if ((usage & ResourceUsage::UnorderedAccess) != ResourceUsage::None) {
                    GetLogChannel().Log(Logging::LogLevel::Warning,
                        "CPU accessible UAV buffer is uncommon and may have performance implications");
                }
            }

            return true;
        }

    }



    // ========================================================================
    // D3D12StagingBuffer Implementation
    // ========================================================================

    bool D3D12StagingBuffer::Initialize(const StagingBufferDesc& desc,
        ID3D12Device* device,
        D3D12MA::Allocator* allocator) {
        BufferDesc bufferDesc = {};
        bufferDesc.size = desc.size;
        bufferDesc.stride = 0;
        bufferDesc.usage = ResourceUsage::CopySource;
        bufferDesc.cpuAccessible = true;
        bufferDesc.debugName = desc.debugName;

        buffer_ = std::make_unique<D3D12Buffer>();
        if (!buffer_->InitializeWithDevice(bufferDesc, device, allocator)) {
            return false;
        }

        size_ = desc.size;
        isPersistentlyMapped_ = desc.persistent;

        if (isPersistentlyMapped_) {
            mappedData_ = buffer_->Map(0, size_);
            if (!mappedData_) {
                return false;
            }
        }

        return true;
    }

    void D3D12StagingBuffer::Shutdown() {
        if (isPersistentlyMapped_ && mappedData_) {
            buffer_->Unmap();
            mappedData_ = nullptr;
        }

        buffer_.reset();
        currentOffset_ = 0;
        size_ = 0;
    }

    D3D12StagingBuffer::StagingAllocation D3D12StagingBuffer::Allocate(uint64_t size, uint32_t alignment) {
        StagingAllocation allocation = {};

        std::lock_guard<std::mutex> lock(offsetMutex_);

        // Align the offset
        uint64_t alignedOffset = (currentOffset_ + alignment - 1) & ~(static_cast<uint64_t>(alignment) - 1);

        if (alignedOffset + size > size_) {
            // Not enough space
            allocation.valid = false;
            return allocation;
        }

        void* baseAddress = isPersistentlyMapped_ ? mappedData_ : buffer_->Map(alignedOffset, size);
        if (!baseAddress) {
            allocation.valid = false;
            return allocation;
        }

        allocation.cpuAddress = static_cast<uint8_t*>(baseAddress) + (isPersistentlyMapped_ ? alignedOffset : 0);
        allocation.gpuAddress = buffer_->GetGPUAddress() + alignedOffset;
        allocation.offset = alignedOffset;
        allocation.size = size;
        allocation.valid = true;

        currentOffset_ = alignedOffset + size;

        return allocation;
    }

    void D3D12StagingBuffer::Reset() {
        std::lock_guard<std::mutex> lock(offsetMutex_);
        currentOffset_ = 0;
    }

    // ========================================================================
    // D3D12BufferHandleManager Implementation
    // ========================================================================

    BufferHandle D3D12BufferHandleManager::CreateHandle(std::unique_ptr<D3D12Buffer> buffer) {
        std::lock_guard<std::mutex> lock(handleMutex_);

        uint32_t handleId;
        if (!availableHandles_.empty()) {
            handleId = availableHandles_.front();
            availableHandles_.pop();
        }
        else {
            handleId = nextHandleId_.fetch_add(1);
        }

        HandleEntry& entry = handleMap_[handleId];
        entry.buffer = std::move(buffer);
        entry.refCount = 1;
        entry.isValid = true;

        return BufferHandle(handleId, 0);
    }

    D3D12Buffer* D3D12BufferHandleManager::GetBuffer(BufferHandle handle) {
        std::lock_guard<std::mutex> lock(handleMutex_);

        auto it = handleMap_.find(handle.index);
        if (it != handleMap_.end() && it->second.isValid) {
            return it->second.buffer.get();
        }

        return nullptr;
    }

    bool D3D12BufferHandleManager::IsValid(BufferHandle handle) const {
        std::lock_guard<std::mutex> lock(handleMutex_);

        auto it = handleMap_.find(handle.index);
        return it != handleMap_.end() && it->second.isValid;
    }

    void D3D12BufferHandleManager::AddRef(BufferHandle handle) {
        std::lock_guard<std::mutex> lock(handleMutex_);

        auto it = handleMap_.find(handle.index);
        if (it != handleMap_.end() && it->second.isValid) {
            ++it->second.refCount;
        }
    }

    void D3D12BufferHandleManager::Release(BufferHandle handle) {
        std::lock_guard<std::mutex> lock(handleMutex_);

        auto it = handleMap_.find(handle.index);
        if (it != handleMap_.end()) {
            --it->second.refCount;
            if (it->second.refCount == 0) {
                it->second.buffer.reset();
                it->second.isValid = false;
                availableHandles_.push(handle.index);
                handleMap_.erase(it);
            }
        }
    }

    void D3D12BufferHandleManager::InvalidateHandle(BufferHandle handle) {
        std::lock_guard<std::mutex> lock(handleMutex_);

        auto it = handleMap_.find(handle.index);
        if (it != handleMap_.end()) {
            // Mark handle as invalid but don't immediately destroy
            // This allows for graceful cleanup of any pending references
            it->second.isValid = false;

            // If no references remain, clean up immediately
            if (it->second.refCount == 0) {
                it->second.buffer.reset();
                availableHandles_.push(handle.index);
                handleMap_.erase(it);
            }
            // Note: If refCount > 0, cleanup will happen when Release() brings it to 0
        }
    }

    uint32_t D3D12BufferHandleManager::GetActiveHandleCount() const {
        std::lock_guard<std::mutex> lock(handleMutex_);

        uint32_t activeCount = 0;
        for (const auto& pair : handleMap_) {
            if (pair.second.isValid) {
                ++activeCount;
            }
        }

        return activeCount;
    }

    std::vector<BufferHandle> D3D12BufferHandleManager::GetActiveHandles() const {
        std::lock_guard<std::mutex> lock(handleMutex_);

        std::vector<BufferHandle> activeHandles;
        activeHandles.reserve(handleMap_.size()); // Reserve for efficiency

        for (const auto& pair : handleMap_) {
            if (pair.second.isValid) {
                activeHandles.emplace_back(BufferHandle(pair.first, 0));
            }
        }

        return activeHandles;
    }

    // ========================================================================
    // D3D12BufferPool Implementation  
    // ========================================================================

    D3D12BufferPool::D3D12BufferPool(const BufferPoolConfig& config)
        : config_(config) {
    }

    D3D12BufferPool::~D3D12BufferPool() {
        TrimPools(0); // Clear all pools
    }

    uint64_t D3D12BufferPool::GetSizeCategory(uint64_t size) const {
        // Create size buckets for pooling (powers of 2)
        uint64_t category = 1024; // Start at 1KB
        while (category < size && category < config_.maxBufferSize) {
            category <<= 1;
        }
        return category;
    }

    D3D12BufferPool::PoolKey D3D12BufferPool::CreatePoolKey(const BufferDesc& desc) const {
        return PoolKey{
            desc.usage,
            GetSizeCategory(desc.size),
            desc.cpuAccessible
        };
    }

    std::unique_ptr<D3D12Buffer> D3D12BufferPool::AcquireBuffer(const BufferDesc& desc,
        ID3D12Device* device,
        D3D12MA::Allocator* allocator) {
        if (!config_.enablePooling || desc.size < config_.minBufferSize || desc.size > config_.maxBufferSize) {
            // Create new buffer directly
            auto buffer = std::make_unique<D3D12Buffer>();
            if (buffer->InitializeWithDevice(desc, device, allocator)) {
                std::lock_guard<std::mutex> lock(poolMutex_);
                ++stats_.totalBuffersCreated;
                ++stats_.poolMisses;
                stats_.totalMemoryAllocated += desc.size;
                return buffer;
            }
            return nullptr;
        }

        PoolKey key = CreatePoolKey(desc);

        std::lock_guard<std::mutex> lock(poolMutex_);
        auto poolIt = pools_.find(key);

        if (poolIt != pools_.end() && !poolIt->second.empty()) {
            // Found a pooled buffer
            auto buffer = std::move(poolIt->second.front());
            poolIt->second.pop();

            ++stats_.poolHits;
            --stats_.pooledBuffers;
            ++stats_.activeBuffers;
            stats_.totalMemoryInUse += desc.size;
            stats_.peakMemoryUsage = std::max(stats_.peakMemoryUsage, stats_.totalMemoryInUse);

            return buffer;
        }
        else {
            // Create new buffer
            auto buffer = std::make_unique<D3D12Buffer>();
            if (buffer->InitializeWithDevice(desc, device, allocator)) {
                ++stats_.totalBuffersCreated;
                ++stats_.poolMisses;
                ++stats_.activeBuffers;
                stats_.totalMemoryAllocated += desc.size;
                stats_.totalMemoryInUse += desc.size;
                stats_.peakMemoryUsage = std::max(stats_.peakMemoryUsage, stats_.totalMemoryInUse);
                return buffer;
            }
        }

        return nullptr;
    }

    void D3D12BufferPool::ReturnBuffer(std::unique_ptr<D3D12Buffer> buffer) {
        if (!buffer || !config_.enablePooling) {
            return;
        }

        BufferDesc desc;
        desc.usage = buffer->GetUsage();
        desc.size = buffer->GetSize();
        desc.cpuAccessible = buffer->IsCPUAccessible();

        PoolKey key = CreatePoolKey(desc);

        std::lock_guard<std::mutex> lock(poolMutex_);

        auto& pool = pools_[key];
        if (pool.size() < config_.maxPooledBuffers) {
            pool.push(std::move(buffer));
            ++stats_.pooledBuffers;
            --stats_.activeBuffers;
            stats_.totalMemoryInUse -= desc.size;
        }
        else {
            // Pool is full, destroy the buffer
            buffer.reset();
            ++stats_.totalBuffersDestroyed;
            --stats_.activeBuffers;
            stats_.totalMemoryInUse -= desc.size;
        }
    }

    void D3D12BufferPool::PreallocateBuffers(const BufferDesc& desc, uint32_t count,
        ID3D12Device* device, D3D12MA::Allocator* allocator) {

        if (!config_.enablePooling || count == 0) {
            return;
        }

        if (desc.size < config_.minBufferSize || desc.size > config_.maxBufferSize) {
            // Don't preallocate buffers outside our pooling range
            return;
        }

        PoolKey poolKey = CreatePoolKey(desc);

        std::lock_guard<std::mutex> lock(poolMutex_);

        auto& pool = pools_[poolKey];
        uint32_t currentPoolSize = static_cast<uint32_t>(pool.size());

        // Don't exceed maximum pool size
        uint32_t maxAllowed = config_.maxPooledBuffers > currentPoolSize ?
            config_.maxPooledBuffers - currentPoolSize : 0;
        uint32_t buffersToCreate = std::min(count, maxAllowed);

        if (buffersToCreate == 0) {
            return;
        }

        uint32_t successfullyCreated = 0;

        for (uint32_t i = 0; i < buffersToCreate; ++i) {
            auto buffer = std::make_unique<D3D12Buffer>();

            if (buffer->InitializeWithDevice(desc, device, allocator)) {
                pool.push(std::move(buffer));
                ++successfullyCreated;
                ++stats_.totalBuffersCreated;
                ++stats_.pooledBuffers;
                stats_.totalMemoryAllocated += desc.size;

                // Update peak memory usage if needed
                if (stats_.totalMemoryAllocated > stats_.peakMemoryUsage) {
                    stats_.peakMemoryUsage = stats_.totalMemoryAllocated;
                }
            }
            else {
                // Failed to create buffer, stop trying
                break;
            }
        }

        // Log preallocation results if any buffers were created
        if (successfullyCreated > 0) {
            auto& logChannel = Logging::LogManager::Instance().GetChannel("D3D12BufferPool");
            logChannel.LogFormat(Logging::LogLevel::Debug,
                "Preallocated {} buffers (size: {}, usage: 0x{:X}, cpuAccessible: {})",
                successfullyCreated, desc.size, static_cast<uint32_t>(desc.usage), desc.cpuAccessible);
        }
    }

    void D3D12BufferPool::TrimPools(uint32_t maxBuffersPerPool) {
        std::lock_guard<std::mutex> lock(poolMutex_);

        uint32_t trimLimit = (maxBuffersPerPool == 0) ? 0 : maxBuffersPerPool;
        uint32_t totalBuffersDestroyed = 0;
        uint64_t memoryFreed = 0;

        auto poolIt = pools_.begin();
        while (poolIt != pools_.end()) {
            auto& pool = poolIt->second;

            // Trim this pool to the specified limit
            while (pool.size() > trimLimit) {
                auto buffer = std::move(pool.front());
                pool.pop();

                if (buffer) {
                    uint64_t bufferSize = buffer->GetSize();
                    memoryFreed += bufferSize;
                    ++totalBuffersDestroyed;
                    --stats_.pooledBuffers;
                    stats_.totalMemoryAllocated -= bufferSize;

                    // Buffer will be destroyed when unique_ptr goes out of scope
                }
            }

            // Remove empty pools to free memory
            if (pool.empty()) {
                poolIt = pools_.erase(poolIt);
            }
            else {
                ++poolIt;
            }
        }

        // Update statistics
        stats_.totalBuffersDestroyed += totalBuffersDestroyed;

        // Log trimming results if anything was trimmed
        if (totalBuffersDestroyed > 0) {
            auto& logChannel = Logging::LogManager::Instance().GetChannel("D3D12BufferPool");
            logChannel.LogFormat(Logging::LogLevel::Debug,
                "Trimmed {} buffers from pools, freed {:.2f} MB memory",
                totalBuffersDestroyed, memoryFreed / (1024.0 * 1024.0));
        }
    }

    void D3D12BufferPool::LogPoolStatistics() {
        std::lock_guard<std::mutex> lock(poolMutex_);

        auto& logChannel = Logging::LogManager::Instance().GetChannel("D3D12BufferPool");

        // Log overall statistics
        logChannel.LogFormat(Logging::LogLevel::Info,
            "=== D3D12 Buffer Pool Statistics ===");

        logChannel.LogFormat(Logging::LogLevel::Info,
            "Total Buffers: Created={}, Destroyed={}, Active={}, Pooled={}",
            stats_.totalBuffersCreated, stats_.totalBuffersDestroyed,
            stats_.activeBuffers, stats_.pooledBuffers);

        logChannel.LogFormat(Logging::LogLevel::Info,
            "Memory Usage: Allocated={:.2f} MB, In Use={:.2f} MB, Peak={:.2f} MB",
            stats_.totalMemoryAllocated / (1024.0 * 1024.0),
            stats_.totalMemoryInUse / (1024.0 * 1024.0),
            stats_.peakMemoryUsage / (1024.0 * 1024.0));

        logChannel.LogFormat(Logging::LogLevel::Info,
            "Pool Efficiency: Hits={}, Misses={}, Hit Rate={:.1f}%",
            stats_.poolHits, stats_.poolMisses,
            (stats_.poolHits + stats_.poolMisses) > 0 ?
            (100.0 * stats_.poolHits) / (stats_.poolHits + stats_.poolMisses) : 0.0);

        // Log per-pool statistics
        if (!pools_.empty()) {
            logChannel.LogFormat(Logging::LogLevel::Info,
                "Active Pools: {} pools with details below:", pools_.size());

            size_t poolIndex = 0;
            for (const auto& poolPair : pools_) {
                const auto& poolKey = poolPair.first;
                const auto& pool = poolPair.second;

                // Calculate total memory for this pool
                uint64_t poolMemory = poolKey.sizeCategory * pool.size();

                logChannel.LogFormat(Logging::LogLevel::Info,
                    "  Pool {}: Usage=0x{:X}, Size={} KB, CPU={}, Buffers={}, Memory={:.2f} MB",
                    poolIndex++,
                    static_cast<uint32_t>(poolKey.usage),
                    poolKey.sizeCategory / 1024,
                    poolKey.cpuAccessible ? "Yes" : "No",
                    pool.size(),
                    poolMemory / (1024.0 * 1024.0));
            }
        }
        else {
            logChannel.Log(Logging::LogLevel::Info, "No active pools");
        }

        // Log configuration
        if (config_.enablePooling) {
            logChannel.LogFormat(Logging::LogLevel::Info,
                "Pool Configuration: MinSize={} KB, MaxSize={} MB, MaxPerPool={}, Pooling=Enabled",
                config_.minBufferSize / 1024,
                config_.maxBufferSize / (1024 * 1024),
                config_.maxPooledBuffers);
        }
        else {
            logChannel.LogFormat(Logging::LogLevel::Info,
                "Pool Configuration: MinSize={} KB, MaxSize={} MB, MaxPerPool={}, Pooling=Disabled",
                config_.minBufferSize / 1024,
                config_.maxBufferSize / (1024 * 1024),
                config_.maxPooledBuffers);
        }

        logChannel.Log(Logging::LogLevel::Info, "=== End Pool Statistics ===");
    }

    // ========================================================================
    // D3D12BufferManager Implementation
    // ========================================================================

    D3D12BufferManager::D3D12BufferManager()
        : logChannel_(Logging::LogManager::Instance().GetChannel("D3D12BufferManager")) {
    }

    D3D12BufferManager::~D3D12BufferManager() {
        if (isInitialized_.load()) {
            Shutdown();
        }
    }

    bool D3D12BufferManager::Initialize(D3D12Device* device,
        D3D12MA::Allocator* allocator,
        const BufferPoolConfig& config) {
        if (isInitialized_.load()) {
            logChannel_.Log(Logging::LogLevel::Warning, "Buffer manager already initialized");
            return true;
        }

        if (!device || !allocator) {
            logChannel_.Log(Logging::LogLevel::Error, "Invalid device or allocator provided");
            return false;
        }

        device_ = device;
        allocator_ = allocator;
        poolConfig_ = config;

        // Initialize buffer pool
        bufferPool_ = std::make_unique<D3D12BufferPool>(config);

        // Initialize handle manager
        handleManager_ = std::make_unique<D3D12BufferHandleManager>();

        // Create staging buffers
        if (!CreateStagingBuffers()) {
            logChannel_.Log(Logging::LogLevel::Error, "Failed to create staging buffers");
            Shutdown();
            return false;
        }

        isInitialized_.store(true);

        logChannel_.Log(Logging::LogLevel::Info, "D3D12 Buffer Manager initialized successfully");
        LogMemoryUsage();

        return true;
    }

    void D3D12BufferManager::Shutdown() {
        if (!isInitialized_.load()) {
            return;
        }

        logChannel_.Log(Logging::LogLevel::Debug, "Shutting down D3D12 Buffer Manager");

        // Log final statistics
        LogDetailedStatistics();

        // Destroy staging buffers
        DestroyStagingBuffers();

        // Clean up handle manager
        handleManager_.reset();

        // Clean up buffer pool
        bufferPool_.reset();

        // Reset state
        device_ = nullptr;
        allocator_ = nullptr;
        currentStagingBufferIndex_ = 0;

        isInitialized_.store(false);

        logChannel_.Log(Logging::LogLevel::Info, "D3D12 Buffer Manager shutdown completed");
    }

    BufferHandle D3D12BufferManager::CreateBuffer(const BufferDesc& desc) {
        if (!isInitialized_.load()) {
            logChannel_.Log(Logging::LogLevel::Error, "Buffer manager not initialized");
            return BufferHandle{ 0 };
        }

        if (!ValidateBufferDesc(desc)) {
            return BufferHandle{ 0 };
        }

        auto buffer = bufferPool_->AcquireBuffer(desc, device_->GetD3D12Device(), allocator_);
        if (!buffer) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Failed to create buffer: size={}, usage=0x{:X}", desc.size, static_cast<uint32_t>(desc.usage));
            return BufferHandle{ 0 };
        }

        BufferHandle handle = handleManager_->CreateHandle(std::move(buffer));

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Created buffer handle {}: size={}, usage=0x{:X}", handle.index, desc.size, static_cast<uint32_t>(desc.usage));

        return handle;
    }

    BufferHandle D3D12BufferManager::CreateVertexBuffer(uint64_t size, uint32_t stride,
        const void* initialData,
        const char* debugName) {
        BufferDesc desc = {};
        desc.size = size;
        desc.stride = stride;
        desc.usage = ResourceUsage::VertexBuffer;
        desc.cpuAccessible = initialData != nullptr;
        desc.initialData = initialData;
        desc.debugName = debugName ? debugName : "Vertex Buffer";

        return CreateBuffer(desc);
    }

    BufferHandle D3D12BufferManager::CreateConstantBuffer(uint64_t size,
        const void* initialData,
        const char* debugName) {
        // Align size to constant buffer requirements
        uint64_t alignedSize = BufferUtils::CalculateConstantBufferSize(size);

        BufferDesc desc = {};
        desc.size = alignedSize;
        desc.stride = 0;
        desc.usage = ResourceUsage::ConstantBuffer;
        desc.cpuAccessible = true; // Constant buffers are typically CPU accessible
        desc.initialData = initialData;
        desc.debugName = debugName ? debugName : "Constant Buffer";

        return CreateBuffer(desc);
    }

    void D3D12BufferManager::LogStatistics() const {
        if (!isInitialized_.load()) {
            return;
        }

        BufferPoolStats poolStats = bufferPool_->GetStats();
        BufferPoolStats globalStats = GetGlobalStatistics();

        logChannel_.LogFormat(Logging::LogLevel::Info,
            "Buffer Manager Stats - Active: {}, Created: {}, Pool Hits: {}, Pool Misses: {}",
            globalStats.activeBuffers, globalStats.totalBuffersCreated,
            globalStats.poolHits, globalStats.poolMisses);

        logChannel_.LogFormat(Logging::LogLevel::Info,
            "Memory Usage - In Use: {:.2f} MB, Allocated: {:.2f} MB, Peak: {:.2f} MB",
            globalStats.totalMemoryInUse / (1024.0 * 1024.0),
            globalStats.totalMemoryAllocated / (1024.0 * 1024.0),
            globalStats.peakMemoryUsage / (1024.0 * 1024.0));
    }

    bool D3D12BufferManager::CreateStagingBuffers() {
        const uint64_t stagingBufferSize = 16 * 1024 * 1024; // 16MB per staging buffer

        for (size_t i = 0; i < stagingBuffers_.size(); ++i) {
            StagingBufferDesc desc = {};
            desc.size = stagingBufferSize;
            desc.alignment = 256;
            desc.persistent = true;
            desc.debugName = "Staging Buffer";

            auto stagingBuffer = std::make_unique<D3D12StagingBuffer>();
            if (!stagingBuffer->Initialize(desc, device_->GetD3D12Device(), allocator_)) {
                logChannel_.LogFormat(Logging::LogLevel::Error,
                    "Failed to create staging buffer {}", i);
                return false;
            }

            stagingBuffers_[i] = std::move(stagingBuffer);
        }

        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Created {} staging buffers ({:.2f} MB each)",
            stagingBuffers_.size(), stagingBufferSize / (1024.0 * 1024.0));

        return true;
    }

    bool D3D12BufferManager::ValidateBufferDesc(const BufferDesc& desc) const {
        if (desc.size == 0) {
            logChannel_.Log(Logging::LogLevel::Error, "Buffer size cannot be zero");
            return false;
        }

        if (desc.usage == ResourceUsage::None) {
            logChannel_.Log(Logging::LogLevel::Error, "Buffer must have valid usage flags");
            return false;
        }

        if ((desc.usage & ResourceUsage::VertexBuffer) != ResourceUsage::None && desc.stride == 0) {
            logChannel_.Log(Logging::LogLevel::Error, "Vertex buffer must have non-zero stride");
            return false;
        }

        return true;
    }


    bool D3D12BufferManager::IsValidBuffer(BufferHandle handle) const {
        if (!isInitialized_.load()) {
            return false;
        }

        return handleManager_->IsValid(handle);
    }

    BufferHandle D3D12BufferManager::CreateIndexBuffer(uint64_t size, Format indexFormat,
        const void* initialData, const char* debugName) {

        if (!isInitialized_.load()) {
            logChannel_.Log(Logging::LogLevel::Error, "Buffer manager not initialized");
            return BufferHandle{ 0 };
        }

        // Determine stride based on index format
        uint32_t stride = 0;
        switch (indexFormat) {
        case Format::R16_UInt:
            stride = 2;
            break;
        case Format::R32_UInt:
            stride = 4;
            break;
        default:
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Invalid index format: {}", static_cast<uint32_t>(indexFormat));
            return BufferHandle{ 0 };
        }

        BufferDesc desc = {};
        desc.size = size;
        desc.stride = stride;
        desc.usage = ResourceUsage::IndexBuffer;
        desc.cpuAccessible = initialData != nullptr; // CPU accessible if we have initial data
        desc.initialData = initialData;
        desc.debugName = debugName ? debugName : "Index Buffer";

        BufferHandle handle = CreateBuffer(desc);

        if (handle.index != 0) {
            logChannel_.LogFormat(Logging::LogLevel::Trace,
                "Created index buffer handle {}: size={}, format={}, stride={}",
                handle.index, size, static_cast<uint32_t>(indexFormat), stride);
        }

        return handle;
    }

    BufferHandle D3D12BufferManager::CreateStructuredBuffer(uint64_t elementCount, uint32_t elementSize,
        ResourceUsage usage, const void* initialData, const char* debugName) {

        if (!isInitialized_.load()) {
            logChannel_.Log(Logging::LogLevel::Error, "Buffer manager not initialized");
            return BufferHandle{ 0 };
        }

        if (elementCount == 0 || elementSize == 0) {
            logChannel_.Log(Logging::LogLevel::Error,
                "Structured buffer must have non-zero element count and size");
            return BufferHandle{ 0 };
        }

        // Ensure structured buffer usage flags are set
        ResourceUsage structuredUsage = usage | ResourceUsage::ShaderResource;
        if ((usage & ResourceUsage::UnorderedAccess) != ResourceUsage::None) {
            structuredUsage = structuredUsage | ResourceUsage::UnorderedAccess;
        }

        uint64_t totalSize = elementCount * elementSize;

        BufferDesc desc = {};
        desc.size = totalSize;
        desc.stride = elementSize; // For structured buffers, stride = element size
        desc.usage = structuredUsage;
        desc.cpuAccessible = initialData != nullptr;
        desc.initialData = initialData;
        desc.debugName = debugName ? debugName : "Structured Buffer";

        BufferHandle handle = CreateBuffer(desc);

        if (handle.index != 0) {
            logChannel_.LogFormat(Logging::LogLevel::Trace,
                "Created structured buffer handle {}: elements={}, elementSize={}, totalSize={}",
                handle.index, elementCount, elementSize, totalSize);
        }

        return handle;
    }

    void D3D12BufferManager::AddBufferRef(BufferHandle handle) {
        if (!isInitialized_.load()) {
            logChannel_.Log(Logging::LogLevel::Warning,
                "Cannot add reference - buffer manager not initialized");
            return;
        }

        if (handle.index == 0) {
            logChannel_.Log(Logging::LogLevel::Warning, "Cannot add reference to null buffer handle");
            return;
        }

        handleManager_->AddRef(handle);

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Added reference to buffer handle {}", handle.index);
    }

    void D3D12BufferManager::ReleaseBufferRef(BufferHandle handle) {
        if (!isInitialized_.load()) {
            logChannel_.Log(Logging::LogLevel::Warning,
                "Cannot release reference - buffer manager not initialized");
            return;
        }

        if (handle.index == 0) {
            logChannel_.Log(Logging::LogLevel::Warning, "Cannot release null buffer handle");
            return;
        }

        handleManager_->Release(handle);

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Released reference to buffer handle {}", handle.index);
    }

    void D3D12BufferManager::DestroyStagingBuffers() {
        std::lock_guard<std::mutex> lock(stagingBufferMutex_);

        uint32_t destroyedCount = 0;
        uint64_t memoryFreed = 0;

        for (auto& stagingBuffer : stagingBuffers_) {
            if (stagingBuffer) {
                memoryFreed += stagingBuffer->GetSize();
                stagingBuffer->Shutdown();
                stagingBuffer.reset();
                ++destroyedCount;
            }
        }

        currentStagingBufferIndex_ = 0;

        if (destroyedCount > 0) {
            logChannel_.LogFormat(Logging::LogLevel::Debug,
                "Destroyed {} staging buffers, freed {:.2f} MB",
                destroyedCount, memoryFreed / (1024.0 * 1024.0));
        }
    }

    void D3D12BufferManager::UpdatePoolConfiguration(const BufferPoolConfig& newConfig) {
        if (!isInitialized_.load()) {
            logChannel_.Log(Logging::LogLevel::Warning,
                "Cannot update pool configuration - buffer manager not initialized");
            return;
        }

        // Validate new configuration
        if (newConfig.maxBufferSize < newConfig.minBufferSize) {
            logChannel_.Log(Logging::LogLevel::Error,
                "Invalid pool configuration: maxBufferSize < minBufferSize");
            return;
        }

        if (newConfig.maxPooledBuffers == 0) {
            logChannel_.Log(Logging::LogLevel::Warning,
                "Pool configuration sets maxPooledBuffers to 0 - pooling will be disabled");
        }

        BufferPoolConfig oldConfig = poolConfig_;
        poolConfig_ = newConfig;

        logChannel_.LogFormat(Logging::LogLevel::Info,
            "Updated pool configuration - MinSize: {} KB -> {} KB, MaxSize: {} MB -> {} MB, MaxPerPool: {} -> {}, Enabled: {} -> {}",
            oldConfig.minBufferSize / 1024, newConfig.minBufferSize / 1024,
            oldConfig.maxBufferSize / (1024 * 1024), newConfig.maxBufferSize / (1024 * 1024),
            oldConfig.maxPooledBuffers, newConfig.maxPooledBuffers,
            oldConfig.enablePooling ? "Yes" : "No", newConfig.enablePooling ? "Yes" : "No");

        // If pooling was disabled, trim all pools
        if (!newConfig.enablePooling && bufferPool_) {
            bufferPool_->TrimPools(0);
            logChannel_.Log(Logging::LogLevel::Info, "Trimmed all pools due to pooling being disabled");
        }

        // If buffer size limits changed significantly, consider trimming pools outside new limits
        if (newConfig.maxBufferSize < oldConfig.maxBufferSize / 2 ||
            newConfig.minBufferSize > oldConfig.minBufferSize * 2) {
            logChannel_.Log(Logging::LogLevel::Info,
                "Significant buffer size limit changes detected - consider calling TrimMemory()");
        }
    }

    void D3D12BufferManager::UpdateGlobalStats() {
        if (!isInitialized_.load()) {
            return;
        }

        std::lock_guard<std::mutex> lock(statsMutex_);

        // Get current pool statistics
        BufferPoolStats poolStats = bufferPool_->GetStats();

        // Update global statistics with pool data
        globalStats_ = poolStats;

        // Add handle manager statistics
        uint32_t activeHandles = handleManager_->GetActiveHandleCount();
        globalStats_.activeBuffers = activeHandles;

        // Calculate staging buffer memory usage
        uint64_t stagingMemoryUsed = 0;
        {
            std::lock_guard<std::mutex> stagingLock(stagingBufferMutex_);
            for (const auto& stagingBuffer : stagingBuffers_) {
                if (stagingBuffer) {
                    stagingMemoryUsed += stagingBuffer->GetUsedSize();
                }
            }
        }

        // Add staging memory to total memory in use
        globalStats_.totalMemoryInUse += stagingMemoryUsed;

        // Update peak memory usage if needed
        if (globalStats_.totalMemoryInUse > globalStats_.peakMemoryUsage) {
            globalStats_.peakMemoryUsage = globalStats_.totalMemoryInUse;
        }

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Updated global stats - Active: {}, Memory In Use: {:.2f} MB (including {:.2f} MB staging)",
            globalStats_.activeBuffers,
            globalStats_.totalMemoryInUse / (1024.0 * 1024.0),
            stagingMemoryUsed / (1024.0 * 1024.0));
    }


    D3D12StagingBuffer::StagingAllocation D3D12BufferManager::AllocateStagingMemory(uint64_t size, uint32_t alignment) {
        D3D12StagingBuffer::StagingAllocation invalidAllocation = {};
        invalidAllocation.valid = false;

        if (!isInitialized_.load()) {
            logChannel_.Log(Logging::LogLevel::Error,
                "Cannot allocate staging memory - buffer manager not initialized");
            return invalidAllocation;
        }

        if (size == 0) {
            logChannel_.Log(Logging::LogLevel::Error, "Cannot allocate zero-sized staging memory");
            return invalidAllocation;
        }

        // Ensure alignment is power of 2 and at least 1
        if (alignment == 0) alignment = 1;
        if ((alignment & (alignment - 1)) != 0) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Staging memory alignment {} is not a power of 2", alignment);
            return invalidAllocation;
        }

        std::lock_guard<std::mutex> lock(stagingBufferMutex_);

        // Try current staging buffer first
        auto currentBuffer = GetCurrentStagingBuffer();
        if (currentBuffer) {
            auto allocation = currentBuffer->Allocate(size, alignment);
            if (allocation.valid) {
                logChannel_.LogFormat(Logging::LogLevel::Trace,
                    "Allocated {} bytes from current staging buffer (index: {})",
                    size, currentStagingBufferIndex_);
                return allocation;
            }
        }

        // Try other staging buffers in round-robin fashion
        uint32_t originalIndex = currentStagingBufferIndex_;
        for (uint32_t attempt = 0; attempt < stagingBuffers_.size(); ++attempt) {
            AdvanceStagingBuffer();

            auto stagingBuffer = GetCurrentStagingBuffer();
            if (stagingBuffer) {
                auto allocation = stagingBuffer->Allocate(size, alignment);
                if (allocation.valid) {
                    logChannel_.LogFormat(Logging::LogLevel::Trace,
                        "Allocated {} bytes from staging buffer {} after {} attempts",
                        size, currentStagingBufferIndex_, attempt + 1);
                    return allocation;
                }
            }
        }

        // Restore original index if all failed
        currentStagingBufferIndex_ = originalIndex;

        // All staging buffers are full - log detailed information
        uint64_t totalStagingSize = 0;
        uint64_t totalUsedSize = 0;
        for (const auto& buffer : stagingBuffers_) {
            if (buffer) {
                totalStagingSize += buffer->GetSize();
                totalUsedSize += buffer->GetUsedSize();
            }
        }

        logChannel_.LogFormat(Logging::LogLevel::Warning,
            "Failed to allocate {} bytes staging memory. Total staging: {:.2f} MB, Used: {:.2f} MB ({:.1f}%)",
            size,
            totalStagingSize / (1024.0 * 1024.0),
            totalUsedSize / (1024.0 * 1024.0),
            totalStagingSize > 0 ? (100.0 * totalUsedSize) / totalStagingSize : 0.0);

        return invalidAllocation;
    }

    bool D3D12BufferManager::UploadToBuffer(BufferHandle handle, const void* data,
        uint64_t size, uint64_t offset) {

        if (!isInitialized_.load()) {
            logChannel_.Log(Logging::LogLevel::Error,
                "Cannot upload to buffer - buffer manager not initialized");
            return false;
        }

        if (!data || size == 0) {
            logChannel_.Log(Logging::LogLevel::Error, "Invalid data or size for buffer upload");
            return false;
        }

        D3D12Buffer* targetBuffer = GetBuffer(handle);
        if (!targetBuffer) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Cannot upload to invalid buffer handle {}", handle.index);
            return false;
        }

        if (offset + size > targetBuffer->GetSize()) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Upload range (offset: {}, size: {}) exceeds buffer size {}",
                offset, size, targetBuffer->GetSize());
            return false;
        }

        // If target buffer is CPU accessible, do direct update
        if (targetBuffer->IsCPUAccessible()) {
            targetBuffer->UpdateData(data, size, offset);

            logChannel_.LogFormat(Logging::LogLevel::Trace,
                "Direct CPU upload to buffer {}: {} bytes at offset {}",
                handle.index, size, offset);
            return true;
        }

        // For GPU-only buffers, use staging buffer approach
        auto stagingAllocation = AllocateStagingMemory(size, 256); // D3D12 copy alignment
        if (!stagingAllocation.valid) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Failed to allocate staging memory for buffer {} upload", handle.index);
            return false;
        }

        // Copy data to staging buffer
        std::memcpy(stagingAllocation.cpuAddress, data, static_cast<size_t>(size));

        // TODO: Record copy command to command list
        // This would require a command list parameter or internal command recording system
        // For now, log that GPU upload is pending implementation
        logChannel_.LogFormat(Logging::LogLevel::Warning,
            "GPU buffer upload via staging buffer allocated but command recording not yet implemented. "
            "Buffer: {}, Size: {}, Offset: {}, Staging GPU Address: 0x{:016X}",
            handle.index, size, offset, stagingAllocation.gpuAddress);

        // Note: In a complete implementation, you would:
        // 1. Get or create a command list
        // 2. Record CopyBufferRegion command from staging to target buffer
        // 3. Either execute immediately or defer execution
        // 4. Handle synchronization to ensure upload completes before staging buffer reuse

        return true;
    }

    D3D12StagingBuffer* D3D12BufferManager::GetCurrentStagingBuffer() {
        // Note: This method assumes stagingBufferMutex_ is already locked by caller
        // It's designed to be called from within other methods that handle locking

        if (currentStagingBufferIndex_ >= stagingBuffers_.size()) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Invalid staging buffer index: {}", currentStagingBufferIndex_);
            return nullptr;
        }

        auto& stagingBuffer = stagingBuffers_[currentStagingBufferIndex_];
        if (!stagingBuffer) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Staging buffer at index {} is null", currentStagingBufferIndex_);
            return nullptr;
        }

        return stagingBuffer.get();
    }

    void D3D12BufferManager::AdvanceStagingBuffer() {
        // Note: This method assumes stagingBufferMutex_ is already locked by caller
        // It's designed to be called from within other methods that handle locking

        uint32_t previousIndex = currentStagingBufferIndex_;
        currentStagingBufferIndex_ = (currentStagingBufferIndex_ + 1) % stagingBuffers_.size();

        // Reset the new current staging buffer for reuse
        auto currentBuffer = GetCurrentStagingBuffer();
        if (currentBuffer) {
            currentBuffer->Reset();

            logChannel_.LogFormat(Logging::LogLevel::Trace,
                "Advanced staging buffer from {} to {} and reset for reuse",
                previousIndex, currentStagingBufferIndex_);
        }
        else {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Failed to get staging buffer after advancing to index {}",
                currentStagingBufferIndex_);
        }
    }

    // ========================================================================
    // Additional Staging System Helper Methods
    // ========================================================================

    void D3D12BufferManager::ResetAllStagingBuffers() {
        std::lock_guard<std::mutex> lock(stagingBufferMutex_);

        for (auto& stagingBuffer : stagingBuffers_) {
            if (stagingBuffer) {
                stagingBuffer->Reset();
            }
        }

        currentStagingBufferIndex_ = 0;

        logChannel_.Log(Logging::LogLevel::Debug, "Reset all staging buffers");
    }

    uint64_t D3D12BufferManager::GetTotalStagingMemorySize() const {
        std::lock_guard<std::mutex> lock(stagingBufferMutex_);

        uint64_t totalSize = 0;
        for (const auto& stagingBuffer : stagingBuffers_) {
            if (stagingBuffer) {
                totalSize += stagingBuffer->GetSize();
            }
        }

        return totalSize;
    }

    uint64_t D3D12BufferManager::GetUsedStagingMemorySize() const {
        std::lock_guard<std::mutex> lock(stagingBufferMutex_);

        uint64_t usedSize = 0;
        for (const auto& stagingBuffer : stagingBuffers_) {
            if (stagingBuffer) {
                usedSize += stagingBuffer->GetUsedSize();
            }
        }

        return usedSize;
    }

    float D3D12BufferManager::GetStagingMemoryUsagePercentage() const {
        uint64_t totalSize = GetTotalStagingMemorySize();
        if (totalSize == 0) {
            return 0.0f;
        }

        uint64_t usedSize = GetUsedStagingMemorySize();
        return (100.0f * usedSize) / totalSize;
    }



    // ========================================================================
    // D3D12BufferManager - Memory Management & Statistics Implementation
    // ========================================================================

    void D3D12BufferManager::TrimMemory(bool aggressive) {
        if (!isInitialized_.load()) {
            logChannel_.Log(Logging::LogLevel::Warning,
                "Cannot trim memory - buffer manager not initialized");
            return;
        }

        logChannel_.LogFormat(Logging::LogLevel::Info,
            "Starting {} memory trim operation", aggressive ? "aggressive" : "normal");

        uint64_t memoryBeforeTrim = 0;
        uint64_t memoryAfterTrim = 0;

        // Update statistics before trimming
        UpdateGlobalStats();
        {
            std::lock_guard<std::mutex> lock(statsMutex_);
            memoryBeforeTrim = globalStats_.totalMemoryAllocated;
        }

        // Trim buffer pools
        if (bufferPool_) {
            uint32_t maxBuffersPerPool = aggressive ? 0 : (poolConfig_.maxPooledBuffers / 2);
            bufferPool_->TrimPools(maxBuffersPerPool);

            logChannel_.LogFormat(Logging::LogLevel::Debug,
                "Trimmed buffer pools to max {} buffers per pool",
                aggressive ? 0 : maxBuffersPerPool);
        }

        // Reset staging buffers if aggressive trimming
        if (aggressive) {
            ResetAllStagingBuffers();
            logChannel_.Log(Logging::LogLevel::Debug, "Reset all staging buffers for aggressive trim");
        }

        // Force garbage collection of released handles
        if (handleManager_) {
            // Get all active handles and check for any that should be cleaned up
            auto activeHandles = handleManager_->GetActiveHandles();
            uint32_t handlesBeforeCleanup = static_cast<uint32_t>(activeHandles.size());

            // Note: In a more sophisticated implementation, you might want to:
            // - Check for handles with zero references that can be cleaned up
            // - Implement a delayed cleanup system for recently released handles
            // For now, we log the current handle count

            logChannel_.LogFormat(Logging::LogLevel::Debug,
                "Handle cleanup check: {} active handles", handlesBeforeCleanup);
        }

        // Update statistics after trimming
        UpdateGlobalStats();
        {
            std::lock_guard<std::mutex> lock(statsMutex_);
            memoryAfterTrim = globalStats_.totalMemoryAllocated;
        }

        uint64_t memoryFreed = memoryBeforeTrim - memoryAfterTrim;

        logChannel_.LogFormat(Logging::LogLevel::Info,
            "Memory trim completed: freed {:.2f} MB ({:.1f}% reduction)",
            memoryFreed / (1024.0 * 1024.0),
            memoryBeforeTrim > 0 ? (100.0 * memoryFreed) / memoryBeforeTrim : 0.0);

        // Log current memory state
        LogMemoryUsage();
    }

    void D3D12BufferManager::FlushPendingUploads() {
        if (!isInitialized_.load()) {
            logChannel_.Log(Logging::LogLevel::Warning,
                "Cannot flush pending uploads - buffer manager not initialized");
            return;
        }

        // TODO: Implement proper upload queue flushing
        // In a complete implementation, this would:
        // 1. Execute any pending command lists with copy operations
        // 2. Wait for GPU completion of upload operations
        // 3. Reset staging buffers that are no longer in use by GPU
        // 4. Update synchronization state

        // For now, we can at least ensure staging buffers are properly managed
        {
            std::lock_guard<std::mutex> lock(stagingBufferMutex_);

            uint64_t totalPendingUploads = 0;
            for (const auto& stagingBuffer : stagingBuffers_) {
                if (stagingBuffer) {
                    totalPendingUploads += stagingBuffer->GetUsedSize();
                }
            }

            if (totalPendingUploads > 0) {
                logChannel_.LogFormat(Logging::LogLevel::Info,
                    "Flushing pending uploads: {:.2f} MB of staging data",
                    totalPendingUploads / (1024.0 * 1024.0));

                // In the absence of proper command list integration, we can at least
                // log what would need to be flushed and reset staging buffers
                // NOTE: This is not safe for actual GPU operations without proper synchronization

                logChannel_.Log(Logging::LogLevel::Warning,
                    "Command list integration not implemented - staging buffers NOT reset");

                // TODO: When command recording is implemented:
                // - Execute all pending upload command lists
                // - Wait for GPU fence/completion
                // - Then call ResetAllStagingBuffers()
            }
            else {
                logChannel_.Log(Logging::LogLevel::Debug, "No pending uploads to flush");
            }
        }
    }

    BufferPoolStats D3D12BufferManager::GetPoolStatistics() const {
        if (!isInitialized_.load()) {
            BufferPoolStats emptyStats = {};
            return emptyStats;
        }

        // Get pool statistics
        BufferPoolStats poolStats = bufferPool_->GetStats();

        // Enhance with handle manager data
        uint32_t activeHandles = handleManager_->GetActiveHandleCount();
        poolStats.activeBuffers = activeHandles;

        // Add staging buffer statistics
        uint64_t stagingMemoryTotal = GetTotalStagingMemorySize();
        uint64_t stagingMemoryUsed = GetUsedStagingMemorySize();

        // Note: Staging memory is separate from pooled buffer memory
        // but we can track it in separate fields or extend the stats structure

        logChannel_.LogFormat(Logging::LogLevel::Trace,
            "Retrieved pool statistics: {} active, {} created, {} pooled, {:.2f} MB allocated",
            poolStats.activeBuffers, poolStats.totalBuffersCreated,
            poolStats.pooledBuffers, poolStats.totalMemoryAllocated / (1024.0 * 1024.0));

        return poolStats;
    }

    void D3D12BufferManager::LogDetailedStatistics() const {
        if (!isInitialized_.load()) {
            logChannel_.Log(Logging::LogLevel::Warning,
                "Cannot log statistics - buffer manager not initialized");
            return;
        }

        logChannel_.Log(Logging::LogLevel::Info, "=== D3D12 Buffer Manager Detailed Statistics ===");

        // Pool statistics
        if (bufferPool_) {
            bufferPool_->LogPoolStatistics();
        }

        // Handle manager statistics
        if (handleManager_) {
            uint32_t activeHandleCount = handleManager_->GetActiveHandleCount();
            auto activeHandles = handleManager_->GetActiveHandles();

            logChannel_.LogFormat(Logging::LogLevel::Info,
                "Handle Manager: {} active handles", activeHandleCount);

            if (!activeHandles.empty() && activeHandleCount <= 20) {
                // Log individual handles if there aren't too many
                std::string handleList;
                for (size_t i = 0; i < activeHandles.size(); ++i) {
                    if (i > 0) handleList += ", ";
                    handleList += std::to_string(activeHandles[i].index);
                }
                logChannel_.LogFormat(Logging::LogLevel::Info,
                    "Active handle IDs: {}", handleList);
            }
        }

        // Staging buffer statistics
        {
            std::lock_guard<std::mutex> lock(stagingBufferMutex_);

            uint64_t totalStagingSize = 0;
            uint64_t totalUsedSize = 0;
            uint32_t activeStagingBuffers = 0;

            logChannel_.LogFormat(Logging::LogLevel::Info,
                "Staging Buffers: {} total, current index: {}",
                stagingBuffers_.size(), currentStagingBufferIndex_);

            for (size_t i = 0; i < stagingBuffers_.size(); ++i) {
                const auto& buffer = stagingBuffers_[i];
                if (buffer) {
                    uint64_t bufferSize = buffer->GetSize();
                    uint64_t usedSize = buffer->GetUsedSize();
                    uint64_t availableSize = buffer->GetAvailableSize();

                    totalStagingSize += bufferSize;
                    totalUsedSize += usedSize;
                    ++activeStagingBuffers;

                    logChannel_.LogFormat(Logging::LogLevel::Info,
                        "  Staging Buffer {}: {:.2f} MB total, {:.2f} MB used ({:.1f}%), {:.2f} MB available {}",
                        i, bufferSize / (1024.0 * 1024.0), usedSize / (1024.0 * 1024.0),
                        bufferSize > 0 ? (100.0 * usedSize) / bufferSize : 0.0,
                        availableSize / (1024.0 * 1024.0),
                        i == currentStagingBufferIndex_ ? "(CURRENT)" : "");
                }
                else {
                    logChannel_.LogFormat(Logging::LogLevel::Info,
                        "  Staging Buffer {}: NULL", i);
                }
            }

            logChannel_.LogFormat(Logging::LogLevel::Info,
                "Staging Summary: {} active buffers, {:.2f} MB total, {:.2f} MB used ({:.1f}%)",
                activeStagingBuffers, totalStagingSize / (1024.0 * 1024.0),
                totalUsedSize / (1024.0 * 1024.0),
                totalStagingSize > 0 ? (100.0 * totalUsedSize) / totalStagingSize : 0.0);
        }

        // Configuration
        logChannel_.LogFormat(Logging::LogLevel::Info,
            "Configuration: Pooling={}, MinSize={} KB, MaxSize={} MB, MaxPerPool={}",
            poolConfig_.enablePooling ? "Enabled" : "Disabled",
            poolConfig_.minBufferSize / 1024,
            poolConfig_.maxBufferSize / (1024 * 1024),
            poolConfig_.maxPooledBuffers);

        logChannel_.Log(Logging::LogLevel::Info, "=== End Detailed Statistics ===");
    }

    void D3D12BufferManager::LogMemoryUsage() const {
        if (!isInitialized_.load()) {
            logChannel_.Log(Logging::LogLevel::Warning,
                "Cannot log memory usage - buffer manager not initialized");
            return;
        }

        // Update and get current statistics
        const_cast<D3D12BufferManager*>(this)->UpdateGlobalStats();

        BufferPoolStats stats;
        {
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats = globalStats_;
        }

        // Get staging memory info
        uint64_t stagingMemoryTotal = GetTotalStagingMemorySize();
        uint64_t stagingMemoryUsed = GetUsedStagingMemorySize();

        // Calculate totals
        uint64_t totalAllocated = stats.totalMemoryAllocated + stagingMemoryTotal;
        uint64_t totalInUse = stats.totalMemoryInUse + stagingMemoryUsed;

        logChannel_.Log(Logging::LogLevel::Info, "=== D3D12 Buffer Manager Memory Usage ===");

        // Buffer memory
        logChannel_.LogFormat(Logging::LogLevel::Info,
            "Buffer Memory: Allocated={:.2f} MB, In Use={:.2f} MB, Peak={:.2f} MB",
            stats.totalMemoryAllocated / (1024.0 * 1024.0),
            stats.totalMemoryInUse / (1024.0 * 1024.0),
            stats.peakMemoryUsage / (1024.0 * 1024.0));

        // Staging memory
        logChannel_.LogFormat(Logging::LogLevel::Info,
            "Staging Memory: Total={:.2f} MB, Used={:.2f} MB ({:.1f}%)",
            stagingMemoryTotal / (1024.0 * 1024.0),
            stagingMemoryUsed / (1024.0 * 1024.0),
            stagingMemoryTotal > 0 ? (100.0 * stagingMemoryUsed) / stagingMemoryTotal : 0.0);

        // Combined totals
        logChannel_.LogFormat(Logging::LogLevel::Info,
            "Total Memory: Allocated={:.2f} MB, In Use={:.2f} MB ({:.1f}%)",
            totalAllocated / (1024.0 * 1024.0),
            totalInUse / (1024.0 * 1024.0),
            totalAllocated > 0 ? (100.0 * totalInUse) / totalAllocated : 0.0);

        // Buffer counts
        logChannel_.LogFormat(Logging::LogLevel::Info,
            "Buffer Counts: Active={}, Created={}, Destroyed={}, Pooled={}",
            stats.activeBuffers, stats.totalBuffersCreated,
            stats.totalBuffersDestroyed, stats.pooledBuffers);

        // Pool efficiency
        uint32_t totalPoolOperations = stats.poolHits + stats.poolMisses;
        logChannel_.LogFormat(Logging::LogLevel::Info,
            "Pool Efficiency: Hits={}, Misses={}, Hit Rate={:.1f}%",
            stats.poolHits, stats.poolMisses,
            totalPoolOperations > 0 ? (100.0 * stats.poolHits) / totalPoolOperations : 0.0);

        logChannel_.Log(Logging::LogLevel::Info, "=== End Memory Usage ===");
    }

} // namespace Akhanda::RHI::D3D12
