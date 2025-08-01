// Engine/Renderer/RHI/D3D12/D3D12Pipeline.cpp
// Akhanda Game Engine - D3D12 Pipeline Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.


#include "D3D12Pipeline.hpp"

import Akhanda.Core.Logging;
import Akhanda.Engine.RHI;

namespace Akhanda::RHI::D3D12 {

    // Stub implementation of D3D12Pipeline constructor
    D3D12Pipeline::D3D12Pipeline()
        : logChannel_(Logging::LogManager::Instance().GetChannel("D3D12Pipeline")) {
        logChannel_.Log(Logging::LogLevel::Info, "D3D12Pipeline stub constructor called");
    }
    D3D12Pipeline::~D3D12Pipeline() {
    }

    // Stub implementation of InitializeWithDevice for graphics pipeline
    bool D3D12Pipeline::InitializeWithDevice(const GraphicsPipelineDesc& desc, ID3D12Device* device) {
        logChannel_.Log(Logging::LogLevel::Warning,
            "D3D12Pipeline::InitializeWithDevice(GraphicsPipelineDesc) - STUB IMPLEMENTATION");

        // For now, just return false to indicate not implemented
        // This will cause pipeline creation to fail gracefully
        return false;
    }

    // Stub implementation of InitializeWithDevice for compute pipeline
    bool D3D12Pipeline::InitializeWithDevice(const ComputePipelineDesc& desc, ID3D12Device* device) {
        logChannel_.Log(Logging::LogLevel::Warning,
            "D3D12Pipeline::InitializeWithDevice(ComputePipelineDesc) - STUB IMPLEMENTATION");

        // For now, just return false to indicate not implemented
        return false;
    }


    bool D3D12Pipeline::Initialize(const ComputePipelineDesc& desc)
    {
        return false;
    }

    bool D3D12Pipeline::Initialize(const GraphicsPipelineDesc& desc)
    {
        return false;
    }

    void D3D12Pipeline::Shutdown()
    {
        
    }

    void D3D12Pipeline::SetDebugName(const char* name)
    {
        
    }


} // namespace Akhanda::RHI::D3D12