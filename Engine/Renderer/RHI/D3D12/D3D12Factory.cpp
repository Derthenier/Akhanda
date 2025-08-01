// Engine/Renderer/RHI/D3D12/D3D12Factory.cpp
// Akhanda Game Engine - D3D12 Factory Implementation
// Integrates with updated AdapterInfo and DeviceCapabilities structs
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#include "D3D12Device.hpp"
#include "D3D12Core.hpp"

#include <dxgidebug.h>
#include <comdef.h>
#include <format>
#include <algorithm>

import Akhanda.Engine.RHI;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Platform.Interfaces;
import Akhanda.Platform.Windows;
import Akhanda.Platform.RendererIntegration;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Core.Logging;
import Akhanda.Core.Memory;

using Microsoft::WRL::ComPtr;

namespace Akhanda::RHI::D3D12 {

    using namespace Logging;
    using namespace Configuration;
    using namespace Platform;

    // ========================================================================
    // D3D12Factory Implementation - Complete implementation using updated structs
    // ========================================================================

    D3D12Factory::D3D12Factory()
        : logChannel_(LogManager::Instance().GetChannel("D3D12Factory")) {

        logChannel_.Log(LogLevel::Info, "Initializing D3D12 Factory");
        debugName_ = "D3D12Factory";
    }

    D3D12Factory::~D3D12Factory() {
        Shutdown();
    }

    bool D3D12Factory::Initialize() {
        logChannel_.Log(LogLevel::Info, "Starting D3D12 Factory initialization");

        // Create DXGI Factory
        UINT dxgiFlags = 0;

#ifdef _DEBUG
        // Enable DXGI debug layer in debug builds
        ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue)))) {
            dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
            logChannel_.Log(LogLevel::Info, "DXGI debug layer enabled");
        }
