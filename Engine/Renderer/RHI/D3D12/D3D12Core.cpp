// Engine/Renderer/RHI/D3D12/D3D12Core.cpp
// Akhanda Game Engine - D3D12 Core Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#include "D3D12Core.hpp"
#include <string>

namespace Akhanda::RHI::D3D12 {

    uint32_t GetFormatSize(Format format) {
        switch (format) {
            // 32-bit formats (4 bytes per component)
        case Format::R32G32B32A32_Float:
        case Format::R32G32B32A32_UInt:
        case Format::R32G32B32A32_SInt:         return 16;

        case Format::R32G32B32_Float:
        case Format::R32G32B32_UInt:
        case Format::R32G32B32_SInt:            return 12;

        case Format::R32G32_Float:
        case Format::R32G32_UInt:
        case Format::R32G32_SInt:               return 8;

        case Format::R32_Float:
        case Format::R32_UInt:
        case Format::R32_SInt:
        case Format::D32_Float:                 return 4;

            // 16-bit formats (2 bytes per component)
        case Format::R16G16B16A16_Float:
        case Format::R16G16B16A16_UNorm:
        case Format::R16G16B16A16_SNorm:
        case Format::R16G16B16A16_UInt:
        case Format::R16G16B16A16_SInt:         return 8;

        case Format::R16G16_Float:
        case Format::R16G16_UNorm:
        case Format::R16G16_SNorm:
        case Format::R16G16_UInt:
        case Format::R16G16_SInt:               return 4;

        case Format::R16_Float:
        case Format::R16_UNorm:
        case Format::R16_SNorm:
        case Format::R16_UInt:
        case Format::R16_SInt:
        case Format::D16_UNorm:                 return 2;

            // 8-bit formats (1 byte per component)
        case Format::R8G8B8A8_UNorm:
        case Format::R8G8B8A8_UNorm_sRGB:
        case Format::R8G8B8A8_UInt:
        case Format::R8G8B8A8_SNorm:
        case Format::R8G8B8A8_SInt:
        case Format::B8G8R8A8_UNorm:
        case Format::B8G8R8A8_UNorm_sRGB:
        case Format::D24_UNorm_S8_UInt:         return 4;

            // Unknown/Undefined
        case Format::Undefined:
        default:
            return 0;
        }
    }



    // ========================================================================
    // Format Conversion Methods
    // ========================================================================

