// Shaders/Constants.hlsli
// Akhanda Game Engine - Constant Buffer Definitions
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#ifndef AKHANDA_CONSTANTS_HLSLI
#define AKHANDA_CONSTANTS_HLSLI

#include "Common.hlsli"

// ============================================================================
// Constant Buffer Register Assignments
// ============================================================================

// Standard register layout for Akhanda shaders
// b0-b7: Per-frame constants (updated once per frame)
// b8-b15: Per-view constants (updated per camera/viewport)
// b16-b23: Per-object constants (updated per draw call)
// b24-b31: Per-material constants (updated per material)

#define REGISTER_FRAME_CONSTANTS        b0
#define REGISTER_TIME_CONSTANTS         b1
#define REGISTER_ENGINE_CONSTANTS       b2
#define REGISTER_POST_PROCESS_CONSTANTS b3

#define REGISTER_CAMERA_CONSTANTS       b8
#define REGISTER_LIGHTING_CONSTANTS     b9
#define REGISTER_SHADOW_CONSTANTS       b10
#define REGISTER_FOG_CONSTANTS          b11

#define REGISTER_OBJECT_CONSTANTS       b16
#define REGISTER_SKINNING_CONSTANTS     b17
#define REGISTER_INSTANCING_CONSTANTS   b18

#define REGISTER_MATERIAL_CONSTANTS     b24
#define REGISTER_PBR_CONSTANTS          b25
#define REGISTER_TEXTURE_INDICES        b26

// ============================================================================
// Frame-Level Constants (Updated Once Per Frame)
// ============================================================================

cbuffer FrameConstants : register(REGISTER_FRAME_CONSTANTS)
{
    // Screen and viewport information
    float2 screenSize; // Screen resolution (width, height)
    float2 invScreenSize; // 1.0 / screenSize
    
    float2 viewportSize; // Current viewport size
    float2 invViewportSize; // 1.0 / viewportSize
    
    float2 viewportOffset; // Viewport offset from screen origin
    float2 _frameConstants_pad0; // Padding for alignment
    
    // Frame information
    uint frameNumber; // Current frame number
    uint renderPassIndex; // Current render pass (0=main, 1=shadow, etc.)
    float frameTime; // Time taken for previous frame (seconds)
    float averageFrameTime; // Running average frame time
    
    // Quality and LOD settings
    float globalLODBias; // Global LOD bias for textures
    float globalLODScale; // Global LOD scaling factor
    uint maxAnisotropy; // Maximum anisotropic filtering
    uint msaaSamples; // MSAA sample count (1, 2, 4, 8, 16)
    
    // Rendering features flags (bit flags)
    uint renderingFlags; // Bit flags for enabled features
    uint debugVisualizationMode; // Debug visualization mode
    float debugIntensity; // Debug visualization intensity
    float _frameConstants_pad1; // Padding for alignment
};

// Rendering flags bit definitions
#define RENDERING_FLAG_VSYNC_ENABLED        (1u << 0)
#define RENDERING_FLAG_HDR_ENABLED          (1u << 1)
#define RENDERING_FLAG_GAMMA_CORRECTION     (1u << 2)
#define RENDERING_FLAG_TONEMAPPING_ENABLED  (1u << 3)
#define RENDERING_FLAG_BLOOM_ENABLED        (1u << 4)
#define RENDERING_FLAG_SSAO_ENABLED         (1u << 5)
#define RENDERING_FLAG_SSR_ENABLED          (1u << 6)
#define RENDERING_FLAG_MOTION_BLUR_ENABLED  (1u << 7)

// ============================================================================
// Time Constants (Updated Once Per Frame)
// ============================================================================

