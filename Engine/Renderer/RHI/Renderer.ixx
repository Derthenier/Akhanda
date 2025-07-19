// Renderer.ixx
// Akhanda Game Engine - High-Level Renderer System
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include <memory>
#include <vector>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <functional>
#include <chrono>

export module Akhanda.Engine.Renderer;

import Akhanda.Engine.RHI;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Platform.Interfaces;
import Akhanda.Platform.RendererIntegration;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Core.Math;
import Akhanda.Core.Memory;
import Akhanda.Core.Logging;

export namespace Akhanda::Renderer {

    // ========================================================================
    // Forward Declarations
    // ========================================================================

    class RenderGraph;
    class Material;
    class Mesh;
    class Camera;
    class Light;
    class RenderTarget;
    class FrameData;

    // ========================================================================
    // Rendering Primitives
    // ========================================================================

    struct Vertex {
        Math::Vector3 position;
        Math::Vector3 normal;
        Math::Vector2 texCoord;
        Math::Vector3 tangent;
        Math::Vector3 bitangent;
        Math::Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        Math::BoundingBox boundingBox;
        std::string debugName;
    };

    enum class LightType : uint32_t {
        Directional = 0,
        Point,
        Spot,
        Area
    };

    struct LightData {
        Math::Vector3 position;
        float intensity = 1.0f;
        Math::Vector3 direction;
        float range = 100.0f;
        Math::Vector3 color = { 1.0f, 1.0f, 1.0f };
        float innerConeAngle = 30.0f;
        Math::Vector3 attenuation = { 1.0f, 0.09f, 0.032f };
        float outerConeAngle = 45.0f;
        LightType type = LightType::Directional;
        bool castShadows = false;
        uint32_t shadowMapSize = 1024;
        float shadowBias = 0.005f;
    };

    struct CameraData {
        Math::Matrix4 viewMatrix;
        Math::Matrix4 projectionMatrix;
        Math::Matrix4 viewProjectionMatrix;
        Math::Vector3 position;
        Math::Vector3 forward;
        Math::Vector3 up;
        Math::Vector3 right;
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
        float fieldOfView = 60.0f;
        float aspectRatio = 16.0f / 9.0f;
        bool isOrthographic = false;
        float orthographicSize = 10.0f;
    };

    // ========================================================================
    // Material System
    // ========================================================================

    enum class MaterialType : uint32_t {
        Standard = 0,
        Unlit,
        Transparent,
        Cutout,
        Emissive,
        Custom
    };

    struct MaterialProperties {
        Math::Vector4 albedoColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        Math::Vector3 emissiveColor = { 0.0f, 0.0f, 0.0f };
        float metallic = 0.0f;
        float roughness = 0.5f;
        float normalScale = 1.0f;
        float occlusionStrength = 1.0f;
        float alphaCutoff = 0.5f;
        bool doubleSided = false;
        MaterialType type = MaterialType::Standard;
    };

    struct MaterialTextures {
        RHI::TextureHandle albedoTexture;
        RHI::TextureHandle normalTexture;
        RHI::TextureHandle metallicRoughnessTexture;
        RHI::TextureHandle occlusionTexture;
        RHI::TextureHandle emissiveTexture;
        RHI::TextureHandle heightTexture;
    };

    class Material {
    private:
        MaterialProperties properties_;
        MaterialTextures textures_;
        RHI::PipelineHandle pipeline_;
        std::string name_;
        uint32_t materialId_;

    public:
        Material(const std::string& name = "");
        ~Material() = default;

        // Property access
        const MaterialProperties& GetProperties() const { return properties_; }
        void SetProperties(const MaterialProperties& properties) { properties_ = properties; }

        const MaterialTextures& GetTextures() const { return textures_; }
        void SetTextures(const MaterialTextures& textures) { textures_ = textures; }

        // Pipeline access
        RHI::PipelineHandle GetPipeline() const { return pipeline_; }
        void SetPipeline(RHI::PipelineHandle pipeline) { pipeline_ = pipeline; }

