// Shaders/BasicPixel.hlsl
// Akhanda Game Engine - Basic Pixel Shader with PBR Lighting
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#include "Common.hlsli"
#include "Constants.hlsli"

// ============================================================================
// Texture and Sampler Declarations
// ============================================================================

// Standard texture slots (t0-t31 reserved for material textures)
Texture2D<float4> albedoTexture : register(t0);
Texture2D<float4> normalTexture : register(t1);
Texture2D<float4> metallicRoughnessTexture : register(t2);
Texture2D<float3> emissiveTexture : register(t3);
Texture2D<float> occlusionTexture : register(t4);
Texture2D<float> heightTexture : register(t5);
Texture2D<float4> detailAlbedoTexture : register(t6);
Texture2D<float4> detailNormalTexture : register(t7);

// Environment and lighting textures (t32-t63)
TextureCube<float4> environmentMap : register(t32);
TextureCube<float4> irradianceMap : register(t33);
TextureCube<float4> prefilterMap : register(t34);
Texture2D<float2> brdfLut : register(t35);

// Shadow maps (t64-t95)
Texture2DArray<float> shadowMaps : register(t64);
TextureCubeArray<float> shadowCubeMaps : register(t65);

// Samplers
SamplerState defaultSampler : register(s0);
SamplerState linearSampler : register(s1);
SamplerState anisotropicSampler : register(s2);
SamplerComparisonState shadowSampler : register(s3);

// ============================================================================
// PBR Lighting Functions
// ============================================================================

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;
    
    return num / denom;
}

// Geometry Function (Smith's method)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    
    float num = NdotV;
    float denom = NdotV * (1.0f - k) + k;
    
    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel-Schlick approximation
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

// Fresnel-Schlick with roughness for IBL
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) *
           pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

// ============================================================================
// Shadow Mapping Functions
// ============================================================================

// PCF (Percentage Closer Filtering) for directional lights
float SampleDirectionalShadow(float3 shadowCoord, uint shadowMapIndex)
{
    if (shadowCoord.x < 0.0f || shadowCoord.x > 1.0f ||
        shadowCoord.y < 0.0f || shadowCoord.y > 1.0f)
    {
        return 1.0f; // Outside shadow map bounds
    }
    
    float shadow = 0.0f;
    float2 texelSize = 1.0f / float2(2048.0f, 2048.0f); // Assume 2048x2048 shadow maps
    
    // 3x3 PCF sampling
    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            float depth = shadowMaps.Sample(defaultSampler, float3(shadowCoord.xy + offset, shadowMapIndex)).r;
            shadow += (shadowCoord.z <= depth + directionalLights[0].shadowBias) ? 1.0f : 0.0f;
        }
    }
    
    return shadow / 9.0f;
}

// Point light shadow sampling (cube map)
float SamplePointShadow(float3 worldPos, uint lightIndex)
{
    float3 lightToFrag = worldPos - pointLights[lightIndex].position;
    float currentDepth = length(lightToFrag);
    
    // Sample cube map
    float closestDepth = shadowCubeMaps.Sample(defaultSampler,
        float4(normalize(lightToFrag), pointLights[lightIndex].shadowMapIndex)).r;
    closestDepth *= pointLights[lightIndex].range; // Convert back to world space
    
    float bias = 0.05f; // Adjustable bias
    return currentDepth - bias > closestDepth ? 0.0f : 1.0f;
}

// ============================================================================
// Lighting Calculation Functions
// ============================================================================

// Calculate directional light contribution
float3 CalculateDirectionalLight(DirectionalLight light, float3 normal, float3 viewDir,
                                float3 albedo, float metallic, float roughness, float3 F0,
                                float3 worldPos, float shadowFactor)
{
    float3 lightDir = normalize(-light.direction);
    float3 halfwayDir = normalize(lightDir + viewDir);
    
    // Calculate radiance
    float3 radiance = light.color * light.intensity;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, halfwayDir, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);
    float3 F = FresnelSchlick(max(dot(halfwayDir, viewDir), 0.0f), F0);
    
    float3 kS = F;
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    kD *= 1.0f - metallic; // Metallic surfaces don't have diffuse
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0f * max(dot(normal, viewDir), 0.0f) * max(dot(normal, lightDir), 0.0f) + 0.0001f;
    float3 specular = numerator / denominator;
    
    float NdotL = max(dot(normal, lightDir), 0.0f);
    
    return (kD * albedo / PI + specular) * radiance * NdotL * shadowFactor;
}