    DXGI_FORMAT ConvertFormat(Format format) {
        switch (format) {
        // 32-bit float formats
        case Format::R32G32B32A32_Float:    return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case Format::R32G32B32_Float:       return DXGI_FORMAT_R32G32B32_FLOAT;
        case Format::R32G32_Float:          return DXGI_FORMAT_R32G32_FLOAT;
        case Format::R32_Float:             return DXGI_FORMAT_R32_FLOAT;

        // 16-bit float formats
        case Format::R16G16B16A16_Float:    return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case Format::R16G16_Float:          return DXGI_FORMAT_R16G16_FLOAT;
        case Format::R16_Float:             return DXGI_FORMAT_R16_FLOAT;

        // 8-bit unsigned normalized formats
        case Format::R8G8B8A8_UNorm:        return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Format::R8G8B8A8_UNorm_sRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case Format::B8G8R8A8_UNorm:        return DXGI_FORMAT_B8G8R8A8_UNORM;
        case Format::B8G8R8A8_UNorm_sRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case Format::R8G8_UNorm:            return DXGI_FORMAT_R8G8_UNORM;
        case Format::R8_UNorm:              return DXGI_FORMAT_R8_UNORM;

        // 16-bit unsigned normalized formats  
        case Format::R16G16B16A16_UNorm:    return DXGI_FORMAT_R16G16B16A16_UNORM;
        case Format::R16G16_UNorm:          return DXGI_FORMAT_R16G16_UNORM;
        case Format::R16_UNorm:             return DXGI_FORMAT_R16_UNORM;

        // 32-bit unsigned integer formats
        case Format::R32G32B32A32_UInt:     return DXGI_FORMAT_R32G32B32A32_UINT;
        case Format::R32G32B32_UInt:        return DXGI_FORMAT_R32G32B32_UINT;
        case Format::R32G32_UInt:           return DXGI_FORMAT_R32G32_UINT;
        case Format::R32_UInt:              return DXGI_FORMAT_R32_UINT;

        // 32-bit signed integer formats
        case Format::R32G32B32A32_SInt:     return DXGI_FORMAT_R32G32B32A32_SINT;
        case Format::R32G32B32_SInt:        return DXGI_FORMAT_R32G32B32_SINT;
        case Format::R32G32_SInt:           return DXGI_FORMAT_R32G32_SINT;
        case Format::R32_SInt:              return DXGI_FORMAT_R32_SINT;

        // Special formats
        case Format::R11G11B10_Float:       return DXGI_FORMAT_R11G11B10_FLOAT;
        case Format::R10G10B10A2_UNorm:     return DXGI_FORMAT_R10G10B10A2_UNORM;
        case Format::R10G10B10A2_UInt:      return DXGI_FORMAT_R10G10B10A2_UINT;

        // Depth formats
        case Format::D32_Float:             return DXGI_FORMAT_D32_FLOAT;
        case Format::D24_UNorm_S8_UInt:     return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case Format::D16_UNorm:             return DXGI_FORMAT_D16_UNORM;
        case Format::D32_Float_S8X24_UInt:  return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

            // Block compression formats
        case Format::BC1_UNorm:             return DXGI_FORMAT_BC1_UNORM;
        case Format::BC1_UNorm_sRGB:        return DXGI_FORMAT_BC1_UNORM_SRGB;
        case Format::BC2_UNorm:             return DXGI_FORMAT_BC2_UNORM;
        case Format::BC2_UNorm_sRGB:        return DXGI_FORMAT_BC2_UNORM_SRGB;
        case Format::BC3_UNorm:             return DXGI_FORMAT_BC3_UNORM;
        case Format::BC3_UNorm_sRGB:        return DXGI_FORMAT_BC3_UNORM_SRGB;
        case Format::BC4_UNorm:             return DXGI_FORMAT_BC4_UNORM;
        case Format::BC4_SNorm:             return DXGI_FORMAT_BC4_SNORM;
        case Format::BC5_UNorm:             return DXGI_FORMAT_BC5_UNORM;
        case Format::BC5_SNorm:             return DXGI_FORMAT_BC5_SNORM;
        case Format::BC6H_UF16:             return DXGI_FORMAT_BC6H_UF16;
        case Format::BC6H_SF16:             return DXGI_FORMAT_BC6H_SF16;
        case Format::BC7_UNorm:             return DXGI_FORMAT_BC7_UNORM;
        case Format::BC7_UNorm_sRGB:        return DXGI_FORMAT_BC7_UNORM_SRGB;

        // Undefined format
        case Format::Unknown:               return DXGI_FORMAT_UNKNOWN;

        default:                            return DXGI_FORMAT_UNKNOWN;
        }
    }