        // Identification
        const std::string& GetName() const { return name_; }
        uint32_t GetId() const { return materialId_; }

        // Convenience setters
        void SetAlbedoColor(const Math::Vector4& color) { properties_.albedoColor = color; }
        void SetMetallic(float metallic) { properties_.metallic = metallic; }
        void SetRoughness(float roughness) { properties_.roughness = roughness; }
        void SetEmissive(const Math::Vector3& emissive) { properties_.emissiveColor = emissive; }

        void SetAlbedoTexture(RHI::TextureHandle texture) { textures_.albedoTexture = texture; }
        void SetNormalTexture(RHI::TextureHandle texture) { textures_.normalTexture = texture; }
        void SetMetallicRoughnessTexture(RHI::TextureHandle texture) { textures_.metallicRoughnessTexture = texture; }

    private:
        static std::atomic<uint32_t> nextMaterialId_;
    };

    // ========================================================================
    // Mesh System
    // ========================================================================

    class Mesh {
    private:
        RHI::BufferHandle vertexBuffer_;
        RHI::BufferHandle indexBuffer_;
        uint32_t vertexCount_;
        uint32_t indexCount_;
        Math::BoundingBox boundingBox_;
        std::string name_;
        uint32_t meshId_;

    public:
        Mesh(const std::string& name = "");
        ~Mesh() = default;

        // Buffer access
        RHI::BufferHandle GetVertexBuffer() const { return vertexBuffer_; }
        RHI::BufferHandle GetIndexBuffer() const { return indexBuffer_; }
        void SetVertexBuffer(RHI::BufferHandle buffer) { vertexBuffer_ = buffer; }
        void SetIndexBuffer(RHI::BufferHandle buffer) { indexBuffer_ = buffer; }

        // Geometry data
        uint32_t GetVertexCount() const { return vertexCount_; }
        uint32_t GetIndexCount() const { return indexCount_; }
        void SetVertexCount(uint32_t count) { vertexCount_ = count; }
        void SetIndexCount(uint32_t count) { indexCount_ = count; }

        // Bounding volume
        const Math::BoundingBox& GetBoundingBox() const { return boundingBox_; }
        void SetBoundingBox(const Math::BoundingBox& boundingBox) { boundingBox_ = boundingBox; }

        // Identification
        const std::string& GetName() const { return name_; }
        uint32_t GetId() const { return meshId_; }

    private:
        static std::atomic<uint32_t> nextMeshId_;
    };

    // ========================================================================
    // Render Target System
    // ========================================================================

    struct RenderTargetDesc {
        uint32_t width = 1280;
        uint32_t height = 720;
        RHI::Format colorFormat = RHI::Format::B8G8R8A8_UNorm;
        RHI::Format depthFormat = RHI::Format::D32_Float;
        uint32_t sampleCount = 1;
        bool useDepth = true;
        bool allowUAV = false;
        Math::Vector4 clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        float clearDepth = 1.0f;
        uint8_t clearStencil = 0;
        const char* debugName = nullptr;
    };

    class RenderTarget {
    private:
        RHI::TextureHandle colorTexture_;
        RHI::TextureHandle depthTexture_;
        RenderTargetDesc desc_;
        std::string name_;
        uint32_t targetId_;

    public:
        RenderTarget(const std::string& name = "");
        ~RenderTarget() = default;

        // Texture access
        RHI::TextureHandle GetColorTexture() const { return colorTexture_; }
        RHI::TextureHandle GetDepthTexture() const { return depthTexture_; }
        void SetColorTexture(RHI::TextureHandle texture) { colorTexture_ = texture; }
        void SetDepthTexture(RHI::TextureHandle texture) { depthTexture_ = texture; }

        // Description
        const RenderTargetDesc& GetDesc() const { return desc_; }
        void SetDesc(const RenderTargetDesc& desc) { desc_ = desc; }

        // Properties
        uint32_t GetWidth() const { return desc_.width; }
        uint32_t GetHeight() const { return desc_.height; }
        RHI::Format GetColorFormat() const { return desc_.colorFormat; }
        RHI::Format GetDepthFormat() const { return desc_.depthFormat; }

