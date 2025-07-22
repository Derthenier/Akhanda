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

    bool D3D12BufferHandleManager::IsValid(BufferHandle handle) {
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

} // namespace Akhanda::RHI::D3D12