    D3D12_FILTER ConvertFilter(Filter filter) {
        switch (filter) {
        case Filter::MinMagMipPoint:                        return D3D12_FILTER_MIN_MAG_MIP_POINT;
        case Filter::MinMagPointMipLinear:                  return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        case Filter::MinPointMagLinearMipPoint:             return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case Filter::MinPointMagMipLinear:                  return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
        case Filter::MinLinearMagMipPoint:                  return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        case Filter::MinLinearMagPointMipLinear:            return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case Filter::MinMagLinearMipPoint:                  return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        case Filter::MinMagMipLinear:                       return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        case Filter::Anisotropic:                           return D3D12_FILTER_ANISOTROPIC;

        // Comparison filters
        case Filter::ComparisonMinMagMipPoint:              return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        case Filter::ComparisonMinMagPointMipLinear:        return D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
        case Filter::ComparisonMinPointMagLinearMipPoint:   return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case Filter::ComparisonMinPointMagMipLinear:        return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
        case Filter::ComparisonMinLinearMagMipPoint:        return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
        case Filter::ComparisonMinLinearMagPointMipLinear:  return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case Filter::ComparisonMinMagLinearMipPoint:        return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        case Filter::ComparisonMinMagMipLinear:             return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        case Filter::ComparisonAnisotropic:                 return D3D12_FILTER_COMPARISON_ANISOTROPIC;

        // Minimum/Maximum filters (newer D3D12 features)
        case Filter::MinimumMinMagMipPoint:                 return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
        case Filter::MinimumMinMagPointMipLinear:           return D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
        case Filter::MinimumMinPointMagLinearMipPoint:      return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case Filter::MinimumMinPointMagMipLinear:           return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
        case Filter::MinimumMinLinearMagMipPoint:           return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
        case Filter::MinimumMinLinearMagPointMipLinear:     return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case Filter::MinimumMinMagLinearMipPoint:           return D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
        case Filter::MinimumMinMagMipLinear:                return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
        case Filter::MinimumAnisotropic:                    return D3D12_FILTER_MINIMUM_ANISOTROPIC;

        case Filter::MaximumMinMagMipPoint:                 return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
        case Filter::MaximumMinMagPointMipLinear:           return D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
        case Filter::MaximumMinPointMagLinearMipPoint:      return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case Filter::MaximumMinPointMagMipLinear:           return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
        case Filter::MaximumMinLinearMagMipPoint:           return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
        case Filter::MaximumMinLinearMagPointMipLinear:     return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case Filter::MaximumMinMagLinearMipPoint:           return D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
        case Filter::MaximumMinMagMipLinear:                return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
        case Filter::MaximumAnisotropic:                    return D3D12_FILTER_MAXIMUM_ANISOTROPIC;

        default:                                            return D3D12_FILTER_MIN_MAG_MIP_POINT; // Safe fallback
        }
    }

    D3D12_TEXTURE_ADDRESS_MODE ConvertAddressMode(AddressMode mode) {
        switch (mode) {
        case AddressMode::Wrap:         return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case AddressMode::Mirror:       return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case AddressMode::Clamp:        return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case AddressMode::Border:       return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case AddressMode::MirrorOnce:   return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;

        default:                        return D3D12_TEXTURE_ADDRESS_MODE_CLAMP; // Safe fallback
        }
    }

    D3D12_COMPARISON_FUNC ConvertComparisonFunc(CompareFunction func) {
        switch (func) {
        case CompareFunction::Never:        return D3D12_COMPARISON_FUNC_NEVER;
        case CompareFunction::Less:         return D3D12_COMPARISON_FUNC_LESS;
        case CompareFunction::Equal:        return D3D12_COMPARISON_FUNC_EQUAL;
        case CompareFunction::LessEqual:    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case CompareFunction::Greater:      return D3D12_COMPARISON_FUNC_GREATER;
        case CompareFunction::NotEqual:     return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case CompareFunction::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case CompareFunction::Always:       return D3D12_COMPARISON_FUNC_ALWAYS;

        default:                            return D3D12_COMPARISON_FUNC_ALWAYS; // Safe fallback
        }
    }

    D3D12_RESOURCE_STATES ConvertResourceState(ResourceState state) {
        switch (state) {
        case ResourceState::Common:                     return D3D12_RESOURCE_STATE_COMMON;
        case ResourceState::VertexAndConstantBuffer:    return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case ResourceState::IndexBuffer:                return D3D12_RESOURCE_STATE_INDEX_BUFFER;
        case ResourceState::RenderTarget:               return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case ResourceState::UnorderedAccess:            return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case ResourceState::DepthWrite:                 return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case ResourceState::DepthRead:                  return D3D12_RESOURCE_STATE_DEPTH_READ;
        case ResourceState::ShaderResource:             return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case ResourceState::StreamOut:                  return D3D12_RESOURCE_STATE_STREAM_OUT;
        case ResourceState::IndirectArgument:           return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        case ResourceState::CopySource:                 return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case ResourceState::ResolveDest:                return D3D12_RESOURCE_STATE_RESOLVE_DEST;
        case ResourceState::ResolveSource:              return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        default:                                        return D3D12_RESOURCE_STATE_COMMON;
        }
    }
    