        // Identification
        const std::string& GetName() const { return name_; }
        uint32_t GetId() const { return targetId_; }

    private:
        static std::atomic<uint32_t> nextTargetId_;
    };

    // ========================================================================
    // Draw Commands
    // ========================================================================

    struct DrawCommand {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Material> material;
        Math::Matrix4 worldMatrix;
        Math::BoundingBox worldBounds;
        uint32_t instanceCount = 1;
        uint32_t sortKey = 0;
    };

    struct InstanceData {
        Math::Matrix4 worldMatrix;
        Math::Matrix4 normalMatrix;
        Math::Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
        uint32_t materialIndex = 0;
        uint32_t padding[3] = { 0, 0, 0 };
    };

    // ========================================================================
    // Render Pass System
    // ========================================================================

    enum class RenderPassType : uint32_t {
        Forward = 0,
        Deferred,
        Shadow,
        PostProcess,
        Compute,
        Custom
    };

    class RenderPass {
    public:
        virtual ~RenderPass() = default;
        virtual void Setup(RHI::ICommandList* commandList) = 0;
        virtual void Execute(RHI::ICommandList* commandList, const FrameData& frameData) = 0;
        virtual void Cleanup(RHI::ICommandList* commandList) = 0;
        virtual RenderPassType GetType() const = 0;
        virtual const char* GetName() const = 0;
    };

    // ========================================================================
    // Frame Data
    // ========================================================================

    struct SceneLighting {
        Math::Vector3 ambientColor = { 0.2f, 0.2f, 0.2f };
        std::vector<LightData> lights;
        RHI::TextureHandle shadowMaps[8]; // Support up to 8 shadow casting lights
        Math::Matrix4 shadowMatrices[8];
    };

    struct FrameConstants {
        Math::Matrix4 viewMatrix;
        Math::Matrix4 projectionMatrix;
        Math::Matrix4 viewProjectionMatrix;
        Math::Matrix4 inverseViewMatrix;
        Math::Matrix4 inverseProjectionMatrix;
        Math::Vector4 cameraPosition;
        Math::Vector4 cameraDirection;
        Math::Vector2 screenSize;
        Math::Vector2 invScreenSize;
        float time;
        float deltaTime;
        uint32_t frameNumber;
        uint32_t padding;
    };

    class FrameData {
    private:
        CameraData camera_;
        SceneLighting lighting_;
        FrameConstants constants_;
        std::vector<DrawCommand> drawCommands_;
        std::vector<std::shared_ptr<RenderPass>> renderPasses_;
        RHI::BufferHandle constantBuffer_;

    public:
        FrameData() = default;
        ~FrameData() = default;

        // Camera data
        const CameraData& GetCamera() const { return camera_; }
        void SetCamera(const CameraData& camera) { camera_ = camera; }

        // Lighting data
        const SceneLighting& GetLighting() const { return lighting_; }
        void SetLighting(const SceneLighting& lighting) { lighting_ = lighting; }

        // Frame constants
        const FrameConstants& GetConstants() const { return constants_; }
        void SetConstants(const FrameConstants& constants) { constants_ = constants; }

        // Draw commands
        const std::vector<DrawCommand>& GetDrawCommands() const { return drawCommands_; }
        void AddDrawCommand(const DrawCommand& command) { drawCommands_.push_back(command); }
        void ClearDrawCommands() { drawCommands_.clear(); }

        // Render passes
        const std::vector<std::shared_ptr<RenderPass>>& GetRenderPasses() const { return renderPasses_; }
        void AddRenderPass(std::shared_ptr<RenderPass> pass) { renderPasses_.push_back(pass); }
        void ClearRenderPasses() { renderPasses_.clear(); }

        // Constant buffer
        RHI::BufferHandle GetConstantBuffer() const { return constantBuffer_; }
        void SetConstantBuffer(RHI::BufferHandle buffer) { constantBuffer_ = buffer; }

