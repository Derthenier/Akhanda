// ForwardRenderPass.hpp
// Akhanda Game Engine - Forward Rendering Pass Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>

import Akhanda.Engine.Renderer;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Core.Math;
import Akhanda.Core.Logging;
import std;

namespace Akhanda::Renderer {

    // ========================================================================
    // Forward Render Pass
    // ========================================================================

    class ForwardRenderPass : public RenderPass {
    private:
        // Render state
        RHI::IRenderDevice* device_ = nullptr;

        // Pipeline states
        RHI::PipelineHandle opaquePipeline_;
        RHI::PipelineHandle transparentPipeline_;
        RHI::PipelineHandle wireframePipeline_;

        // Buffers
        RHI::BufferHandle sceneConstantBuffer_;
        RHI::BufferHandle lightingBuffer_;
        RHI::BufferHandle materialBuffer_;
        RHI::BufferHandle instanceBuffer_;

        // Default resources
        RHI::TextureHandle whiteTexture_;
        RHI::TextureHandle blackTexture_;
        RHI::TextureHandle defaultNormalTexture_;

        // Render settings
        struct RenderSettings {
            bool enableWireframe = false;
            bool enableDepthTest = true;
            bool enableAlphaBlending = true;
            bool enableBackfaceCulling = true;
            bool enableEarlyZTest = true;
            float wireframeWidth = 1.0f;
        };
        RenderSettings settings_;

        // Statistics
        struct PassStats {
            uint32_t opaqueDrawCalls = 0;
            uint32_t transparentDrawCalls = 0;
            uint32_t totalTriangles = 0;
            uint32_t totalVertices = 0;
            uint32_t materialSwitches = 0;
            uint32_t pipelineSwitches = 0;
        };
        mutable PassStats stats_;

        // Shader constants
        struct SceneConstants {
            Math::Matrix4 viewMatrix;
            Math::Matrix4 projectionMatrix;
            Math::Matrix4 viewProjectionMatrix;
            Math::Matrix4 inverseViewMatrix;
            Math::Matrix4 inverseProjectionMatrix;
            Math::Vector4 cameraPosition;
            Math::Vector4 cameraDirection;
            Math::Vector2 screenSize;
            Math::Vector2 inverseScreenSize;
            float time;
            float deltaTime;
            uint32_t frameNumber;
            uint32_t lightCount;
        };

        struct LightConstants {
            static constexpr uint32_t MAX_LIGHTS = 256;

            struct LightData {
                Math::Vector4 positionAndType;      // xyz = position, w = type
                Math::Vector4 directionAndRange;    // xyz = direction, w = range
                Math::Vector4 colorAndIntensity;    // xyz = color, w = intensity
                Math::Vector4 attenuationAndAngles; // x = linear, y = quadratic, z = inner angle, w = outer angle
                Math::Vector4 shadowData;           // x = cast shadows, y = shadow map index, z = shadow bias, w = padding
            };

            LightData lights[MAX_LIGHTS];
            uint32_t lightCount = 0;
            Math::Vector3 ambientColor = { 0.2f, 0.2f, 0.2f };
        };

        struct MaterialConstants {
            Math::Vector4 albedoColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            Math::Vector3 emissiveColor = { 0.0f, 0.0f, 0.0f };
            float metallic = 0.0f;
            float roughness = 0.5f;
            float normalScale = 1.0f;
            float occlusionStrength = 1.0f;
            float alphaCutoff = 0.5f;
            uint32_t hasAlbedoTexture = 0;
            uint32_t hasNormalTexture = 0;
            uint32_t hasMetallicRoughnessTexture = 0;
            uint32_t hasOcclusionTexture = 0;
            uint32_t hasEmissiveTexture = 0;
            uint32_t doubleSided = 0;
            uint32_t alphaMode = 0; // 0 = opaque, 1 = mask, 2 = blend
        };

        // Batching system
        struct RenderBatch {
            std::shared_ptr<Material> material;
            std::shared_ptr<Mesh> mesh;
            std::vector<Math::Matrix4> worldMatrices;
            std::vector<InstanceData> instances;
        };

        std::vector<RenderBatch> opaqueBatches_;
        std::vector<RenderBatch> transparentBatches_;

        // Debug
        std::string passName_;
        Core::Logging::LogChannel& logChannel_;