    Format ConvertDXGIFormat(DXGI_FORMAT format) {
        switch (format) {
        // 32-bit float formats
        case DXGI_FORMAT_R32G32B32A32_FLOAT:    return Format::R32G32B32A32_Float;
        case DXGI_FORMAT_R32G32B32_FLOAT:       return Format::R32G32B32_Float;
        case DXGI_FORMAT_R32G32_FLOAT:          return Format::R32G32_Float;
        case DXGI_FORMAT_R32_FLOAT:             return Format::R32_Float;

        // 16-bit float formats
        case DXGI_FORMAT_R16G16B16A16_FLOAT:    return Format::R16G16B16A16_Float;
        case DXGI_FORMAT_R16G16_FLOAT:          return Format::R16G16_Float;
        case DXGI_FORMAT_R16_FLOAT:             return Format::R16_Float;

        // 8-bit unsigned normalized formats
        case DXGI_FORMAT_R8G8B8A8_UNORM:        return Format::R8G8B8A8_UNorm;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return Format::R8G8B8A8_UNorm_sRGB;
        case DXGI_FORMAT_B8G8R8A8_UNORM:        return Format::B8G8R8A8_UNorm;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return Format::B8G8R8A8_UNorm_sRGB;
        case DXGI_FORMAT_R8G8_UNORM:            return Format::R8G8_UNorm;
        case DXGI_FORMAT_R8_UNORM:              return Format::R8_UNorm;

        // 16-bit unsigned normalized formats  
        case DXGI_FORMAT_R16G16B16A16_UNORM:    return Format::R16G16B16A16_UNorm;
        case DXGI_FORMAT_R16G16_UNORM:          return Format::R16G16_UNorm;
        case DXGI_FORMAT_R16_UNORM:             return Format::R16_UNorm;

        // 32-bit unsigned integer formats
        case DXGI_FORMAT_R32G32B32A32_UINT:     return Format::R32G32B32A32_UInt;
        case DXGI_FORMAT_R32G32B32_UINT:        return Format::R32G32B32_UInt;
        case DXGI_FORMAT_R32G32_UINT:           return Format::R32G32_UInt;
        case DXGI_FORMAT_R32_UINT:              return Format::R32_UInt;

        // 32-bit signed integer formats
        case DXGI_FORMAT_R32G32B32A32_SINT:     return Format::R32G32B32A32_SInt;
        case DXGI_FORMAT_R32G32B32_SINT:        return Format::R32G32B32_SInt;
        case DXGI_FORMAT_R32G32_SINT:           return Format::R32G32_SInt;
        case DXGI_FORMAT_R32_SINT:              return Format::R32_SInt;

        // Special formats
        case DXGI_FORMAT_R11G11B10_FLOAT:       return Format::R11G11B10_Float;
        case DXGI_FORMAT_R10G10B10A2_UNORM:     return Format::R10G10B10A2_UNorm;
        case DXGI_FORMAT_R10G10B10A2_UINT:      return Format::R10G10B10A2_UInt;

        // Depth formats
        case DXGI_FORMAT_D32_FLOAT:             return Format::D32_Float;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:     return Format::D24_UNorm_S8_UInt;
        case DXGI_FORMAT_D16_UNORM:             return Format::D16_UNorm;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:  return Format::D32_Float_S8X24_UInt;

        // Block compression formats
        case DXGI_FORMAT_BC1_UNORM:             return Format::BC1_UNorm;
        case DXGI_FORMAT_BC1_UNORM_SRGB:        return Format::BC1_UNorm_sRGB;
        case DXGI_FORMAT_BC2_UNORM:             return Format::BC2_UNorm;
        case DXGI_FORMAT_BC2_UNORM_SRGB:        return Format::BC2_UNorm_sRGB;
        case DXGI_FORMAT_BC3_UNORM:             return Format::BC3_UNorm;
        case DXGI_FORMAT_BC3_UNORM_SRGB:        return Format::BC3_UNorm_sRGB;
        case DXGI_FORMAT_BC4_UNORM:             return Format::BC4_UNorm;
        case DXGI_FORMAT_BC4_SNORM:             return Format::BC4_SNorm;
        case DXGI_FORMAT_BC5_UNORM:             return Format::BC5_UNorm;
        case DXGI_FORMAT_BC5_SNORM:             return Format::BC5_SNorm;
        case DXGI_FORMAT_BC6H_UF16:             return Format::BC6H_UF16;
        case DXGI_FORMAT_BC6H_SF16:             return Format::BC6H_SF16;
        case DXGI_FORMAT_BC7_UNORM:             return Format::BC7_UNorm;
        case DXGI_FORMAT_BC7_UNORM_SRGB:        return Format::BC7_UNorm_sRGB;

        // Undefined format
        case DXGI_FORMAT_UNKNOWN:               return Format::Unknown;
        default:                                return Format::Unknown;
        }
    }

