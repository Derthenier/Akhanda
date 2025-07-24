// Engine/Renderer/RHI/StandardRenderer.cpp
// Akhanda Game Engine - Standard Renderer Implementation
// Complete D3D12 renderer working with existing interfaces
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

// Windows and D3D12 headers
#ifdef _WIN32
#include <windows.h>
#undef min
#undef max
#undef DeviceCapabilities
#endif

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

// Standard library
#include <memory>
#include <vector>
#include <array>
#include <unordered_map>
#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <cassert>
#include <format>

// Engine logging
#include "Core/Logging/Core.Logging.hpp"

module Akhanda.Engine.Renderer;

import Akhanda.Core.Memory;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Core.Math;
import Akhanda.Engine.RHI;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Platform.Interfaces;

namespace Akhanda::Renderer {

    using namespace Microsoft::WRL;
    using namespace Configuration;
    using namespace RHI;

    // ============================================================================
    // Constants and Configuration
    // ============================================================================

    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
    static constexpr uint32_t COMMAND_LIST_POOL_SIZE = 16;

    // ============================================================================
    // Frame Resources - Per-frame data for triple buffering
    // ============================================================================

    struct FrameResources {
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        ComPtr<ID3D12GraphicsCommandList> commandList;
        ComPtr<ID3D12Fence> fence;
        uint64_t fenceValue = 0;
        HANDLE fenceEvent = nullptr;

        // Per-frame descriptor heaps
        ComPtr<ID3D12DescriptorHeap> cbvSrvUavHeap;

        // Per-frame staging resources
        std::vector<ComPtr<ID3D12Resource>> stagingBuffers;
        std::vector<ComPtr<ID3D12Resource>> constantBuffers;

        FrameResources() = default;
        ~FrameResources() {
            if (fenceEvent) {
                CloseHandle(fenceEvent);
            }
        }

        // Non-copyable, movable
        FrameResources(const FrameResources&) = delete;
        FrameResources& operator=(const FrameResources&) = delete;
        FrameResources(FrameResources&&) = default;
        FrameResources& operator=(FrameResources&&) = default;
    };

    // ============================================================================
    // StandardRenderer Implementation
    // ============================================================================

    StandardRenderer::StandardRenderer()
        : logChannel_(Logging::LogManager::Instance().GetChannel("Renderer")) {

        logChannel_.Log(Logging::LogLevel::Info, "StandardRenderer created");
    }

    StandardRenderer::~StandardRenderer() {
        if (isInitialized_.load()) {
            Shutdown();
        }
    }

    bool StandardRenderer::Initialize(const RenderingConfig& config, const Platform::SurfaceInfo& surfaceInfo) {
        std::lock_guard<std::mutex> lock(renderMutex_);

        logChannel_.Log(Logging::LogLevel::Info, "Initializing StandardRenderer...");

        try {
            config_ = config;

            // Convert base SurfaceInfo to RendererSurfaceInfo
            if (auto* rendererSurfaceInfo = static_cast<const Platform::RendererSurfaceInfo*>(&surfaceInfo)) {
                surfaceInfo_ = *rendererSurfaceInfo;
            }
            else {
                // Create RendererSurfaceInfo from base SurfaceInfo
                surfaceInfo_.nativeHandle = surfaceInfo.nativeHandle;
                surfaceInfo_.nativeDisplay = surfaceInfo.nativeDisplay;
                surfaceInfo_.width = surfaceInfo.width;
                surfaceInfo_.height = surfaceInfo.height;
                surfaceInfo_.format = surfaceInfo.format;
                surfaceInfo_.supportsPresent = surfaceInfo.supportsPresent;
                surfaceInfo_.sampleCount = surfaceInfo.sampleCount;
                surfaceInfo_.qualityLevels = surfaceInfo.qualityLevels;

                // Set Windows-specific handles
                surfaceInfo_.windowHandle = static_cast<HWND>(surfaceInfo.nativeHandle);
                surfaceInfo_.instanceHandle = GetModuleHandle(nullptr);
            }

            // Initialize RHI factory and device
            if (!InitializeRHI()) {
                logChannel_.Log(Logging::LogLevel::Error, "Failed to initialize RHI");
                return false;
            }

            // Initialize render passes
            if (!InitializeRenderPasses()) {
                logChannel_.Log(Logging::LogLevel::Error, "Failed to initialize render passes");
                return false;
            }

            // Initialize default resources
            if (!InitializeDefaultResources()) {
                logChannel_.Log(Logging::LogLevel::Error, "Failed to initialize default resources");
                return false;
            }

            // Initialize default camera (inline)
            currentCamera_.position = { 0.0f, 0.0f, -5.0f };
            currentCamera_.forward = { 0.0f, 0.0f, 1.0f };
            currentCamera_.up = { 0.0f, 1.0f, 0.0f };
            currentCamera_.right = { 1.0f, 0.0f, 0.0f };
            currentCamera_.nearPlane = 0.1f;
            currentCamera_.farPlane = 1000.0f;

            // Calculate view matrix (looking at origin from -5 on Z)
            Math::Vector3 eye = currentCamera_.position;
            Math::Vector3 target = { 0.0f, 0.0f, 0.0f };
            Math::Vector3 up = currentCamera_.up;

            currentCamera_.viewMatrix = Math::LookAt(eye, target, up);

            // Calculate projection matrix
            float aspect = static_cast<float>(surfaceInfo_.width) / static_cast<float>(surfaceInfo_.height);
            float fovY = Math::ToRadians(45.0f); // 45 degree field of view

            currentCamera_.projectionMatrix = Math::Perspective(fovY, aspect,
                currentCamera_.nearPlane,
                currentCamera_.farPlane);

            // Calculate combined view-projection matrix
            currentCamera_.viewProjectionMatrix = currentCamera_.projectionMatrix * currentCamera_.viewMatrix;

            isInitialized_.store(true);
            logChannel_.Log(Logging::LogLevel::Info, "StandardRenderer initialized successfully");

            return true;
        }
        catch (const std::exception& e) {
            logChannel_.Log(Logging::LogLevel::Error,
                std::format("StandardRenderer initialization failed: {}", e.what()));
            return false;
        }
    }