// Calculate point light contribution
float3 CalculatePointLight(PointLight light, float3 normal, float3 viewDir,
                          float3 albedo, float metallic, float roughness, float3 F0,
                          float3 worldPos)
{
    float3 lightDir = normalize(light.position - worldPos);
    float3 halfwayDir = normalize(lightDir + viewDir);
    
    // Calculate attenuation
    float distance = length(light.position - worldPos);
    float attenuation = 1.0f / (light.attenuation.x + light.attenuation.y * distance +
                               light.attenuation.z * (distance * distance));
    
    // Calculate radiance
    float3 radiance = light.color * light.intensity * attenuation;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, halfwayDir, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);
    float3 F = FresnelSchlick(max(dot(halfwayDir, viewDir), 0.0f), F0);
    
    float3 kS = F;
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    kD *= 1.0f - metallic;
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0f * max(dot(normal, viewDir), 0.0f) * max(dot(normal, lightDir), 0.0f) + 0.0001f;
    float3 specular = numerator / denominator;
    
    float NdotL = max(dot(normal, lightDir), 0.0f);
    
    // Apply shadow if the light casts shadows
    float shadowFactor = 1.0f;
    if (light.castsShadows)
    {
        shadowFactor = SamplePointShadow(worldPos, 0); // Assuming first point light
    }
    
    return (kD * albedo / PI + specular) * radiance * NdotL * shadowFactor;
}

// Calculate spot light contribution
float3 CalculateSpotLight(SpotLight light, float3 normal, float3 viewDir,
                         float3 albedo, float metallic, float roughness, float3 F0,
                         float3 worldPos)
{
    float3 lightDir = normalize(light.position - worldPos);
    float3 halfwayDir = normalize(lightDir + viewDir);
    
    // Calculate attenuation
    float distance = length(light.position - worldPos);
    float attenuation = 1.0f / (light.attenuation.x + light.attenuation.y * distance +
                               light.attenuation.z * (distance * distance));
    
    // Calculate spot light cone attenuation
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.innerConeAngle - light.outerConeAngle;
    float intensity = clamp((theta - light.outerConeAngle) / epsilon, 0.0f, 1.0f);
    
    // Calculate radiance
    float3 radiance = light.color * light.intensity * attenuation * intensity;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, halfwayDir, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);
    float3 F = FresnelSchlick(max(dot(halfwayDir, viewDir), 0.0f), F0);
    
    float3 kS = F;
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    kD *= 1.0f - metallic;
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0f * max(dot(normal, viewDir), 0.0f) * max(dot(normal, lightDir), 0.0f) + 0.0001f;
    float3 specular = numerator / denominator;
    
    float NdotL = max(dot(normal, lightDir), 0.0f);
    
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// ============================================================================
// Image-Based Lighting (IBL)
// ============================================================================

float3 CalculateIBL(float3 normal, float3 viewDir, float3 albedo, float metallic, float roughness, float3 F0)
{
    float3 R = reflect(-viewDir, normal);
    float NdotV = max(dot(normal, viewDir), 0.0f);
    
    // Sample irradiance map for diffuse
    float3 irradiance = irradianceMap.Sample(linearSampler, normal).rgb;
    float3 diffuse = irradiance * albedo;
    
    // Sample prefiltered environment map for specular
    float maxReflectionLod = 4.0f; // Assuming 5 mip levels (0-4)
    float3 prefilteredColor = prefilterMap.SampleLevel(linearSampler, R, roughness * maxReflectionLod).rgb;
    
    // Sample BRDF integration map
    float2 brdf = brdfLut.Sample(linearSampler, float2(NdotV, roughness)).rg;
    
    float3 F = FresnelSchlickRoughness(NdotV, F0, roughness);
    
    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - metallic;
    
    float3 specular = prefilteredColor * (F * brdf.x + brdf.y);
    
    return kD * diffuse + specular;
}

// ============================================================================
// Main Pixel Shader Entry Point
// ============================================================================