    Filter ConvertD3D12Filter(D3D12_FILTER filter) {
        switch (filter) {
        case D3D12_FILTER_MIN_MAG_MIP_POINT:                            return Filter::MinMagMipPoint;
        case D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR:                     return Filter::MinMagPointMipLinear;
        case D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:               return Filter::MinPointMagLinearMipPoint;
        case D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR:                     return Filter::MinPointMagMipLinear;
        case D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT:                     return Filter::MinLinearMagMipPoint;
        case D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:              return Filter::MinLinearMagPointMipLinear;
        case D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT:                     return Filter::MinMagLinearMipPoint;
        case D3D12_FILTER_MIN_MAG_MIP_LINEAR:                           return Filter::MinMagMipLinear;
        case D3D12_FILTER_ANISOTROPIC:                                  return Filter::Anisotropic;

        // Comparison filters
        case D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT:                 return Filter::ComparisonMinMagMipPoint;
        case D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:          return Filter::ComparisonMinMagPointMipLinear;
        case D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:    return Filter::ComparisonMinPointMagLinearMipPoint;
        case D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:          return Filter::ComparisonMinPointMagMipLinear;
        case D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:          return Filter::ComparisonMinLinearMagMipPoint;
        case D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:   return Filter::ComparisonMinLinearMagPointMipLinear;
        case D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:          return Filter::ComparisonMinMagLinearMipPoint;
        case D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:                return Filter::ComparisonMinMagMipLinear;
        case D3D12_FILTER_COMPARISON_ANISOTROPIC:                       return Filter::ComparisonAnisotropic;

        // Minimum/Maximum filters (newer D3D12 features)
        case D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT:                    return Filter::MinimumMinMagMipPoint;
        case D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:             return Filter::MinimumMinMagPointMipLinear;
        case D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:       return Filter::MinimumMinPointMagLinearMipPoint;
        case D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:             return Filter::MinimumMinPointMagMipLinear;
        case D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:             return Filter::MinimumMinLinearMagMipPoint;
        case D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:      return Filter::MinimumMinLinearMagPointMipLinear;
        case D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:             return Filter::MinimumMinMagLinearMipPoint;
        case D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR:                   return Filter::MinimumMinMagMipLinear;
        case D3D12_FILTER_MINIMUM_ANISOTROPIC:                          return Filter::MinimumAnisotropic;

        case D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT:                    return Filter::MaximumMinMagMipPoint;
        case D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:             return Filter::MaximumMinMagPointMipLinear;
        case D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:       return Filter::MaximumMinPointMagLinearMipPoint;
        case D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:             return Filter::MaximumMinPointMagMipLinear;
        case D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:             return Filter::MaximumMinLinearMagMipPoint;
        case D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:      return Filter::MaximumMinLinearMagPointMipLinear;
        case D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:             return Filter::MaximumMinMagLinearMipPoint;
        case D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR:                   return Filter::MaximumMinMagMipLinear;
        case D3D12_FILTER_MAXIMUM_ANISOTROPIC:                          return Filter::MaximumAnisotropic;

        default:                                                        return Filter::MinMagMipPoint; // Safe fallback
        }
    }
    