    public:
        ForwardRenderPass(RHI::IRenderDevice* device);
        virtual ~ForwardRenderPass();

        // RenderPass interface
        void Setup(RHI::ICommandList* commandList) override;
        void Execute(RHI::ICommandList* commandList, const FrameData& frameData) override;
        void Cleanup(RHI::ICommandList* commandList) override;
        RenderPassType GetType() const override { return RenderPassType::Forward; }
        const char* GetName() const override { return passName_.c_str(); }

        // Configuration
        void SetRenderSettings(const RenderSettings& settings) { settings_ = settings; }
        const RenderSettings& GetRenderSettings() const { return settings_; }

        // Statistics
        const PassStats& GetStats() const { return stats_; }
        void ResetStats() const { stats_ = {}; }

    private:
        // Initialization
        bool InitializePipelines();
        bool InitializeBuffers();
        bool InitializeDefaultTextures();

        // Pipeline creation
        RHI::PipelineHandle CreateOpaquePipeline();
        RHI::PipelineHandle CreateTransparentPipeline();
        RHI::PipelineHandle CreateWireframePipeline();

        // Resource management
        void UpdateSceneConstants(const FrameData& frameData);
        void UpdateLightingConstants(const SceneLighting& lighting);
        void UpdateMaterialConstants(const Material& material);

        // Batching
        void BuildRenderBatches(const FrameData& frameData);
        void BatchDrawCommands(const std::vector<DrawCommand>& drawCommands);
        void SortBatches();

        // Rendering
        void RenderOpaqueBatches(RHI::ICommandList* commandList);
        void RenderTransparentBatches(RHI::ICommandList* commandList);
        void RenderBatch(RHI::ICommandList* commandList, const RenderBatch& batch, bool isTransparent);

        // Material binding
        void BindMaterial(RHI::ICommandList* commandList, const Material& material);
        void BindDefaultMaterial(RHI::ICommandList* commandList);

        // Utility functions
        bool IsTransparentMaterial(const Material& material) const;
        uint32_t CalculateSortKey(const DrawCommand& command, const CameraData& camera) const;

        // Validation
        bool ValidateRenderBatch(const RenderBatch& batch) const;
        bool ValidateDrawCommand(const DrawCommand& command) const;

        // Debug helpers
        void LogPassStatistics() const;
        void ValidatePassState() const;

        // Cleanup
        void CleanupBatches();
        void CleanupResources();
    };

    // ========================================================================
    // Forward Render Pass Factory
    // ========================================================================

    class ForwardRenderPassFactory {
    public:
        static std::unique_ptr<ForwardRenderPass> Create(RHI::IRenderDevice* device);
        static std::unique_ptr<ForwardRenderPass> CreateWithSettings(RHI::IRenderDevice* device,
            const ForwardRenderPass::RenderSettings& settings);
    };

    // ========================================================================
    // Shader Source (Embedded for Simplicity)
    // ========================================================================

    namespace ForwardPassShaders {

        // Vertex shader for forward rendering
        constexpr const char* VertexShaderSource = R"(
            struct VSInput {
                float3 position : POSITION;
                float3 normal : NORMAL;
                float2 texCoord : TEXCOORD0;
                float3 tangent : TANGENT;
                float3 bitangent : BITANGENT;
                float4 color : COLOR;
            };

            struct VSOutput {
                float4 position : SV_Position;
                float3 worldPos : POSITION;
                float3 normal : NORMAL;
                float2 texCoord : TEXCOORD0;
                float3 tangent : TANGENT;
                float3 bitangent : BITANGENT;
                float4 color : COLOR;
                float3 viewPos : TEXCOORD1;
            };

            cbuffer SceneConstants : register(b0) {
                float4x4 viewMatrix;
                float4x4 projectionMatrix;
                float4x4 viewProjectionMatrix;
                float4x4 inverseViewMatrix;
                float4x4 inverseProjectionMatrix;
                float4 cameraPosition;
                float4 cameraDirection;
                float2 screenSize;
                float2 inverseScreenSize;
                float time;
                float deltaTime;
                uint frameNumber;
                uint lightCount;
            };

            cbuffer ObjectConstants : register(b1) {
                float4x4 worldMatrix;
                float4x4 normalMatrix;
            };

