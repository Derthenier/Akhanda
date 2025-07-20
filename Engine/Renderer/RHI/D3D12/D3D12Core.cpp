#include "D3D12Core.hpp"


namespace Akhanda::RHI::D3D12 {
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
        case Filter::MinMagMipPoint:
            return D3D12_FILTER_MIN_MAG_MIP_POINT;
        case Filter::MinMagPointMipLinear:
            return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        case Filter::MinPointMagLinearMipPoint:
            return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case Filter::MinPointMagMipLinear:
            return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
        case Filter::MinLinearMagMipPoint:
            return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        case Filter::MinLinearMagPointMipLinear:
            return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case Filter::MinMagLinearMipPoint:
            return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        case Filter::MinMagMipLinear:
            return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        case Filter::Anisotropic:
            return D3D12_FILTER_ANISOTROPIC;

            // Comparison filters
        case Filter::ComparisonMinMagMipPoint:
            return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        case Filter::ComparisonMinMagPointMipLinear:
            return D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
        case Filter::ComparisonMinPointMagLinearMipPoint:
            return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case Filter::ComparisonMinPointMagMipLinear:
            return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
        case Filter::ComparisonMinLinearMagMipPoint:
            return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
        case Filter::ComparisonMinLinearMagPointMipLinear:
            return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case Filter::ComparisonMinMagLinearMipPoint:
            return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        case Filter::ComparisonMinMagMipLinear:
            return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        case Filter::ComparisonAnisotropic:
            return D3D12_FILTER_COMPARISON_ANISOTROPIC;

            // Minimum/Maximum filters (newer D3D12 features)
        case Filter::MinimumMinMagMipPoint:
            return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
        case Filter::MinimumMinMagPointMipLinear:
            return D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
        case Filter::MinimumMinPointMagLinearMipPoint:
            return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case Filter::MinimumMinPointMagMipLinear:
            return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
        case Filter::MinimumMinLinearMagMipPoint:
            return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
        case Filter::MinimumMinLinearMagPointMipLinear:
            return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case Filter::MinimumMinMagLinearMipPoint:
            return D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
        case Filter::MinimumMinMagMipLinear:
            return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
        case Filter::MinimumAnisotropic:
            return D3D12_FILTER_MINIMUM_ANISOTROPIC;

        case Filter::MaximumMinMagMipPoint:
            return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
        case Filter::MaximumMinMagPointMipLinear:
            return D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
        case Filter::MaximumMinPointMagLinearMipPoint:
            return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case Filter::MaximumMinPointMagMipLinear:
            return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
        case Filter::MaximumMinLinearMagMipPoint:
            return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
        case Filter::MaximumMinLinearMagPointMipLinear:
            return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case Filter::MaximumMinMagLinearMipPoint:
            return D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
        case Filter::MaximumMinMagMipLinear:
            return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
        case Filter::MaximumAnisotropic:
            return D3D12_FILTER_MAXIMUM_ANISOTROPIC;

        default:
            return D3D12_FILTER_MIN_MAG_MIP_POINT; // Safe fallback
        }
    }

    D3D12_TEXTURE_ADDRESS_MODE ConvertAddressMode(AddressMode mode) {
        switch (mode) {
        case AddressMode::Wrap:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case AddressMode::Mirror:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case AddressMode::Clamp:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case AddressMode::Border:
            return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case AddressMode::MirrorOnce:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;

        default:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP; // Safe fallback
        }
    }

    D3D12_COMPARISON_FUNC ConvertComparisonFunc(CompareFunction func) {
        switch (func) {
        case CompareFunction::Never:
            return D3D12_COMPARISON_FUNC_NEVER;
        case CompareFunction::Less:
            return D3D12_COMPARISON_FUNC_LESS;
        case CompareFunction::Equal:
            return D3D12_COMPARISON_FUNC_EQUAL;
        case CompareFunction::LessEqual:
            return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case CompareFunction::Greater:
            return D3D12_COMPARISON_FUNC_GREATER;
        case CompareFunction::NotEqual:
            return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case CompareFunction::GreaterEqual:
            return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case CompareFunction::Always:
            return D3D12_COMPARISON_FUNC_ALWAYS;

        default:
            return D3D12_COMPARISON_FUNC_ALWAYS; // Safe fallback
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
            //case ResourceState::NonPixelShaderResource: return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            //case ResourceState::PixelShaderResource: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case ResourceState::ShaderResource: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case ResourceState::StreamOut: return D3D12_RESOURCE_STATE_STREAM_OUT;
        case ResourceState::IndirectArgument: return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
            //case ResourceState::CopyDest: return D3D12_RESOURCE_STATE_COPY_DEST;
        case ResourceState::CopySource: return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case ResourceState::ResolveDest: return D3D12_RESOURCE_STATE_RESOLVE_DEST;
        case ResourceState::ResolveSource: return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
            //case ResourceState::RaytracingAccelerationStructure: return D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
            //case ResourceState::ShadingRateSource: return D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
        default: return D3D12_RESOURCE_STATE_COMMON;
        }
    }
}