    AddressMode ConvertD3D12AddressMode(D3D12_TEXTURE_ADDRESS_MODE mode) {
        switch (mode) {
        case D3D12_TEXTURE_ADDRESS_MODE_WRAP:           return AddressMode::Wrap;
        case D3D12_TEXTURE_ADDRESS_MODE_MIRROR:         return AddressMode::Mirror;
        case D3D12_TEXTURE_ADDRESS_MODE_CLAMP:          return AddressMode::Clamp;
        case D3D12_TEXTURE_ADDRESS_MODE_BORDER:         return AddressMode::Border;
        case D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE:    return AddressMode::MirrorOnce;

        default:                                        return AddressMode::Clamp; // Safe fallback
        }
    }
    
    CompareFunction ConvertD3D12ComparisonFunc(D3D12_COMPARISON_FUNC func) {
        switch (func) {
        case D3D12_COMPARISON_FUNC_NEVER:           return CompareFunction::Never;
        case D3D12_COMPARISON_FUNC_LESS:            return CompareFunction::Less;
        case D3D12_COMPARISON_FUNC_EQUAL:           return CompareFunction::Equal;
        case D3D12_COMPARISON_FUNC_LESS_EQUAL:      return CompareFunction::LessEqual;
        case D3D12_COMPARISON_FUNC_GREATER:         return CompareFunction::Greater;
        case D3D12_COMPARISON_FUNC_NOT_EQUAL:       return CompareFunction::NotEqual;
        case D3D12_COMPARISON_FUNC_GREATER_EQUAL:   return CompareFunction::GreaterEqual;
        case D3D12_COMPARISON_FUNC_ALWAYS:          return CompareFunction::Always;

        default:                                    return CompareFunction::Always; // Safe fallback
        }
    }

    ResourceState ConvertD3D12ResourceState(D3D12_RESOURCE_STATES state) {
        switch (state) {
        case D3D12_RESOURCE_STATE_COMMON:                       return ResourceState::Common;
        case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:   return ResourceState::VertexAndConstantBuffer;
        case D3D12_RESOURCE_STATE_INDEX_BUFFER:                 return ResourceState::IndexBuffer;
        case D3D12_RESOURCE_STATE_RENDER_TARGET:                return ResourceState::RenderTarget;
        case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:             return ResourceState::UnorderedAccess;
        case D3D12_RESOURCE_STATE_DEPTH_WRITE:                  return ResourceState::DepthWrite;
        case D3D12_RESOURCE_STATE_DEPTH_READ:                   return ResourceState::DepthRead;
        case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:        return ResourceState::ShaderResource;
        case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:    return ResourceState::ShaderResource;
        case D3D12_RESOURCE_STATE_STREAM_OUT:                   return ResourceState::StreamOut;
        case D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT:            return ResourceState::IndirectArgument;
        case D3D12_RESOURCE_STATE_COPY_SOURCE:                  return ResourceState::CopySource;
        case D3D12_RESOURCE_STATE_RESOLVE_DEST:                 return ResourceState::ResolveDest;
        case D3D12_RESOURCE_STATE_RESOLVE_SOURCE:               return ResourceState::ResolveSource;
        default:                                                return ResourceState::Common;
        }
    }


    const char* ResourceUsageToString(ResourceUsage usage) {
        static const char* usageStrings[] = {
            "None",
            "VertexBuffer",
            "IndexBuffer",
            "ConstantBuffer",
            "ShaderResource",
            "RenderTarget",
            "DepthStencil",
            "UnorderedAccess",
            "CopySource",
            "CopyDestination"
        };

        // Handle combined flags
        static std::string combinedString;
        combinedString.clear();

        bool first = true;
        uint32_t usageValue = static_cast<uint32_t>(usage);

        for (uint32_t i = 0; i < sizeof(usageStrings) / sizeof(usageStrings[0]); ++i) {
            uint32_t flag = 1u << i;
            if ((usageValue & flag) != 0) {
                if (!first) {
                    combinedString += " | ";
                }
                combinedString += usageStrings[i];
                first = false;
            }
        }

        if (combinedString.empty()) {
            return "None";
        }

        return combinedString.c_str();
    }

