// D3D12Core.hpp
// Akhanda Game Engine - D3D12 Descriptor Heap Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include <d3d12.h>
#include <cstdint>

import Akhanda.Engine.RHI;

namespace Akhanda::RHI::D3D12 {
    uint32_t GetFormatSize(Format format);

    DXGI_FORMAT ConvertFormat(Format format);
    D3D12_RESOURCE_STATES ConvertResourceState(ResourceState state);
    D3D12_FILTER ConvertFilter(Filter filter);
    D3D12_TEXTURE_ADDRESS_MODE ConvertAddressMode(AddressMode mode);
    D3D12_COMPARISON_FUNC ConvertComparisonFunc(CompareFunction func);

    Format ConvertDXGIFormat(DXGI_FORMAT format);
    ResourceState ConvertD3D12ResourceState(D3D12_RESOURCE_STATES state);
    Filter ConvertD3D12Filter(D3D12_FILTER filter);
    AddressMode ConvertD3D12AddressMode(D3D12_TEXTURE_ADDRESS_MODE mode);
    CompareFunction ConvertD3D12ComparisonFunc(D3D12_COMPARISON_FUNC func);

    const char* ResourceUsageToString(ResourceUsage usage);
    const char* ResourceStateToString(ResourceState state);
    const char* D3D12ResourceStateToString(D3D12_RESOURCE_STATES state);
    const char* D3D12HeapTypeToString(D3D12_HEAP_TYPE heapType);
}