cbuffer TimeConstants : register(REGISTER_TIME_CONSTANTS)
{
    // Time values
    float totalTime; // Total elapsed time since engine start
    float deltaTime; // Time since last frame
    float unscaledTotalTime; // Total time ignoring time scale
    float unscaledDeltaTime; // Delta time ignoring time scale
    
    float timeScale; // Global time scaling factor
    float pausedTime; // Time spent paused
    float applicationTime; // Time since application start
    float levelTime; // Time since current level loaded
    
    // Periodic time functions (pre-calculated for performance)
    float sinTime; // sin(totalTime)
    float cosTime; // cos(totalTime)
    float sinTimeHalf; // sin(totalTime * 0.5)
    float cosTimeHalf; // cos(totalTime * 0.5)
    
    float sinTimeDouble; // sin(totalTime * 2.0)
    float cosTimeDouble; // cos(totalTime * 2.0)
    float sinTimeQuad; // sin(totalTime * 4.0)
    float cosTimeQuad; // cos(totalTime * 4.0)
};

// ============================================================================
// Engine-Level Constants (Updated Infrequently)
// ============================================================================

cbuffer EngineConstants : register(REGISTER_ENGINE_CONSTANTS)
{
    // Version and build information
    uint engineVersionMajor; // Engine major version
    uint engineVersionMinor; // Engine minor version
    uint engineVersionPatch; // Engine patch version
    uint engineBuildNumber; // Engine build number
    
    // Platform and API information
    uint renderingAPI; // 0=D3D12, 1=Vulkan, 2=D3D11
    uint shaderModel; // Shader model version (60, 61, 62, etc.)
    uint platformFlags; // Platform-specific flags
    uint gpuVendorID; // GPU vendor ID (0x10DE=NVIDIA, 0x1002=AMD, etc.)
    
    // Global rendering settings
    float globalExposure; // Global exposure compensation
    float globalGamma; // Global gamma correction value
    float globalContrast; // Global contrast adjustment
    float globalSaturation; // Global saturation adjustment
    
    float globalBrightness; // Global brightness adjustment
    float globalVibrance; // Global vibrance adjustment
    float globalHueShift; // Global hue shift (-180 to 180)
    float _engineConstants_pad0; // Padding for alignment
    
    // Quality settings
    float shadowQuality; // Shadow quality multiplier (0.5 - 2.0)
    float textureQuality; // Texture quality multiplier (0.5 - 2.0)
    float effectsQuality; // Effects quality multiplier (0.5 - 2.0)
    float geometryQuality; // Geometry quality multiplier (0.5 - 2.0)
};

// ============================================================================
// Camera Constants (Updated Per View)
// ============================================================================

cbuffer CameraConstants : register(REGISTER_CAMERA_CONSTANTS)
{
    // Transform matrices
    float4x4 viewMatrix; // World to view space transform
    float4x4 projectionMatrix; // View to clip space transform
    float4x4 viewProjectionMatrix; // Combined view-projection matrix
    float4x4 invViewMatrix; // View to world space transform
    
    float4x4 invProjectionMatrix; // Clip to view space transform
    float4x4 invViewProjectionMatrix; // Clip to world space transform
    float4x4 prevViewProjectionMatrix; // Previous frame VP (for motion vectors)
    float4x4 jitteredProjectionMatrix; // Jittered projection (for TAA)
    
    // Camera properties
    float3 cameraPosition; // Camera position in world space
    float nearPlane; // Near clipping plane distance
    
    float3 cameraDirection; // Camera forward direction
    float farPlane; // Far clipping plane distance
    
    float3 cameraUp; // Camera up direction
    float fieldOfView; // Vertical field of view in radians
    
    float3 cameraRight; // Camera right direction
    float aspectRatio; // Width / Height aspect ratio
    
    // Frustum planes (for culling)
    float4 frustumPlanes[6]; // Left, Right, Top, Bottom, Near, Far
    
    // Projection parameters
    float2 projectionParams; // x = near*far/(far-near), y = far/(far-near)
    float2 clipPlanes; // x = near, y = far
    
    // Temporal data
    float2 jitterOffset; // Current frame jitter offset (for TAA)
    float2 prevJitterOffset; // Previous frame jitter offset
    
    // Camera movement data (for motion blur)
    float3 cameraVelocity; // Camera velocity in world space
    float cameraSpeed; // Camera speed magnitude
    
    float3 angularVelocity; // Camera angular velocity
    float _cameraConstants_pad0; // Padding for alignment
};

// ============================================================================
// Lighting Constants (Updated Per Frame or When Lights Change)
// ============================================================================