            VSOutput main(VSInput input) {
                VSOutput output;
                
                float4 worldPos = mul(float4(input.position, 1.0), worldMatrix);
                output.worldPos = worldPos.xyz;
                output.position = mul(worldPos, viewProjectionMatrix);
                
                output.normal = normalize(mul(input.normal, (float3x3)normalMatrix));
                output.tangent = normalize(mul(input.tangent, (float3x3)normalMatrix));
                output.bitangent = normalize(mul(input.bitangent, (float3x3)normalMatrix));
                
                output.texCoord = input.texCoord;
                output.color = input.color;
                
                output.viewPos = mul(worldPos, viewMatrix).xyz;
                
                return output;
            }
        )";

        // Pixel shader for forward rendering (PBR)
        constexpr const char* PixelShaderSource = R"(
            struct PSInput {
                float4 position : SV_Position;
                float3 worldPos : POSITION;
                float3 normal : NORMAL;
                float2 texCoord : TEXCOORD0;
                float3 tangent : TANGENT;
                float3 bitangent : BITANGENT;
                float4 color : COLOR;
                float3 viewPos : TEXCOORD1;
            };

            struct LightData {
                float4 positionAndType;
                float4 directionAndRange;
                float4 colorAndIntensity;
                float4 attenuationAndAngles;
                float4 shadowData;
            };

            cbuffer SceneConstants : register(b0) {
                float4x4 viewMatrix;
                float4x4 projectionMatrix;
                float4x4 viewProjectionMatrix;
                float4x4 inverseViewMatrix;
                float4x4 inverseProjectionMatrix;
                float4 cameraPosition;
                float4 cameraDirection;
                float2 screenSize;
                float2 inverseScreenSize;
                float time;
                float deltaTime;
                uint frameNumber;
                uint lightCount;
            };

            cbuffer LightConstants : register(b1) {
                LightData lights[256];
                uint activeLightCount;
                float3 ambientColor;
            };

            cbuffer MaterialConstants : register(b2) {
                float4 albedoColor;
                float3 emissiveColor;
                float metallic;
                float roughness;
                float normalScale;
                float occlusionStrength;
                float alphaCutoff;
                uint hasAlbedoTexture;
                uint hasNormalTexture;
                uint hasMetallicRoughnessTexture;
                uint hasOcclusionTexture;
                uint hasEmissiveTexture;
                uint doubleSided;
                uint alphaMode;
            };

            Texture2D albedoTexture : register(t0);
            Texture2D normalTexture : register(t1);
            Texture2D metallicRoughnessTexture : register(t2);
            Texture2D occlusionTexture : register(t3);
            Texture2D emissiveTexture : register(t4);

            SamplerState linearSampler : register(s0);

            static const float PI = 3.14159265359;

            float3 getNormalFromMap(PSInput input) {
                if (hasNormalTexture) {
                    float3 tangentNormal = normalTexture.Sample(linearSampler, input.texCoord).xyz * 2.0 - 1.0;
                    tangentNormal.xy *= normalScale;
                    
                    float3 N = normalize(input.normal);
                    float3 T = normalize(input.tangent);
                    float3 B = normalize(input.bitangent);
                    float3x3 TBN = float3x3(T, B, N);
                    
                    return normalize(mul(tangentNormal, TBN));
                }
                return normalize(input.normal);
            }

            float DistributionGGX(float3 N, float3 H, float roughness) {
                float a = roughness * roughness;
                float a2 = a * a;
                float NdotH = max(dot(N, H), 0.0);
                float NdotH2 = NdotH * NdotH;
                
                float nom = a2;
                float denom = (NdotH2 * (a2 - 1.0) + 1.0);
                denom = PI * denom * denom;
                
                return nom / denom;
            }

            float GeometrySchlickGGX(float NdotV, float roughness) {
                float r = (roughness + 1.0);
                float k = (r * r) / 8.0;
                
                float nom = NdotV;
                float denom = NdotV * (1.0 - k) + k;
                
                return nom / denom;
            }

            float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
                float NdotV = max(dot(N, V), 0.0);
                float NdotL = max(dot(N, L), 0.0);
                float ggx2 = GeometrySchlickGGX(NdotV, roughness);
                float ggx1 = GeometrySchlickGGX(NdotL, roughness);
                