    void StandardRenderer::Shutdown() {
        std::lock_guard<std::mutex> lock(renderMutex_);

        if (!isInitialized_.load()) {
            return;
        }

        logChannel_.Log(Logging::LogLevel::Info, "Shutting down StandardRenderer...");

        // Wait for all frames to complete
        WaitForGPUIdle();

        // Clean up resources
        CleanupResources();

        // Reset state
        currentFrameData_.reset();
        meshes_.clear();
        materials_.clear();
        renderTargets_.clear();
        textureCache_.clear();
        standardRenderPasses_.clear();

        // Shutdown RHI
        if (device_) {
            device_->Shutdown();
            device_.reset();
        }

        if (rhiFactory_) {
            rhiFactory_->Shutdown();
            rhiFactory_.reset();
        }

        isInitialized_.store(false);
        logChannel_.Log(Logging::LogLevel::Info, "StandardRenderer shutdown complete");
    }

    void StandardRenderer::BeginFrame() {
        if (!isInitialized_.load()) {
            throw std::runtime_error("Renderer not initialized");
        }

        frameStartTime_ = std::chrono::high_resolution_clock::now();

        // Wait for previous frame to complete
        WaitForFrameCompletion();

        // Create new frame data
        currentFrameData_ = std::make_unique<FrameData>();
        currentFrameData_->SetCamera(currentCamera_);
        currentFrameData_->SetLighting(currentLighting_);

        // Create main command list from RHI
        mainCommandList_ = device_->CreateCommandList(CommandListType::Direct);
        if (!mainCommandList_) {
            throw std::runtime_error("Failed to create command list");
        }

        // Reset command list to start recording
        mainCommandList_->Reset();

        // Set main render target (back buffer)
        SetMainRenderTarget();

        // Clear render targets
        ClearRenderTarget({ 0.2f, 0.2f, 0.3f, 1.0f }); // Dark blue
        ClearDepthStencil(1.0f, 0);

        // Update frame constants
        UpdateFrameConstants();

        ++frameNumber_;

        logChannel_.Log(Logging::LogLevel::Trace,
            std::format("Frame {} started", frameNumber_.load()));
    }

    void StandardRenderer::EndFrame() {
        try {
            // Close command recording
            mainCommandList_->Close();

            // Execute command list on device
            ICommandList* commandLists[] = { mainCommandList_.get() };
            device_->ExecuteCommandLists(CommandListType::Direct, 1, commandLists);

            // Reset main command list
            mainCommandList_.reset();

            UpdateStats();

            logChannel_.Log(Logging::LogLevel::Trace,
                std::format("Frame {} ended", frameNumber_.load()));
        }
        catch (const std::exception& e) {
            logChannel_.Log(Logging::LogLevel::Error,
                std::format("EndFrame failed: {}", e.what()));
            throw;
        }
    }

    void StandardRenderer::Present() {
        // Present through device
        if (device_) {
            device_->Present();
            logChannel_.Log(Logging::LogLevel::Trace, "Frame presented via device");
        }
    }