// Maximum number of lights supported
#define MAX_DIRECTIONAL_LIGHTS  4
#define MAX_POINT_LIGHTS        64
#define MAX_SPOT_LIGHTS         32
#define MAX_AREA_LIGHTS         16

// Light data structures
struct DirectionalLight
{
    float3 direction; // Light direction (normalized)
    float intensity; // Light intensity
    
    float3 color; // Light color (linear RGB)
    float angularSize; // Angular size for soft shadows (radians)
    
    float4x4 lightViewProjection; // Shadow map view-projection matrix
    
    uint shadowMapIndex; // Index into shadow map array
    uint castsShadows; // 1 if this light casts shadows
    float shadowBias; // Shadow bias to prevent acne
    float shadowNormalBias; // Normal-based shadow bias
    
    float shadowStrength; // Shadow opacity (0-1)
    float shadowSoftness; // Shadow softness factor
    float2 _directionalLight_pad; // Padding for alignment
};

struct PointLight
{
    float3 position; // Light position in world space
    float intensity; // Light intensity
    
    float3 color; // Light color (linear RGB)
    float range; // Light range/radius
    
    float3 attenuation; // Constant, linear, quadratic attenuation
    float sourceRadius; // Source radius for soft shadows
    
    uint shadowMapIndex; // Index into shadow cube map array
    uint castsShadows; // 1 if this light casts shadows
    float shadowBias; // Shadow bias
    float shadowNormalBias; // Normal-based shadow bias
};

struct SpotLight
{
    float3 position; // Light position in world space
    float intensity; // Light intensity
    
    float3 direction; // Light direction (normalized)
    float range; // Light range/radius
    
    float3 color; // Light color (linear RGB)
    float innerConeAngle; // Inner cone angle (radians)
    
    float3 attenuation; // Constant, linear, quadratic attenuation
    float outerConeAngle; // Outer cone angle (radians)
    
    float4x4 lightViewProjection; // Shadow map view-projection matrix
    
    uint shadowMapIndex; // Index into shadow map array
    uint castsShadows; // 1 if this light casts shadows
    float shadowBias; // Shadow bias
    float shadowNormalBias; // Normal-based shadow bias
    
    float sourceRadius; // Source radius for soft shadows
    float shadowStrength; // Shadow opacity (0-1)
    float2 _spotLight_pad; // Padding for alignment
};

cbuffer LightingConstants : register(REGISTER_LIGHTING_CONSTANTS)
{
    // Ambient lighting
    float3 ambientColor; // Global ambient light color
    float ambientIntensity; // Global ambient intensity
    
    // Environment lighting
    float3 skyColor; // Sky color for hemisphere lighting
    float skyIntensity; // Sky intensity
    
    float3 groundColor; // Ground color for hemisphere lighting
    float groundIntensity; // Ground intensity
    
    // Light counts
    uint numDirectionalLights; // Number of active directional lights
    uint numPointLights; // Number of active point lights
    uint numSpotLights; // Number of active spot lights
    uint numAreaLights; // Number of active area lights
    
    // Environment mapping
    uint environmentMapIndex; // Index of environment/skybox texture
    uint irradianceMapIndex; // Index of irradiance map for IBL
    uint prefilterMapIndex; // Index of prefiltered environment map
    uint brdfLutIndex; // Index of BRDF integration LUT
    
    // Light arrays (dynamically sized based on counts)
    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    PointLight pointLights[MAX_POINT_LIGHTS];
    SpotLight spotLights[MAX_SPOT_LIGHTS];
};

// ============================================================================
// Object Constants (Updated Per Draw Call)
// ============================================================================

