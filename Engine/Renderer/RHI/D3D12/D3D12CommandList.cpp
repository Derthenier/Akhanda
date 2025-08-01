// Engine/Renderer/RHI/D3D12/D3D12CommandList.cpp
// Akhanda Game Engine - D3D12 Command List Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#include "D3D12CommandList.hpp"

import Akhanda.Core.Logging;

namespace Akhanda::RHI::D3D12 {

    D3D12CommandList::D3D12CommandList(CommandListType type)
        : logChannel_(Logging::LogManager::Instance().GetChannel("D3D12CommandList")) {
        switch (type)
        {
        case CommandListType::Bundle:
            logChannel_.Log(Logging::LogLevel::Info, "D3D12CommandList stub constructor called with type: CommandListType::Bundle");
            break;

        case CommandListType::Compute:
            logChannel_.Log(Logging::LogLevel::Info, "D3D12CommandList stub constructor called with type: CommandListType::Compute");
            break;

        case CommandListType::Copy:
            logChannel_.Log(Logging::LogLevel::Info, "D3D12CommandList stub constructor called with type: CommandListType::Copy");
            break;

        case CommandListType::Direct:
            logChannel_.Log(Logging::LogLevel::Info, "D3D12CommandList stub constructor called with type: CommandListType::Direct");
            break;
        }
    }


    D3D12CommandList::~D3D12CommandList() {}

    // ICommandList implementation
    bool D3D12CommandList::Initialize(CommandListType type) { return false; }
    void D3D12CommandList::Shutdown() {}
    void D3D12CommandList::Reset() {}
    void D3D12CommandList::Close() {}

    void D3D12CommandList::SetPipeline(PipelineHandle pipeline) {}
    void D3D12CommandList::SetRootSignature(const RootSignatureDesc& desc) {}
    void D3D12CommandList::SetViewports(uint32_t count, const Viewport* viewports) {}
    void D3D12CommandList::SetScissorRects(uint32_t count, const Scissor* scissors) {}
    void D3D12CommandList::SetPrimitiveTopology(PrimitiveTopology topology) {}

    void D3D12CommandList::SetVertexBuffers(uint32_t startSlot, uint32_t count,
        const BufferHandle* buffers,
        const uint64_t* offsets) {
    }
    void D3D12CommandList::SetIndexBuffer(BufferHandle buffer, Format indexFormat,
        uint64_t offse) {
    }
    void D3D12CommandList::SetConstantBuffer(uint32_t rootParameterIndex,
        BufferHandle buffer) {
    }
    void D3D12CommandList::SetShaderResource(uint32_t rootParameterIndex,
        TextureHandle texture) {
    }
    void D3D12CommandList::SetUnorderedAccess(uint32_t rootParameterIndex,
        TextureHandle texture) {
    }
    void D3D12CommandList::SetConstants(uint32_t rootParameterIndex,
        uint32_t numConstants,
        const void* constants) {
    }

    void D3D12CommandList::SetRenderTargets(uint32_t count,
        const TextureHandle* renderTargets,
        TextureHandle depthStencil) {
    }
    void D3D12CommandList::ClearRenderTarget(TextureHandle renderTarget,
        const float color[4]) {
    }
    void D3D12CommandList::ClearDepthStencil(TextureHandle depthStencil,
        bool clearDepth,
        bool clearStencil,
        float depth,
        uint8_t stencil) {
    }

    void D3D12CommandList::Draw(uint32_t vertexCount, uint32_t startVertex) {}
    void D3D12CommandList::DrawIndexed(uint32_t indexCount, uint32_t startIndex,
        int32_t baseVertex) {
    }
    void D3D12CommandList::DrawInstanced(uint32_t vertexCountPerInstance,
        uint32_t instanceCount,
        uint32_t startVertex,
        uint32_t startInstance) {
    }
    void D3D12CommandList::DrawIndexedInstanced(uint32_t indexCountPerInstance,
        uint32_t instanceCount,
        uint32_t startIndex,
        int32_t baseVertex,
        uint32_t startInstance) {
    }

    void D3D12CommandList::Dispatch(uint32_t threadGroupCountX,
        uint32_t threadGroupCountY,
        uint32_t threadGroupCountZ) {
    }

    void D3D12CommandList::TransitionResource(TextureHandle texture,
        ResourceState fromState,
        ResourceState toState) {
    }
    void D3D12CommandList::TransitionResource(BufferHandle buffer,
        ResourceState fromState,
        ResourceState toState) {
    }

    void D3D12CommandList::ResourceBarrier() {}
    void D3D12CommandList::UAVBarrier(TextureHandle texture) {}
    void D3D12CommandList::UAVBarrier(BufferHandle buffer) {}

    void D3D12CommandList::CopyBuffer(BufferHandle dest, BufferHandle src,
        uint64_t destOffset,
        uint64_t srcOffset,
        uint64_t size) {
    }
    void D3D12CommandList::CopyTexture(TextureHandle dest, TextureHandle src) {}
    void D3D12CommandList::CopyBufferToTexture(TextureHandle dest, BufferHandle src,
        uint32_t srcOffset) {
    }
    void D3D12CommandList::CopyTextureToBuffer(BufferHandle dest, TextureHandle src,
        uint32_t destOffset) {
    }

    void D3D12CommandList::BeginEvent(const char* name) {}
    void D3D12CommandList::EndEvent() {}
    void D3D12CommandList::SetMarker(const char* name) {}
    void D3D12CommandList::SetDebugName(const char* name) { }


} // namespace Akhanda::RHI::D3D12