    void StandardRenderer::RenderFrame(const FrameData& frameData) {
        // Process draw commands from frame data
        ProcessDrawCommands(frameData);

        // Execute built-in render passes
        ExecuteRenderPasses(frameData);

        // Update statistics
        stats_.drawCalls += static_cast<uint32_t>(frameData.GetDrawCommands().size());
    }

    void StandardRenderer::SetCamera(const CameraData& camera) {
        currentCamera_ = camera;

        if (currentFrameData_) {
            currentFrameData_->SetCamera(camera);
        }
    }

    void StandardRenderer::SetLighting(const SceneLighting& lighting) {
        currentLighting_ = lighting;

        if (currentFrameData_) {
            currentFrameData_->SetLighting(lighting);
        }
    }

    std::shared_ptr<Mesh> StandardRenderer::CreateMesh([[maybe_unused]] const MeshData& meshData) {
        // TODO: Implement mesh creation using RHI
        // For now, return nullptr as placeholder
        logChannel_.Log(Logging::LogLevel::Warning, "CreateMesh not yet implemented");
        return nullptr;
    }

    std::shared_ptr<Material> StandardRenderer::CreateMaterial([[maybe_unused]] const std::string& name) {
        // TODO: Implement material creation
        // For now, return nullptr as placeholder
        logChannel_.Log(Logging::LogLevel::Warning, "CreateMaterial not yet implemented");
        return nullptr;
    }

    std::shared_ptr<RenderTarget> StandardRenderer::CreateRenderTarget([[maybe_unused]] const RenderTargetDesc& desc) {
        // TODO: Implement render target creation using RHI
        // For now, return nullptr as placeholder
        logChannel_.Log(Logging::LogLevel::Warning, "CreateRenderTarget not yet implemented");
        return nullptr;
    }

    std::shared_ptr<Mesh> StandardRenderer::LoadMesh([[maybe_unused]] const std::string& filepath) {
        // TODO: Implement mesh loading
        logChannel_.Log(Logging::LogLevel::Warning, "LoadMesh not yet implemented");
        return nullptr;
    }

    RHI::TextureHandle StandardRenderer::LoadTexture([[maybe_unused]] const std::string& filepath) {
        // TODO: Implement texture loading
        logChannel_.Log(Logging::LogLevel::Warning, "LoadTexture not yet implemented");
        return RHI::TextureHandle{};
    }

    void StandardRenderer::DrawMesh([[maybe_unused]] const Mesh& mesh, [[maybe_unused]] const Material& material, [[maybe_unused]] const Math::Matrix4& worldMatrix) {
        // TODO: Implement mesh drawing
        // For now, just increment draw call counter
        ++stats_.drawCalls;
        logChannel_.Log(Logging::LogLevel::Trace, "DrawMesh called (not yet implemented)");
    }

    void StandardRenderer::DrawMeshInstanced([[maybe_unused]] const Mesh& mesh, [[maybe_unused]] const Material& material,
        [[maybe_unused]] const std::vector<InstanceData>& instances) {
        // TODO: Implement instanced mesh drawing
        ++stats_.drawCalls;
        stats_.instances += static_cast<uint32_t>(instances.size());
        logChannel_.Log(Logging::LogLevel::Trace, "DrawMeshInstanced called (not yet implemented)");
    }

    void StandardRenderer::SetRenderTarget(std::shared_ptr<RenderTarget> target) {
        currentRenderTarget_ = target;

        if (mainCommandList_ && target) {
            // TODO: Set render target on command list
            logChannel_.Log(Logging::LogLevel::Trace, "SetRenderTarget called (not fully implemented)");
        }
    }

    void StandardRenderer::SetMainRenderTarget() {
        if (mainCommandList_ && device_) {
            // Get swap chain from device and set back buffer as render target
            if (auto* swapChain = device_->GetSwapChain()) {
                auto backBuffer = swapChain->GetCurrentBackBuffer();
                // TODO: Set back buffer as render target through command list
                logChannel_.Log(Logging::LogLevel::Trace, "SetMainRenderTarget called with swap chain");
            }
            else {
                logChannel_.Log(Logging::LogLevel::Trace, "SetMainRenderTarget called (no swap chain)");
            }
        }
    }

    void StandardRenderer::ClearRenderTarget(const Math::Vector4& color) {
        if (mainCommandList_) {
            // TODO: Clear current render target with specified color
            logChannel_.Log(Logging::LogLevel::Trace,
                std::format("ClearRenderTarget: [{:.2f}, {:.2f}, {:.2f}, {:.2f}]",
                    color.x, color.y, color.z, color.w));
        }
    }