float4 main(VertexOutput input) : SV_Target
{
    // ========================================================================
    // Debug Visualization Modes
    // ========================================================================
    
#ifdef AKHANDA_DEBUG_MODE
        // Early out for debug visualization modes
        if (debugVisualizationMode == 1) { // Normal visualization
            return float4(NormalToColor(normalize(input.normal)), 1.0f);
        }
        else if (debugVisualizationMode == 2) { // Tangent visualization
            return float4(NormalToColor(normalize(input.tangent)), 1.0f);
        }
        else if (debugVisualizationMode == 3) { // UV visualization
            return float4(frac(input.texCoord), 0.0f, 1.0f);
        }
        else if (debugVisualizationMode == 4) { // World position visualization
            return float4(frac(input.worldPos * 0.1f), 1.0f);
        }
        else if (debugVisualizationMode == 5) { // Depth visualization
            float linearDepth = LinearizeDepth(input.position.z);
            float normalizedDepth = linearDepth / farPlane;
            return float4(normalizedDepth.xxx, 1.0f);
        }
#endif
    
    // ========================================================================
    // Texture Sampling and Material Property Extraction
    // ========================================================================
    
    // Sample albedo texture
    float4 albedoSample = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (IsMaterialFlagSet(MATERIAL_FLAG_HAS_ALBEDO_TEXTURE))
    {
        albedoSample = albedoTexture.Sample(anisotropicSampler, input.texCoord);
    }
    
    // Combine with material color and vertex color
    float4 baseColor = albedoColor * albedoSample * input.color;
    
    // Alpha testing
    if (IsMaterialFlagSet(MATERIAL_FLAG_ALPHA_TEST))
    {
        if (baseColor.a < alphaTestThreshold)
        {
            discard;
        }
    }
    
    // Sample normal map
    float3 normalSample = float3(0.0f, 0.0f, 1.0f);
    if (IsMaterialFlagSet(MATERIAL_FLAG_HAS_NORMAL_MAP))
    {
        normalSample = normalTexture.Sample(anisotropicSampler, input.texCoord).rgb;
        normalSample = normalSample * 2.0f - 1.0f; // Convert from [0,1] to [-1,1]
        normalSample.xy *= normalIntensity; // Scale normal intensity
    }
    
    // Sample metallic-roughness texture
    float metallicSample = metallic;
    float roughnessSample = roughness;
    if (IsMaterialFlagSet(MATERIAL_FLAG_HAS_METALLIC_ROUGHNESS_TEXTURE))
    {
        float4 metallicRoughnessSample = metallicRoughnessTexture.Sample(anisotropicSampler, input.texCoord);
        metallicSample *= metallicRoughnessSample.b; // Blue channel = metallic
        roughnessSample *= metallicRoughnessSample.g; // Green channel = roughness
    }
    
    // Sample emissive texture
    float3 emissiveSample = emissiveColor.rgb;
    if (IsMaterialFlagSet(MATERIAL_FLAG_HAS_EMISSIVE))
    {
        emissiveSample *= emissiveTexture.Sample(anisotropicSampler, input.texCoord).rgb;
    }
    emissiveSample *= emissiveIntensity;
    
    // Sample ambient occlusion texture
    float occlusionSample = 1.0f;
    if (IsMaterialFlagSet(MATERIAL_FLAG_HAS_OCCLUSION))
    {
        occlusionSample = occlusionTexture.Sample(anisotropicSampler, input.texCoord).r;
        occlusionSample = lerp(1.0f, occlusionSample, occlusionStrength);
    }
    
    // ========================================================================
    // Normal Mapping and Tangent Space Calculation
    // ========================================================================
    
    // Create TBN matrix for normal mapping
    float3x3 TBN = CreateTBNMatrix(
        normalize(input.normal),
        normalize(input.tangent),
        normalize(input.bitangent)
    );
    
    // Transform normal from tangent space to world space
    float3 worldNormal;
    if (IsMaterialFlagSet(MATERIAL_FLAG_HAS_NORMAL_MAP))
    {
        worldNormal = TangentToWorld(normalSample, TBN);
    }
    else
    {
        worldNormal = normalize(input.normal);
    }
    
    // Handle double-sided materials
    if (IsMaterialFlagSet(MATERIAL_FLAG_DOUBLE_SIDED))
    {
        worldNormal = normalize(worldNormal * (input.viewDir.z > 0.0f ? 1.0f : -1.0f));
    }
    
    // ========================================================================
    // PBR Material Setup
    // ========================================================================
    
    float3 albedo = baseColor.rgb;
    float alpha = baseColor.a;
    
    // Clamp material values to valid ranges
    metallicSample = saturate(metallicSample);
    roughnessSample = clamp(roughnessSample, 0.04f, 1.0f); // Minimum roughness to avoid numerical issues
    
    // Calculate F0 (surface reflection at normal incidence)
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallicSample);
    
    // Normalize vectors
    float3 N = normalize(worldNormal);
    float3 V = normalize(input.viewDir);
    
    // ========================================================================
    // Lighting Calculations
    // ========================================================================
    
    float3 Lo = float3(0.0f, 0.0f, 0.0f); // Outgoing radiance
    
    // Calculate directional lights
    for (uint i = 0; i < numDirectionalLights; ++i)
    {
        float shadowFactor = 1.0f;
        
        // Calculate shadow factor for primary directional light
        if (i == 0 && directionalLights[i].castsShadows &&
            input.shadowCoord.w > 0.0f)
        { // Valid shadow coordinate
            shadowFactor = SampleDirectionalShadow(input.shadowCoord.xyz, directionalLights[i].shadowMapIndex);
            shadowFactor = lerp(1.0f, shadowFactor, directionalLights[i].shadowStrength);
        }
        
        Lo += CalculateDirectionalLight(directionalLights[i], N, V, albedo,
                                       metallicSample, roughnessSample, F0,
                                       input.worldPos, shadowFactor);
    }
    
    // Calculate point lights
    for (uint j = 0; j < numPointLights; ++j)
    {
        // Distance culling
        float distance = length(pointLights[j].position - input.worldPos);
        if (distance < pointLights[j].range)
        {
            Lo += CalculatePointLight(pointLights[j], N, V, albedo,
                                     metallicSample, roughnessSample, F0,
                                     input.worldPos);
        }
    }
    
    // Calculate spot lights
    for (uint k = 0; k < numSpotLights; ++k)
    {
        // Distance culling
        float distance = length(spotLights[k].position - input.worldPos);
        if (distance < spotLights[k].range)
        {
            Lo += CalculateSpotLight(spotLights[k], N, V, albedo,
                                    metallicSample, roughnessSample, F0,
                                    input.worldPos);
        }
    }
    
    // ========================================================================
    // Image-Based Lighting (IBL)
    // ========================================================================
    
    float3 ambient = float3(0.0f, 0.0f, 0.0f);
    
    // Use IBL if environment maps are available
    if (environmentMapIndex != 0xFFFFFFFF)
    {
        ambient = CalculateIBL(N, V, albedo, metallicSample, roughnessSample, F0);
    }
    else
    {
        // Fallback to simple ambient lighting
        ambient = ambientColor * ambientIntensity * albedo;
    }
    
    // Apply ambient occlusion to ambient lighting
    ambient *= occlusionSample;
    
    // ========================================================================
    // Final Color Composition
    // ========================================================================
    
    float3 color = ambient + Lo + emissiveSample;
    
    // ========================================================================
    // Post-Processing Effects
    // ========================================================================
    
    // Apply global exposure
    color *= pow(2.0f, globalExposure);
    
    // HDR tone mapping
    if (IsRenderingFlagSet(RENDERING_FLAG_TONEMAPPING_ENABLED))
    {
        color = ACESToneMapping(color);
    }
    
    // Gamma correction
    if (IsRenderingFlagSet(RENDERING_FLAG_GAMMA_CORRECTION))
    {
        color = LinearToSRGB(color);
    }
    
    // ========================================================================
    // Debug Overlays
    // ========================================================================
    