        // Sorting and culling
        void SortDrawCommands(const CameraData& camera);
        void CullDrawCommands(const CameraData& camera);
    };

    // ========================================================================
    // Renderer Statistics
    // ========================================================================

    struct RendererStats {
        // Frame statistics
        uint64_t frameNumber = 0;
        float frameTime = 0.0f;
        float cpuTime = 0.0f;
        float gpuTime = 0.0f;
        uint32_t fps = 0;

        // Draw statistics
        uint32_t drawCalls = 0;
        uint32_t triangles = 0;
        uint32_t vertices = 0;
        uint32_t instances = 0;

        // Resource statistics
        uint32_t meshCount = 0;
        uint32_t materialCount = 0;
        uint32_t textureCount = 0;
        uint32_t bufferCount = 0;

        // Memory statistics
        uint64_t cpuMemoryUsed = 0;
        uint64_t gpuMemoryUsed = 0;
        uint64_t totalMemoryAvailable = 0;

        // Culling statistics
        uint32_t totalObjects = 0;
        uint32_t visibleObjects = 0;
        uint32_t culledObjects = 0;

        // Light statistics
        uint32_t totalLights = 0;
        uint32_t activeLights = 0;
        uint32_t shadowCastingLights = 0;
    };

    // ========================================================================
    // Main Renderer Interface
    // ========================================================================

    class IRenderer {
    public:
        virtual ~IRenderer() = default;

        // Lifecycle
        virtual bool Initialize(const Configuration::RenderingConfig& config,
            const Platform::SurfaceInfo& surfaceInfo) = 0;
        virtual void Shutdown() = 0;

        // Frame management
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual void Present() = 0;

        // Scene rendering
        virtual void RenderFrame(const FrameData& frameData) = 0;
        virtual void SetCamera(const CameraData& camera) = 0;
        virtual void SetLighting(const SceneLighting& lighting) = 0;

        // Resource creation
        virtual std::shared_ptr<Mesh> CreateMesh(const MeshData& meshData) = 0;
        virtual std::shared_ptr<Material> CreateMaterial(const std::string& name = "") = 0;
        virtual std::shared_ptr<RenderTarget> CreateRenderTarget(const RenderTargetDesc& desc) = 0;

        // Resource loading
        virtual std::shared_ptr<Mesh> LoadMesh(const std::string& filepath) = 0;
        virtual RHI::TextureHandle LoadTexture(const std::string& filepath) = 0;

        // Draw commands
        virtual void DrawMesh(const Mesh& mesh, const Material& material, const Math::Matrix4& worldMatrix) = 0;
        virtual void DrawMeshInstanced(const Mesh& mesh, const Material& material,
            const std::vector<InstanceData>& instances) = 0;

        // Render targets
        virtual void SetRenderTarget(std::shared_ptr<RenderTarget> target) = 0;
        virtual void SetMainRenderTarget() = 0;
        virtual void ClearRenderTarget(const Math::Vector4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) = 0;
        virtual void ClearDepthStencil(float depth = 1.0f, uint8_t stencil = 0) = 0;

        // Debug features
        virtual void SetDebugName(const char* name) = 0;
        virtual const char* GetDebugName() const = 0;
        virtual void BeginDebugEvent(const char* name) = 0;
        virtual void EndDebugEvent() = 0;

        // Statistics and diagnostics
        virtual const RendererStats& GetStats() const = 0;
        virtual void ResetStats() = 0;

        // Device access
        virtual RHI::IRenderDevice* GetDevice() = 0;
        virtual const RHI::DeviceCapabilities& GetDeviceCapabilities() const = 0;
    };

    // ========================================================================
    // Standard Renderer Implementation
    // ========================================================================

    class StandardRenderer : public IRenderer {
    private:
        // Core systems
        std::unique_ptr<RHI::IRenderDevice> device_;
        std::unique_ptr<RHI::IRHIFactory> rhiFactory_;

        // Current frame data
        std::unique_ptr<FrameData> currentFrameData_;
        CameraData currentCamera_;
        SceneLighting currentLighting_;