#endif

        HRESULT hr = CreateDXGIFactory2(dxgiFlags, IID_PPV_ARGS(&dxgiFactory_));
        if (FAILED(hr)) {
            _com_error err(hr);
            std::string errorMsg = Platform::Windows::WideToUtf8(err.ErrorMessage());
            logChannel_.LogFormat(LogLevel::Error,
                "Failed to create DXGI Factory: {} (HRESULT: 0x{:08X})",
                errorMsg, static_cast<uint32_t>(hr));
            return false;
        }

        // Create platform integration
        platformIntegration_ = Platform::CreateRendererPlatformIntegration();
        if (!platformIntegration_) {
            logChannel_.Log(LogLevel::Error, "Failed to create platform integration");
            return false;
        }

        // Enumerate available adapters
        EnumerateAdapters();

        logChannel_.LogFormat(LogLevel::Info,
            "D3D12 Factory initialized successfully with {} adapters", adapters_.size());
        return true;
    }

    void D3D12Factory::Shutdown() {
        logChannel_.Log(LogLevel::Info, "Shutting down D3D12 Factory");

        adapters_.clear();

        if (platformIntegration_) {
            platformIntegration_.reset();
        }

        dxgiFactory_.Reset();

        logChannel_.Log(LogLevel::Info, "D3D12 Factory shutdown complete");
    }

    std::unique_ptr<IRenderDevice> D3D12Factory::CreateDevice(
        const RenderingConfig& config,
        const SurfaceInfo& surfaceInfo) {

        logChannel_.Log(LogLevel::Info, "Creating D3D12 render device");

        if (adapters_.empty()) {
            logChannel_.Log(LogLevel::Error, "No suitable adapters available for device creation");
            return nullptr;
        }

        // Find the best D3D12 compatible adapter
        const AdapterInfo* selectedAdapter = nullptr;
        for (const auto& adapter : adapters_) {
            if (adapter.isD3D12Compatible) {
                selectedAdapter = &adapter;
                break; // Adapters are already sorted by preference
            }
        }

        if (!selectedAdapter) {
            logChannel_.Log(LogLevel::Error, "No D3D12 compatible adapters found");
            return nullptr;
        }

        logChannel_.LogFormat(LogLevel::Info, "Selected adapter: {} (Index: {})",
            selectedAdapter->name, selectedAdapter->index);

        // Create the D3D12Device using your existing implementation
        auto device = std::make_unique<D3D12Device>();
        if (!device) {
            logChannel_.Log(LogLevel::Error, "Failed to allocate D3D12 device");
            return nullptr;
        }

        // Initialize the device with the provided configuration
        if (!device->Initialize(config, surfaceInfo)) {
            logChannel_.Log(LogLevel::Error, "Failed to initialize D3D12 device");
            return nullptr;
        }

        logChannel_.Log(LogLevel::Info, "D3D12 render device created successfully");
        return std::unique_ptr<IRenderDevice>(device.release());
    }

    bool D3D12Factory::IsSupported() const {
        // Check if we have a valid DXGI factory and at least one compatible adapter
        if (!dxgiFactory_) {
            return false;
        }

        // Check if we have any D3D12 compatible adapters
        for (const auto& adapter : adapters_) {
            if (adapter.isD3D12Compatible) {
                return true;
            }
        }

        // If no cached adapters, do a quick compatibility test
        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory_->EnumAdapters1(i, &adapter); ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            // Skip software adapters
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            // Test D3D12 device creation
            ComPtr<ID3D12Device> testDevice;
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))) {
                return true;
            }
        }

        return false;
    }

    std::vector<std::string> D3D12Factory::GetSupportedDevices() const {
        std::vector<std::string> deviceNames;

        for (const auto& adapter : adapters_) {
            if (adapter.isD3D12Compatible) {
                deviceNames.push_back(adapter.name);
            }
        }

        return deviceNames;
    }

    DeviceCapabilities D3D12Factory::GetDeviceCapabilities(uint32_t deviceIndex) const {
        if (deviceIndex >= adapters_.size()) {
            logChannel_.LogFormat(LogLevel::Warning,
                "Invalid device index {} (available: {})", deviceIndex, adapters_.size());

            // Return default capabilities
            DeviceCapabilities caps;
            caps.SetDefaults();
            return caps;
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

    void D3D12Factory::EnumerateAdapters() {
        adapters_.clear();

        if (!dxgiFactory_) {
            logChannel_.Log(LogLevel::Error, "DXGI Factory not initialized");
            return;
        }

        logChannel_.Log(LogLevel::Info, "Enumerating graphics adapters");

        ComPtr<IDXGIAdapter4> adapter;
        for (UINT adapterIndex = 0; ; ++adapterIndex) {
            HRESULT hr = dxgiFactory_->EnumAdapterByGpuPreference(
                adapterIndex,
                DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                IID_PPV_ARGS(&adapter));

            if (hr == DXGI_ERROR_NOT_FOUND) {
                break; // No more adapters
            }

            if (FAILED(hr)) {
                logChannel_.LogFormat(LogLevel::Warning,
                    "Failed to enumerate adapter {}: HRESULT 0x{:08X}",
                    adapterIndex, static_cast<uint32_t>(hr));
                continue;
            }

            DXGI_ADAPTER_DESC3 desc;
            hr = adapter->GetDesc3(&desc);
            if (FAILED(hr)) {
                logChannel_.LogFormat(LogLevel::Warning,
                    "Failed to get description for adapter {}", adapterIndex);
                continue;
            }

            // Convert adapter info to our updated format
            AdapterInfo info;
            info.index = adapterIndex;
            info.description = desc.Description;
            info.name = Platform::Windows::WideToUtf8(desc.Description);
            info.vendorId = desc.VendorId;
            info.deviceId = desc.DeviceId;
            info.subSysId = desc.SubSysId;
            info.revision = desc.Revision;
            info.luid = *reinterpret_cast<const uint64_t*>(&desc.AdapterLuid);
            info.dedicatedVideoMemory = desc.DedicatedVideoMemory;
            info.dedicatedSystemMemory = desc.DedicatedSystemMemory;
            info.sharedSystemMemory = desc.SharedSystemMemory;

            // Determine if this is a hardware adapter
            info.isHardware = !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE);
            info.isIntegrated = !!(desc.Flags & DXGI_ADAPTER_FLAG3_ACG_COMPATIBLE); // Best heuristic available

            // Check D3D12 compatibility
            ComPtr<ID3D12Device> testDevice;
            D3D_FEATURE_LEVEL featureLevels[] = {
                D3D_FEATURE_LEVEL_12_2,
                D3D_FEATURE_LEVEL_12_1,
                D3D_FEATURE_LEVEL_12_0,
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0
            };

            info.isD3D12Compatible = false;
            for (auto featureLevel : featureLevels) {
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), featureLevel, IID_PPV_ARGS(&testDevice)))) {
                    info.isD3D12Compatible = true;
                    info.maxFeatureLevel = static_cast<uint32_t>(featureLevel);

                    // Query detailed capabilities from the test device
                    info.UpdateFromD3D12Capabilities(testDevice.Get());
                    break;
                }
            }

            if (info.isD3D12Compatible) {
                logChannel_.LogFormat(LogLevel::Info,
                    "Found D3D12 compatible adapter: '{}' ({:.1f} GB VRAM) - {}",
                    info.name,
                    static_cast<double>(info.dedicatedVideoMemory) / (1024.0 * 1024.0 * 1024.0),
                    info.GetCapabilitySummary());
            }
            else {
                logChannel_.LogFormat(LogLevel::Info,
                    "Adapter '{}' does not support D3D12", info.name);
            }

            adapters_.push_back(std::move(info));
        }

        // Sort adapters by preference (D3D12 compatible, discrete, high VRAM first)
        AdapterUtils::SortAdaptersByPreference(adapters_);

        size_t compatibleCount = std::count_if(adapters_.begin(), adapters_.end(),
            [](const AdapterInfo& info) { return info.isD3D12Compatible; });

        logChannel_.LogFormat(LogLevel::Info,
            "Enumerated {} adapters ({} D3D12 compatible)",
            adapters_.size(), compatibleCount);

        // Log details about the best adapter
        if (!adapters_.empty() && adapters_[0].isD3D12Compatible) {
            const auto& best = adapters_[0];
            logChannel_.LogFormat(LogLevel::Info,
                "Best adapter: {} - {} - {:.1f} GB VRAM",
                AdapterUtils::GetVendorName(best.vendorId),
                best.name,
                static_cast<double>(best.dedicatedVideoMemory) / (1024.0 * 1024.0 * 1024.0));
        }
    }

    DeviceCapabilities D3D12Factory::GetAdapterCapabilities(const AdapterInfo& adapter) const {
        DeviceCapabilities caps;
        caps.SetDefaults(); // Start with reasonable defaults

        // Copy basic adapter information
        caps.adapterDescription = Platform::Windows::WideToUtf8(adapter.description);
        caps.adapterName = adapter.name;
        caps.vendorId = adapter.vendorId;
        caps.deviceId = adapter.deviceId;
        caps.subSysId = adapter.subSysId;
        caps.revision = adapter.revision;
        caps.adapterLuid = adapter.luid;

        // Memory information
        caps.dedicatedVideoMemory = adapter.dedicatedVideoMemory;
        caps.dedicatedSystemMemory = adapter.dedicatedSystemMemory;
        caps.sharedSystemMemory = adapter.sharedSystemMemory;
        caps.availableVideoMemory = adapter.dedicatedVideoMemory; // Assume all available initially

        // GPU classification
        caps.isDiscrete = adapter.IsDiscrete();
        caps.isWarpAdapter = (adapter.vendorId == 0x1414); // Microsoft WARP

        // Feature level and D3D12 capabilities
        caps.maxFeatureLevel = adapter.maxFeatureLevel;

        // Copy D3D12 capability flags
        caps.supportsRayTracing = adapter.supportsRayTracing;
        caps.supportsVariableRateShading = adapter.supportsVariableRateShading;
        caps.supportsMeshShaders = adapter.supportsMeshShaders;
        caps.supportsDirectML = adapter.supportsDirectML;

        // Copy tier information
        caps.raytracingTier = adapter.raytracingTier;
        caps.variableRateShadingTier = adapter.variableRateShadingTier;
        caps.meshShaderTier = adapter.meshShaderTier;

        // Shader model support (infer from other capabilities)
        caps.supportsShaderModel6 = (caps.maxFeatureLevel >= static_cast<uint32_t>(D3D_FEATURE_LEVEL_12_0));
        caps.supportsShaderModel6_6 = adapter.supportsShaderModel6_6;
        caps.supportsShaderModel6_7 = false; // Conservative default
        caps.highestShaderModel = caps.supportsShaderModel6_6 ? 0x0606 : 0x0600;

        // Architecture properties
        caps.isUMA = adapter.isIntegrated; // Heuristic: integrated GPUs are usually UMA
        caps.isCacheCoherentUMA = caps.isUMA && (adapter.vendorId == 0x8086); // Intel integrated
        caps.isTileBasedRenderer = false; // Desktop GPUs are typically not tile-based

        // Set conservative resource limits for unknown adapters
        caps.maxTexture2DSize = 16384;
        caps.maxTexture3DSize = 2048;
        caps.maxTextureCubeSize = 16384;
        caps.maxTextureArraySize = 2048;
        caps.maxRenderTargets = 8;
        caps.maxViewports = 16;
        caps.maxVertexInputElements = 32;
        caps.maxConstantBufferSize = 65536;

        // Multi-sampling support (conservative defaults)
        caps.supportedSampleCounts = { 1, 2, 4, 8 };
        if (caps.IsHighEndGPU()) {
            caps.supportedSampleCounts.push_back(16);
        }

        // Debug features availability
#ifdef _DEBUG
        caps.debugLayerAvailable = true;
        caps.gpuValidationAvailable = true;
        caps.dredAvailable = true;
#else
        caps.debugLayerAvailable = false;
        caps.gpuValidationAvailable = false;
        caps.dredAvailable = false;
#endif

        // Performance hints based on GPU type
        caps.recommendedSwapChainBuffers = caps.IsHighEndGPU() ? 3 : 2;
        caps.prefersBundledCommandLists = caps.IsHighEndGPU();
        caps.prefersResourceBarrierBatching = true;

        // Memory alignment (conservative)
        caps.minResourceAlignment = 256;

        // If we can create a temporary device, get precise capabilities
        if (adapter.isD3D12Compatible) {
            QueryPreciseCapabilities(adapter, caps);
        }

        return caps;
    }

    void D3D12Factory::QueryPreciseCapabilities(const AdapterInfo& adapter, DeviceCapabilities& caps) const {
        // Find the DXGI adapter
        ComPtr<IDXGIAdapter4> dxgiAdapter;
        HRESULT hr = dxgiFactory_->EnumAdapterByGpuPreference(
            adapter.index,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&dxgiAdapter));

        if (FAILED(hr)) {
            logChannel_.LogFormat(LogLevel::Warning,
                "Failed to get DXGI adapter {} for precise capability query", adapter.index);
            return;
        }

        // Create temporary device for precise queries
        ComPtr<ID3D12Device> device;
        hr = D3D12CreateDevice(dxgiAdapter.Get(), static_cast<D3D_FEATURE_LEVEL>(adapter.maxFeatureLevel), IID_PPV_ARGS(&device));
        if (FAILED(hr)) {
            logChannel_.LogFormat(LogLevel::Warning,
                "Failed to create temporary device for capability query on adapter {}", adapter.index);
            return;
        }

        // Use the DeviceCapabilities UpdateFromD3D12Device method
        caps.UpdateFromD3D12Device(device.Get());

        logChannel_.LogFormat(LogLevel::Debug,
            "Queried precise capabilities for adapter '{}': {}",
            adapter.name, caps.GetCapabilitySummary());
    }

} // namespace Akhanda::RHI::D3D12

