// D3D12Core.hpp
// Akhanda Game Engine - D3D12 Descriptor Heap Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include <d3d12.h>

import Akhanda.Engine.RHI;

namespace Akhanda::RHI::D3D12 {
    DXGI_FORMAT ConvertFormat(Format format);
    D3D12_RESOURCE_STATES ConvertResourceState(ResourceState state);
    D3D12_FILTER ConvertFilter(Filter filter);
    D3D12_TEXTURE_ADDRESS_MODE ConvertAddressMode(AddressMode mode);
    D3D12_COMPARISON_FUNC ConvertComparisonFunc(CompareFunction func);
}