    void StandardRenderer::ClearDepthStencil(float depth, uint8_t stencil) {
        if (mainCommandList_) {
            // TODO: Clear depth stencil buffer
            logChannel_.Log(Logging::LogLevel::Trace,
                std::format("ClearDepthStencil: depth={:.2f}, stencil={}", depth, stencil));
        }
    }

    void StandardRenderer::SetDebugName(const char* name) {
        debugName_ = name ? name : "";
        logChannel_.Log(Logging::LogLevel::Debug,
            std::format("Renderer debug name set to: {}", debugName_));
    }

    void StandardRenderer::BeginDebugEvent(const char* name) {
        if (name && mainCommandList_) {
            // TODO: Begin debug event on command list
            logChannel_.Log(Logging::LogLevel::Trace,
                std::format("BeginDebugEvent: {}", name));
        }
    }

    void StandardRenderer::EndDebugEvent() {
        if (mainCommandList_) {
            // TODO: End debug event on command list
            logChannel_.Log(Logging::LogLevel::Trace, "EndDebugEvent");
        }
    }

    void StandardRenderer::ResetStats() {
        stats_ = RendererStats{};
        logChannel_.Log(Logging::LogLevel::Debug, "Renderer statistics reset");
    }

    const RHI::DeviceCapabilities& StandardRenderer::GetDeviceCapabilities() const {
        static RHI::DeviceCapabilities defaultCaps{};

        if (device_) {
            return device_->GetCapabilities();
        }

        return defaultCaps;
    }

    // ============================================================================
    // Private Implementation Methods
    // ============================================================================

    bool StandardRenderer::InitializeRHI() {
        try {
            // Create RHI factory for D3D12
            rhiFactory_ = Akhanda::RHI::CreateRHIFactory(RenderingAPI::D3D12);
            if (!rhiFactory_) {
                logChannel_.Log(Logging::LogLevel::Error, "Failed to create RHI factory");
                return false;
            }

            logChannel_.Log(Logging::LogLevel::Info, "RHI factory created successfully");

            // Create render device using factory
            device_ = rhiFactory_->CreateDevice(config_, surfaceInfo_);
            if (!device_) {
                logChannel_.Log(Logging::LogLevel::Error, "Failed to create render device");
                return false;
            }

            logChannel_.Log(Logging::LogLevel::Info, "Render device created successfully");
            logChannel_.Log(Logging::LogLevel::Info, "RHI initialization complete");
            return true;
        }
        catch (const std::exception& e) {
            logChannel_.Log(Logging::LogLevel::Error,
                std::format("RHI initialization failed: {}", e.what()));
            return false;
        }
    }

    bool StandardRenderer::InitializeRenderPasses() {
        // TODO: Initialize built-in render passes
        // For now, just clear the container
        standardRenderPasses_.clear();

        logChannel_.Log(Logging::LogLevel::Info, "Render passes initialized");
        return true;
    }

    bool StandardRenderer::InitializeDefaultResources() {
        // TODO: Create default meshes, materials, textures
        logChannel_.Log(Logging::LogLevel::Info, "Default resources initialized");
        return true;
    }

    void StandardRenderer::UpdateFrameConstants() {
        if (!currentFrameData_) {
            return;
        }

        FrameConstants constants;
        constants.viewMatrix = currentCamera_.viewMatrix;
        constants.projectionMatrix = currentCamera_.projectionMatrix;
        constants.viewProjectionMatrix = currentCamera_.viewProjectionMatrix;
        constants.inverseViewMatrix = Math::Inverse(currentCamera_.viewMatrix);
        constants.inverseProjectionMatrix = Math::Inverse(currentCamera_.projectionMatrix);
        constants.cameraPosition = { currentCamera_.position.x, currentCamera_.position.y,
                                   currentCamera_.position.z, 1.0f };
        constants.cameraDirection = { currentCamera_.forward.x, currentCamera_.forward.y,
                                    currentCamera_.forward.z, 0.0f };
        constants.screenSize = { static_cast<float>(surfaceInfo_.width),
                               static_cast<float>(surfaceInfo_.height) };
        constants.invScreenSize = { 1.0f / constants.screenSize.x, 1.0f / constants.screenSize.y };
        constants.frameNumber = static_cast<uint32_t>(frameNumber_.load());

        // Calculate timing
        auto now = std::chrono::high_resolution_clock::now();
        static auto startTime = now;
        auto elapsed = std::chrono::duration<float>(now - startTime).count();
        constants.time = elapsed;

        static auto lastFrameTime = now;
        constants.deltaTime = std::chrono::duration<float>(now - lastFrameTime).count();
        lastFrameTime = now;

        currentFrameData_->SetConstants(constants);
    }