// ========================================================================
// Global Factory Creation Functions - This fixes the main linker error!
// ========================================================================

namespace Akhanda::RHI {

    /// Creates a D3D12 RHI Factory instance
    /// This is the missing function that was causing your original linker error
    std::unique_ptr<IRHIFactory> CreateD3D12Factory() {
        // Enable D3D12 debug layer in debug builds
#ifdef _DEBUG
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();

            // Enable GPU-based validation for additional debugging
            ComPtr<ID3D12Debug1> debugController1;
            if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1)))) {
                debugController1->SetEnableGPUBasedValidation(true);
            }

            Logging::LogManager::Instance().GetChannel("RHI").Log(
                Logging::LogLevel::Info, "D3D12 debug layer and GPU validation enabled");
        }
#endif

        auto factory = std::make_unique<D3D12::D3D12Factory>();

        if (!factory->Initialize()) {
            Logging::LogManager::Instance().GetChannel("RHI").Log(
                Logging::LogLevel::Error, "Failed to initialize D3D12 Factory");
            return nullptr;
        }

        return std::unique_ptr<IRHIFactory>(factory.release());
    }

    /// Helper function to create D3D12 factory with specific debug configuration
    std::unique_ptr<IRHIFactory> CreateD3D12FactoryWithDebug(bool enableDebugLayer, bool enableGBV) {
        if (enableDebugLayer) {
#ifdef _DEBUG
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
                debugController->EnableDebugLayer();

                if (enableGBV) {
                    ComPtr<ID3D12Debug1> debugController1;
                    if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1)))) {
                        debugController1->SetEnableGPUBasedValidation(true);
                    }
                }

                Logging::LogManager::Instance().GetChannel("RHI").Log(
                    Logging::LogLevel::Info,
                    std::format("D3D12 debug layer enabled (GBV: {})", enableGBV ? "Yes" : "No"));
            }
#endif
        }

        return CreateD3D12Factory();
    }

    /// Query if D3D12 is supported on this system without creating a full factory
    bool IsD3D12Supported() {
        ComPtr<IDXGIFactory4> factory;
        if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) {
            return false;
        }

        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter); ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            // Skip software adapters
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            // Test D3D12 device creation
            ComPtr<ID3D12Device> testDevice;
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))) {
                return true;
            }
        }

        return false;
    }

    /// Get basic system GPU information without creating a factory
    std::vector<std::string> GetAvailableD3D12Adapters() {
        std::vector<std::string> adapterNames;

        ComPtr<IDXGIFactory4> factory;
        if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) {
            return adapterNames;
        }

        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter); ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            // Skip software adapters
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            // Test D3D12 compatibility
            ComPtr<ID3D12Device> testDevice;
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))) {
                std::string name = Platform::Windows::WideToUtf8(desc.Description);
                adapterNames.push_back(name);
            }
        }

        return adapterNames;
    }

} // namespace Akhanda::RHI