#ifdef AKHANDA_DEBUG_MODE
        // Add debug information overlays
        if (debugVisualizationMode == 10) { // Metallic visualization
            color = lerp(color, float3(0.0f, 0.0f, 1.0f), metallicSample);
        }
        else if (debugVisualizationMode == 11) { // Roughness visualization
            color = lerp(color, float3(1.0f, 0.0f, 0.0f), roughnessSample);
        }
        else if (debugVisualizationMode == 12) { // AO visualization
            color = lerp(color, float3(1.0f - occlusionSample, 1.0f - occlusionSample, 1.0f - occlusionSample), 0.5f);
        }
#endif
    
    // ========================================================================
    // Final Output
    // ========================================================================
    
    // Ensure color is in valid range
    color = max(color, 0.0f);
    
    // Handle transparent materials
    if (IsMaterialFlagSet(MATERIAL_FLAG_ALPHA_BLEND))
    {
        alpha *= objectTransparency;
    }
    else
    {
        alpha = 1.0f; // Opaque
    }
    
    return float4(color, alpha);
}

// ============================================================================
// Alternative Entry Points for Different Rendering Passes
// ============================================================================

// Simple unlit shader for basic geometry
float4 UnlitMain(VertexOutput input) : SV_Target
{
    float4 albedoSample = albedoTexture.Sample(anisotropicSampler, input.texCoord);
    float4 baseColor = albedoColor * albedoSample * input.color;
    
    // Alpha testing
    if (IsMaterialFlagSet(MATERIAL_FLAG_ALPHA_TEST))
    {
        if (baseColor.a < alphaTestThreshold)
        {
            discard;
        }
    }
    
    return baseColor;
}

// Depth-only pixel shader (for shadow mapping)
void DepthOnlyMain(VertexOutput input)
{
    // Alpha testing for depth-only pass
    if (IsMaterialFlagSet(MATERIAL_FLAG_ALPHA_TEST))
    {
        float4 albedoSample = albedoTexture.Sample(anisotropicSampler, input.texCoord);
        if (albedoSample.a * albedoColor.a < alphaTestThreshold)
        {
            discard;
        }
    }
    // No output needed for depth-only pass
}

// Simple color output for wireframe rendering
float4 WireframeMain(VertexOutput input) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f); // White wireframe
}