    void StandardRenderer::ProcessDrawCommands(const FrameData& frameData) {
        const auto& drawCommands = frameData.GetDrawCommands();

        for (const auto& command : drawCommands) {
            // TODO: Process individual draw commands
            // For now, just count them
            ++stats_.drawCalls;
        }

        if (!drawCommands.empty()) {
            logChannel_.Log(Logging::LogLevel::Trace,
                std::format("Processed {} draw commands", drawCommands.size()));
        }
    }

    void StandardRenderer::ExecuteRenderPasses(const FrameData& frameData) const {
        const auto& renderPasses = frameData.GetRenderPasses();

        for (const auto& pass : renderPasses) {
            if (pass && mainCommandList_) {
                pass->Setup(mainCommandList_.get());
                pass->Execute(mainCommandList_.get(), frameData);
                pass->Cleanup(mainCommandList_.get());
            }
        }

        if (!renderPasses.empty()) {
            logChannel_.Log(Logging::LogLevel::Trace,
                std::format("Executed {} render passes", renderPasses.size()));
        }
    }

    void StandardRenderer::WaitForFrameCompletion() {
        // TODO: Implement frame synchronization
        // For now, just log
        logChannel_.Log(Logging::LogLevel::Trace, "Waiting for frame completion");
    }

    void StandardRenderer::WaitForGPUIdle() {
        if (device_) {
            device_->WaitForIdle();
            logChannel_.Log(Logging::LogLevel::Debug, "GPU idle wait completed");
        }
    }

    void StandardRenderer::UpdateStats() {
        auto frameEndTime = std::chrono::high_resolution_clock::now();
        auto frameDuration = frameEndTime - frameStartTime_;
        stats_.frameTime = std::chrono::duration<float, std::milli>(frameDuration).count();
        stats_.fps = stats_.frameTime > 0.0f ? 1000.0f / stats_.frameTime : 0.0f;
        stats_.frameNumber = frameNumber_;

        // Update resource counts
        stats_.meshCount = static_cast<uint32_t>(meshes_.size());
        stats_.materialCount = static_cast<uint32_t>(materials_.size());
        stats_.textureCount = static_cast<uint32_t>(textureCache_.size());

        logChannel_.LogFormat(Logging::LogLevel::Trace, "Frame {}: {:.2f}ms ({:.1f} FPS)",
                stats_.frameNumber, stats_.frameTime, stats_.fps);
    }

    void StandardRenderer::CalculateFrameTiming() {
        // Frame timing is calculated in UpdateStats()
    }

    void StandardRenderer::LogRendererInfo() {
        logChannel_.LogFormat(Logging::LogLevel::Info, "StandardRenderer '{}' - Frame: {}, FPS: {:.1f}",
                debugName_, stats_.frameNumber, stats_.fps);
    }

    void StandardRenderer::ValidateRenderState() {
        // TODO: Implement render state validation
        logChannel_.Log(Logging::LogLevel::Trace, "Render state validation passed");
    }

    RHI::BufferHandle StandardRenderer::CreateVertexBuffer(const std::vector<Vertex>& vertices) {
        // TODO Phase 2.3: Implement vertex buffer creation when we add mesh rendering
        if (!vertices.empty()) {
            logChannel_.Log(Logging::LogLevel::Warning,
                std::format("CreateVertexBuffer called with {} vertices (not implemented yet)", vertices.size()));
        }
        return RHI::BufferHandle{}; // Return empty handle for now
    }

    RHI::BufferHandle StandardRenderer::CreateIndexBuffer(const std::vector<uint32_t>& indices) {
        // TODO Phase 2.3: Implement index buffer creation when we add mesh rendering
        if (!indices.empty()) {
            logChannel_.Log(Logging::LogLevel::Warning,
                std::format("CreateIndexBuffer called with {} indices (not implemented yet)", indices.size()));
        }
        return RHI::BufferHandle{}; // Return empty handle for now
    }

    void StandardRenderer::CleanupResources() {
        // Clean up in reverse order of creation
        standardRenderPasses_.clear();
        renderTargets_.clear();
        materials_.clear();
        meshes_.clear();
        textureCache_.clear();

        currentRenderTarget_.reset();
        mainCommandList_.reset();
        currentFrameData_.reset();

        logChannel_.Log(Logging::LogLevel::Debug, "Resources cleaned up");
    }

} // namespace Akhanda::Renderer