cbuffer ObjectConstants : register(REGISTER_OBJECT_CONSTANTS)
{
    // Transform matrices
    float4x4 worldMatrix; // Object to world transform
    float4x4 invWorldMatrix; // World to object transform
    float4x4 normalMatrix; // Normal transformation matrix
    float4x4 prevWorldMatrix; // Previous frame world matrix (for motion vectors)
    
    // Object properties
    float3 objectBoundsMin; // Object bounding box minimum
    float objectScale; // Uniform scale factor
    
    float3 objectBoundsMax; // Object bounding box maximum
    float objectTransparency; // Object transparency (0-1)
    
    float3 objectCenter; // Object center in world space
    uint objectID; // Unique object identifier
    
    // Material binding
    uint materialIndex; // Index into material buffer
    uint diffuseTextureIndex; // Index of diffuse/albedo texture
    uint normalTextureIndex; // Index of normal map texture
    uint specularTextureIndex; // Index of specular/metallic texture
    
    // Rendering properties
    uint renderingFlags; // Per-object rendering flags
    float lodBias; // Per-object LOD bias
    float lodDistance; // Distance-based LOD factor
    uint lodLevel; // Current LOD level
    
    // Animation data
    float animationTime; // Current animation time
    uint animationIndex; // Current animation index
    float morphWeight; // Morph target weight
    uint _objectConstants_pad0; // Padding for alignment
};

// Object rendering flags
#define OBJECT_FLAG_CAST_SHADOWS        (1u << 0)
#define OBJECT_FLAG_RECEIVE_SHADOWS     (1u << 1)
#define OBJECT_FLAG_MOTION_VECTORS      (1u << 2)
#define OBJECT_FLAG_TRANSPARENT         (1u << 3)
#define OBJECT_FLAG_TWO_SIDED           (1u << 4)
#define OBJECT_FLAG_WIREFRAME           (1u << 5)
#define OBJECT_FLAG_INSTANCED           (1u << 6)
#define OBJECT_FLAG_SKINNED             (1u << 7)

// ============================================================================
// Material Constants (Updated Per Material)
// ============================================================================

cbuffer MaterialConstants : register(REGISTER_MATERIAL_CONSTANTS)
{
    // PBR material properties
    float4 albedoColor; // Base color/albedo (RGB + alpha)
    float3 emissiveColor; // Emissive color
    float emissiveIntensity; // Emissive intensity multiplier
    
    float metallic; // Metallic factor (0-1)
    float roughness; // Roughness factor (0-1)
    float specular; // Specular factor (0-1, for dielectric materials)
    float normalIntensity; // Normal map intensity (0-2)
    
    float occlusionStrength; // Ambient occlusion strength (0-1)
    float heightScale; // Height/parallax mapping scale
    float detailScale; // Detail texture tiling scale
    float clearCoat; // Clear coat factor (0-1)
    
    // Texture indices
    uint albedoTexture; // Albedo/diffuse texture index
    uint normalTexture; // Normal map texture index
    uint metallicRoughnessTexture; // Metallic-roughness texture index
    uint emissiveTexture; // Emissive texture index
    
    uint occlusionTexture; // Ambient occlusion texture index
    uint heightTexture; // Height/displacement texture index
    uint detailAlbedoTexture; // Detail albedo texture index
    uint detailNormalTexture; // Detail normal texture index
    
    // Material flags and properties
    uint materialFlags; // Material feature flags
    float alphaTestThreshold; // Alpha test cutoff threshold
    float indexOfRefraction; // IOR for transmission/refraction
    float transmission; // Transmission factor (0-1)
    
    // UV transform
    float2 uvOffset; // UV coordinate offset
    float2 uvScale; // UV coordinate scale
    
    float uvRotation; // UV rotation in radians
    float anisotropy; // Anisotropic factor (-1 to 1)
    float anisotropyRotation; // Anisotropy rotation
    float sheen; // Sheen factor (0-1)
    
    float3 sheenColor; // Sheen color tint
    float sheenRoughness; // Sheen roughness (0-1)
};

// Material flags
#define MATERIAL_FLAG_ALPHA_TEST        (1u << 0)
#define MATERIAL_FLAG_ALPHA_BLEND       (1u << 1)
#define MATERIAL_FLAG_DOUBLE_SIDED      (1u << 2)
#define MATERIAL_FLAG_UNLIT             (1u << 3)
#define MATERIAL_FLAG_HAS_NORMAL_MAP    (1u << 4)
#define MATERIAL_FLAG_HAS_EMISSIVE      (1u << 5)
#define MATERIAL_FLAG_HAS_OCCLUSION     (1u << 6)
#define MATERIAL_FLAG_HAS_HEIGHT_MAP    (1u << 7)
#define MATERIAL_FLAG_CLEAR_COAT        (1u << 8)
#define MATERIAL_FLAG_TRANSMISSION      (1u << 9)
#define MATERIAL_FLAG_ANISOTROPIC       (1u << 10)
#define MATERIAL_FLAG_SHEEN             (1u << 11)