        // Resource management
        std::vector<std::shared_ptr<Mesh>> meshes_;
        std::vector<std::shared_ptr<Material>> materials_;
        std::vector<std::shared_ptr<RenderTarget>> renderTargets_;
        std::unordered_map<std::string, RHI::TextureHandle> textureCache_;

        // Render state
        std::shared_ptr<RenderTarget> currentRenderTarget_;
        std::unique_ptr<RHI::ICommandList> mainCommandList_;

        // Built-in render passes
        std::vector<std::unique_ptr<RenderPass>> standardRenderPasses_;

        // Configuration
        Configuration::RenderingConfig config_;
        Platform::RendererSurfaceInfo surfaceInfo_;

        // Statistics
        RendererStats stats_;
        std::chrono::high_resolution_clock::time_point frameStartTime_;

        // Threading and synchronization
        std::mutex renderMutex_;
        std::atomic<bool> isInitialized_{ false };
        std::atomic<uint64_t> frameNumber_{ 0 };

        // Debug
        std::string debugName_;
        Logging::LogChannel& logChannel_;

    public:
        StandardRenderer();
        virtual ~StandardRenderer();

        // IRenderer implementation
        bool Initialize(const Configuration::RenderingConfig& config,
            const Platform::SurfaceInfo& surfaceInfo) override;
        void Shutdown() override;

        void BeginFrame() override;
        void EndFrame() override;
        void Present() override;

        void RenderFrame(const FrameData& frameData) override;
        void SetCamera(const CameraData& camera) override;
        void SetLighting(const SceneLighting& lighting) override;

        std::shared_ptr<Mesh> CreateMesh(const MeshData& meshData) override;
        std::shared_ptr<Material> CreateMaterial(const std::string& name = "") override;
        std::shared_ptr<RenderTarget> CreateRenderTarget(const RenderTargetDesc& desc) override;

        std::shared_ptr<Mesh> LoadMesh(const std::string& filepath) override;
        RHI::TextureHandle LoadTexture(const std::string& filepath) override;

        void DrawMesh(const Mesh& mesh, const Material& material, const Math::Matrix4& worldMatrix) override;
        void DrawMeshInstanced(const Mesh& mesh, const Material& material,
            const std::vector<InstanceData>& instances) override;

        void SetRenderTarget(std::shared_ptr<RenderTarget> target) override;
        void SetMainRenderTarget() override;
        void ClearRenderTarget(const Math::Vector4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) override;
        void ClearDepthStencil(float depth = 1.0f, uint8_t stencil = 0) override;

        void SetDebugName(const char* name) override;
        const char* GetDebugName() const override { return debugName_.c_str(); }
        void BeginDebugEvent(const char* name) override;
        void EndDebugEvent() override;

        const RendererStats& GetStats() const override { return stats_; }
        void ResetStats() override;

        RHI::IRenderDevice* GetDevice() override { return device_.get(); }
        const RHI::DeviceCapabilities& GetDeviceCapabilities() const override;

    private:
        // Initialization helpers
        bool InitializeRHI();
        bool InitializeRenderPasses();
        bool InitializeDefaultResources();

        // Frame processing
        void UpdateFrameConstants();
        void ProcessDrawCommands(const FrameData& frameData);
        void ExecuteRenderPasses(const FrameData& frameData);

        // Resource helpers
        RHI::BufferHandle CreateVertexBuffer(const std::vector<Vertex>& vertices);
        RHI::BufferHandle CreateIndexBuffer(const std::vector<uint32_t>& indices);

        // Statistics
        void UpdateStats();
        void CalculateFrameTiming();

        // Debug helpers
        void LogRendererInfo();
        void ValidateRenderState();

        // Cleanup
        void CleanupResources();
    };

    // ========================================================================
    // Factory Functions
    // ========================================================================

    // Create renderer instance
    std::unique_ptr<IRenderer> CreateRenderer();

    // Create standard renderer
    std::unique_ptr<StandardRenderer> CreateStandardRenderer();

} // namespace Akhanda::Renderer