    const char* ResourceStateToString(ResourceState state) {
        switch (state) {
        case ResourceState::Common:                     return "Common";
        case ResourceState::VertexAndConstantBuffer:   return "VertexAndConstantBuffer";
        case ResourceState::IndexBuffer:               return "IndexBuffer";
        case ResourceState::RenderTarget:              return "RenderTarget";
        case ResourceState::UnorderedAccess:           return "UnorderedAccess";
        case ResourceState::DepthWrite:                return "DepthWrite";
        case ResourceState::DepthRead:                 return "DepthRead";
        case ResourceState::ShaderResource:            return "ShaderResource";
        case ResourceState::StreamOut:                 return "StreamOut";
        case ResourceState::IndirectArgument:          return "IndirectArgument";
        case ResourceState::CopyDestination:           return "CopyDestination";
        case ResourceState::CopySource:                return "CopySource";
        case ResourceState::ResolveDest:               return "ResolveDest";
        case ResourceState::ResolveSource:             return "ResolveSource";
        case ResourceState::Present:                   return "Present";
        case ResourceState::Predication:               return "Predication";
        default:                                        return "Unknown";
        }
    }

    const char* D3D12ResourceStateToString(D3D12_RESOURCE_STATES state) {
        switch (state) {
        case D3D12_RESOURCE_STATE_COMMON:                       return "D3D12_RESOURCE_STATE_COMMON";
        case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:   return "D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER";
        case D3D12_RESOURCE_STATE_INDEX_BUFFER:                 return "D3D12_RESOURCE_STATE_INDEX_BUFFER";
        case D3D12_RESOURCE_STATE_RENDER_TARGET:                return "D3D12_RESOURCE_STATE_RENDER_TARGET";
        case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:             return "D3D12_RESOURCE_STATE_UNORDERED_ACCESS";
        case D3D12_RESOURCE_STATE_DEPTH_WRITE:                  return "D3D12_RESOURCE_STATE_DEPTH_WRITE";
        case D3D12_RESOURCE_STATE_DEPTH_READ:                   return "D3D12_RESOURCE_STATE_DEPTH_READ";
        case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:        return "D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE";
        case D3D12_RESOURCE_STATE_STREAM_OUT:                   return "D3D12_RESOURCE_STATE_STREAM_OUT";
        case D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT:            return "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT";
        case D3D12_RESOURCE_STATE_COPY_DEST:                    return "D3D12_RESOURCE_STATE_COPY_DEST";
        case D3D12_RESOURCE_STATE_COPY_SOURCE:                  return "D3D12_RESOURCE_STATE_COPY_SOURCE";
        case D3D12_RESOURCE_STATE_RESOLVE_DEST:                 return "D3D12_RESOURCE_STATE_RESOLVE_DEST";
        case D3D12_RESOURCE_STATE_RESOLVE_SOURCE:               return "D3D12_RESOURCE_STATE_RESOLVE_SOURCE";
        default:                                                return "Unknown D3D12 Resource State";
        }
    }

    const char* D3D12HeapTypeToString(D3D12_HEAP_TYPE heapType) {
        switch (heapType) {
        case D3D12_HEAP_TYPE_DEFAULT:   return "D3D12_HEAP_TYPE_DEFAULT";
        case D3D12_HEAP_TYPE_UPLOAD:    return "D3D12_HEAP_TYPE_UPLOAD";
        case D3D12_HEAP_TYPE_READBACK:  return "D3D12_HEAP_TYPE_READBACK";
        case D3D12_HEAP_TYPE_CUSTOM:    return "D3D12_HEAP_TYPE_CUSTOM";
        default:                        return "Unknown D3D12 Heap Type";
        }
    }
}