// ============================================================================
// Post-Processing Constants (Updated Per Post-Process Pass)
// ============================================================================

cbuffer PostProcessConstants : register(REGISTER_POST_PROCESS_CONSTANTS)
{
    // Tone mapping parameters
    float exposure; // Exposure adjustment
    float whitePoint; // White point for tone mapping
    float bloomIntensity; // Bloom effect intensity
    float bloomThreshold; // Bloom threshold
    
    // Color grading
    float gamma; // Gamma correction
    float contrast; // Contrast adjustment
    float saturation; // Saturation adjustment
    float vibrance; // Vibrance adjustment
    
    float brightness; // Brightness adjustment
    float hueShift; // Hue shift adjustment
    float temperature; // Color temperature adjustment
    float tint; // Color tint adjustment
    
    // Vignette
    float vignetteIntensity; // Vignette intensity
    float vignetteSmoothness; // Vignette edge smoothness
    float vignetteRoundness; // Vignette shape roundness
    float _postProcess_pad0; // Padding for alignment
    
    // Film grain
    float filmGrainIntensity; // Film grain intensity
    float filmGrainSize; // Film grain size
    float2 filmGrainOffset; // Film grain animation offset
    
    // Screen-space effects
    float ssaoIntensity; // SSAO intensity
    float ssaoRadius; // SSAO radius
    float ssaoBias; // SSAO bias
    float ssrIntensity; // SSR intensity
    
    // Motion blur
    float motionBlurIntensity; // Motion blur intensity
    uint motionBlurSamples; // Motion blur sample count
    float2 _postProcess_pad1; // Padding for alignment
};

// ============================================================================
// Utility Functions for Constant Access
// ============================================================================

// Helper functions to check flags
bool IsRenderingFlagSet(uint flag)
{
    return (renderingFlags & flag) != 0;
}

bool IsObjectFlagSet(uint flag)
{
    return (objectRenderingFlags & flag) != 0;
}

bool IsMaterialFlagSet(uint flag)
{
    return (materialFlags & flag) != 0;
}

// Time-based animation helpers
float GetAnimatedValue(float frequency, float phase = 0.0f, float amplitude = 1.0f, float offset = 0.0f)
{
    return offset + amplitude * sin(totalTime * frequency + phase);
}

float GetPulsatingValue(float frequency, float phase = 0.0f, float amplitude = 0.5f, float offset = 0.5f)
{
    return offset + amplitude * (1.0f + sin(totalTime * frequency + phase));
}

// Screen space to world space conversion
float3 ScreenToWorldPosition(float2 screenPos, float depth)
{
    float4 clipPos = float4(screenPos * 2.0f - 1.0f, depth, 1.0f);
    clipPos.y = -clipPos.y; // Flip Y coordinate
    float4 worldPos = mul(clipPos, invViewProjectionMatrix);
    return worldPos.xyz / worldPos.w;
}

// World space to screen space conversion
float2 WorldToScreenPosition(float3 worldPos)
{
    float4 clipPos = mul(float4(worldPos, 1.0f), viewProjectionMatrix);
    float2 screenPos = clipPos.xy / clipPos.w;
    screenPos.y = -screenPos.y; // Flip Y coordinate
    return screenPos * 0.5f + 0.5f;
}

// Calculate world space view direction
float3 GetWorldSpaceViewDirection(float3 worldPos)
{
    return normalize(worldPos - cameraPosition);
}

// Calculate linear depth from device depth
float LinearizeDepth(float deviceDepth)
{
    return projectionParams.x / (projectionParams.y - deviceDepth);
}

// Calculate device depth from linear depth
float DeviceDepth(float linearDepth)
{
    return projectionParams.y - projectionParams.x / linearDepth;
}

#endif // AKHANDA_CONSTANTS_HLSLI