                return ggx1 * ggx2;
            }

            float3 fresnelSchlick(float cosTheta, float3 F0) {
                return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
            }

            float3 calculateLighting(PSInput input, float3 albedo, float metallic, float roughness, float3 normal) {
                float3 V = normalize(cameraPosition.xyz - input.worldPos);
                float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
                
                float3 Lo = float3(0.0, 0.0, 0.0);
                
                for (uint i = 0; i < min(activeLightCount, 256u); ++i) {
                    LightData light = lights[i];
                    uint lightType = (uint)light.positionAndType.w;
                    
                    float3 L = float3(0.0, 0.0, 0.0);
                    float attenuation = 1.0;
                    
                    if (lightType == 0) { // Directional light
                        L = normalize(-light.directionAndRange.xyz);
                    } else if (lightType == 1) { // Point light
                        L = normalize(light.positionAndType.xyz - input.worldPos);
                        float distance = length(light.positionAndType.xyz - input.worldPos);
                        attenuation = 1.0 / (1.0 + light.attenuationAndAngles.x * distance + 
                                           light.attenuationAndAngles.y * (distance * distance));
                    } else if (lightType == 2) { // Spot light
                        L = normalize(light.positionAndType.xyz - input.worldPos);
                        float distance = length(light.positionAndType.xyz - input.worldPos);
                        float theta = dot(L, normalize(-light.directionAndRange.xyz));
                        float epsilon = light.attenuationAndAngles.z - light.attenuationAndAngles.w;
                        float intensity = clamp((theta - light.attenuationAndAngles.w) / epsilon, 0.0, 1.0);
                        attenuation = intensity / (1.0 + light.attenuationAndAngles.x * distance + 
                                                 light.attenuationAndAngles.y * (distance * distance));
                    }
                    
                    float3 H = normalize(V + L);
                    float3 radiance = light.colorAndIntensity.xyz * light.colorAndIntensity.w * attenuation;
                    
                    float NDF = DistributionGGX(normal, H, roughness);
                    float G = GeometrySmith(normal, V, L, roughness);
                    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
                    
                    float3 kS = F;
                    float3 kD = float3(1.0, 1.0, 1.0) - kS;
                    kD *= 1.0 - metallic;
                    
                    float3 numerator = NDF * G * F;
                    float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0);
                    float3 specular = numerator / max(denominator, 0.001);
                    
                    float NdotL = max(dot(normal, L), 0.0);
                    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
                }
                
                float3 ambient = ambientColor * albedo;
                float3 color = ambient + Lo;
                
                return color;
            }

            float4 main(PSInput input) : SV_Target {
                // Sample material textures
                float4 baseColor = albedoColor * input.color;
                if (hasAlbedoTexture) {
                    baseColor *= albedoTexture.Sample(linearSampler, input.texCoord);
                }
                
                // Alpha testing
                if (alphaMode == 1 && baseColor.a < alphaCutoff) {
                    discard;
                }
                
                float3 albedo = baseColor.rgb;
                float alpha = baseColor.a;
                
                float metallicValue = metallic;
                float roughnessValue = roughness;
                if (hasMetallicRoughnessTexture) {
                    float3 mrSample = metallicRoughnessTexture.Sample(linearSampler, input.texCoord).rgb;
                    metallicValue *= mrSample.b;
                    roughnessValue *= mrSample.g;
                }
                
                float3 normal = getNormalFromMap(input);
                
                // Calculate lighting
                float3 color = calculateLighting(input, albedo, metallicValue, roughnessValue, normal);
                
                // Add emissive
                if (hasEmissiveTexture) {
                    color += emissiveTexture.Sample(linearSampler, input.texCoord).rgb * emissiveColor;
                } else {
                    color += emissiveColor;
                }
                
                // Apply occlusion
                if (hasOcclusionTexture) {
                    float occlusion = occlusionTexture.Sample(linearSampler, input.texCoord).r;
                    color = lerp(color, color * occlusion, occlusionStrength);
                }
                
                // Tone mapping (simple Reinhard)
                color = color / (color + float3(1.0, 1.0, 1.0));
                
                // Gamma correction
                color = pow(color, float3(1.0/2.2, 1.0/2.2, 1.0/2.2));
                
                return float4(color, alpha);
            }
        )";

    } // namespace ForwardPassShaders

} // namespace Akhanda::Renderer
