// Shaders/BasicVertex.hlsl
// Akhanda Game Engine - Basic Vertex Shader
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#include "Common.hlsli"
#include "Constants.hlsli"

// ============================================================================
// Vertex Shader Entry Point
// ============================================================================

VertexOutput main(VertexInput input, uint instanceID : SV_InstanceID)
{
    VertexOutput output = (VertexOutput) 0;
    
    // ========================================================================
    // Transform Vertex Position
    // ========================================================================
    
    // Transform from object space to world space
    float4 worldPosition = mul(float4(input.position, 1.0f), worldMatrix);
    output.worldPos = worldPosition.xyz;
    
    // Transform from world space to clip space
    output.position = mul(worldPosition, viewProjectionMatrix);
    
    // Store linear depth for various effects
    output.depth = output.position.w;
    
    // ========================================================================
    // Transform Normals and Tangent Space
    // ========================================================================
    
    // Transform normal to world space (using normal matrix to handle non-uniform scaling)
    output.normal = normalize(mul(input.normal, (float3x3) normalMatrix));
    
    // Transform tangent and bitangent to world space
    output.tangent = normalize(mul(input.tangent, (float3x3) worldMatrix));
    output.bitangent = normalize(mul(input.bitangent, (float3x3) worldMatrix));
    
    // Ensure orthogonality of tangent space (Gram-Schmidt orthogonalization)
    output.tangent = normalize(output.tangent - dot(output.tangent, output.normal) * output.normal);
    output.bitangent = normalize(cross(output.normal, output.tangent));
    
    // ========================================================================
    // Texture Coordinates and Vertex Color
    // ========================================================================
    
    // Apply UV transformations from material constants
    float2 transformedUV = input.texCoord;
    
    // Apply UV scaling and offset
    transformedUV = transformedUV * uvScale + uvOffset;
    
    // Apply UV rotation if specified
    if (abs(uvRotation) > EPSILON)
    {
        float cosRot = cos(uvRotation);
        float sinRot = sin(uvRotation);
        float2x2 rotationMatrix = float2x2(cosRot, -sinRot, sinRot, cosRot);
        
        // Rotate around center (0.5, 0.5)
        float2 centeredUV = transformedUV - 0.5f;
        centeredUV = mul(centeredUV, rotationMatrix);
        transformedUV = centeredUV + 0.5f;
    }
    
    output.texCoord = transformedUV;
    
    // Pass through vertex color (can be modulated by material)
    output.color = input.color;
    
    // ========================================================================
    // View Direction Calculation
    // ========================================================================
    
    // Calculate world space view direction (from surface to camera)
    output.viewDir = normalize(cameraPosition - output.worldPos);
    
    // ========================================================================
    // Shadow Mapping Coordinates
    // ========================================================================
    
    // Calculate shadow coordinates for primary directional light
    if (numDirectionalLights > 0 && directionalLights[0].castsShadows)
    {
        // Transform world position to light's clip space
        float4 lightSpacePos = mul(worldPosition, directionalLights[0].lightViewProjection);
        
        // Convert to shadow map texture coordinates
        output.shadowCoord.xy = lightSpacePos.xy * 0.5f + 0.5f;
        output.shadowCoord.y = 1.0f - output.shadowCoord.y; // Flip Y for texture sampling
        output.shadowCoord.z = lightSpacePos.z / lightSpacePos.w; // Store normalized depth
        output.shadowCoord.w = lightSpacePos.w; // Store W for perspective divide validation
    }
    else
    {
        output.shadowCoord = float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    // ========================================================================
    // Instance ID and Additional Data
    // ========================================================================
    
    // Store instance ID for instanced rendering
    output.instanceID = instanceID;
    
    // ========================================================================
    // Motion Vector Calculation (for Temporal Anti-Aliasing and Motion Blur)
    // ========================================================================
    
    // Calculate current frame clip position
    float4 currentClipPos = output.position;
    
    // Calculate previous frame world position (using previous world matrix if available)
    float4 prevWorldPos = mul(float4(input.position, 1.0f), prevWorldMatrix);
    
    // Calculate previous frame clip position
    float4 prevClipPos = mul(prevWorldPos, prevViewProjectionMatrix);
    
    // Convert to NDC (Normalized Device Coordinates)
    float2 currentNDC = currentClipPos.xy / currentClipPos.w;
    float2 prevNDC = prevClipPos.xy / prevClipPos.w;
    
    // Calculate motion vector (current - previous)
    float2 motionVector = currentNDC - prevNDC;
    
    // Store motion vector in unused channels (can be retrieved in pixel shader if needed)
    // For now, we'll pack it into the shadow coordinate's unused channels
    // This is a simple approach - in a full engine, you'd have dedicated motion vector output
    
    // ========================================================================
    // Level-of-Detail (LOD) Calculations
    // ========================================================================
    
    // Calculate distance from camera for LOD calculations
    float distanceToCamera = length(cameraPosition - output.worldPos);
    
    // Apply global and per-object LOD bias
    float lodFactor = distanceToCamera * globalLODScale * (1.0f + globalLODBias + lodBias);
    
    // You can use this lodFactor in the pixel shader for texture LOD selection
    // Store it in an unused channel or create a dedicated output if needed
    
    // ========================================================================
    // Debug and Development Features
    // ========================================================================
    
#ifdef AKHANDA_DEBUG_MODE
        // In debug mode, you can add additional calculations for visualization
        // For example, visualizing normal vectors, tangent space, UV coordinates, etc.
        
        // Debug: Visualize normal as color (can be toggled via debug flags)
        if (debugVisualizationMode == 1) { // Normal visualization
            output.color.rgb = NormalToColor(output.normal);
        }
        else if (debugVisualizationMode == 2) { // Tangent visualization
            output.color.rgb = NormalToColor(output.tangent);
        }
        else if (debugVisualizationMode == 3) { // UV visualization
            output.color.rgb = float3(frac(output.texCoord), 0.0f);
        }
        else if (debugVisualizationMode == 4) { // World position visualization
            output.color.rgb = frac(output.worldPos * 0.1f);
        }
#endif
    
    // ========================================================================
    // Vertex Animation Support (Basic Framework)
    // ========================================================================
    
    // If vertex animation is enabled, apply basic transformations
    if (animationTime > 0.0f)
    {
        // Simple sine wave animation for demonstration
        // In a real engine, this would be replaced with proper skeletal animation
        float waveFreq = 2.0f;
        float waveAmp = 0.1f;
        
        // Apply wave animation to vertex position
        float wave = sin(animationTime * waveFreq + input.position.x * 0.5f) * waveAmp;
        output.worldPos.y += wave;
        
        // Recalculate clip space position
        output.position = mul(float4(output.worldPos, 1.0f), viewProjectionMatrix);
    }
    
    // ========================================================================
    // Wind Animation (Environmental Effects)
    // ========================================================================
    
    // Simple wind effect based on world position and time
    // This could be controlled by material flags or object properties
    if (IsObjectFlagSet(OBJECT_FLAG_AFFECTED_BY_WIND))
    {
        float windStrength = 0.5f;
        float windSpeed = 1.0f;
        
        // Create wind displacement based on world position and time
        float2 windOffset = float2(
            sin(totalTime * windSpeed + output.worldPos.x * 0.1f),
            cos(totalTime * windSpeed * 0.7f + output.worldPos.z * 0.1f)
        ) * windStrength;
        
        // Apply wind displacement (primarily affects Y position for vegetation)
        output.worldPos.xz += windOffset * input.position.y; // Scale by height
        
        // Recalculate clip space position
        output.position = mul(float4(output.worldPos, 1.0f), viewProjectionMatrix);
        
        // Adjust normal slightly for wind-bent surfaces
        float3 windNormalOffset = float3(windOffset.x, 0.0f, windOffset.y) * 0.1f;
        output.normal = normalize(output.normal + windNormalOffset);
    }
    
    // ========================================================================
    // Screen-Space Derivatives Preparation
    // ========================================================================
    
    // Prepare data that might be needed for screen-space derivatives in pixel shader
    // This helps with texture filtering and normal map calculations
    
    // Calculate screen space position for derivative calculations
    float2 screenPos = output.position.xy / output.position.w;
    
    // Store screen position in world position w component (if not needed elsewhere)
    // This is a hack for passing additional data - proper implementation would use
    // dedicated outputs or a more sophisticated packing scheme
    
    // ========================================================================
    // Clipping and Culling Support
    // ========================================================================
    
    // Support for user-defined clipping planes (if needed)
    // For now, we don't implement custom clipping, but the framework is here
    
    // Calculate distance to custom clipping plane (if enabled)
    // This would be used for effects like portals, water clipping, etc.
    
    // ========================================================================
    // Final Output Validation and Cleanup
    // ========================================================================
    
    // Ensure all vectors are properly normalized
    output.normal = SafeNormalize(output.normal);
    output.tangent = SafeNormalize(output.tangent);
    output.bitangent = SafeNormalize(output.bitangent);
    output.viewDir = SafeNormalize(output.viewDir);
    
    // Clamp values that might cause issues downstream
    output.texCoord = clamp(output.texCoord, -10.0f, 10.0f); // Prevent extreme UV values
    output.color = saturate(output.color); // Ensure color is in valid range
    
    // Ensure depth is valid
    output.depth = max(output.depth, nearPlane);
    
    return output;
}

// ============================================================================
// Alternative Entry Points for Different Use Cases
// ============================================================================

// Simple vertex shader for basic geometry (no lighting calculations)
SimpleVertexOutput SimpleMain(VertexInput input)
{
    SimpleVertexOutput output = (SimpleVertexOutput) 0;
    
    // Basic transform
    float4 worldPos = mul(float4(input.position, 1.0f), worldMatrix);
    output.position = mul(worldPos, viewProjectionMatrix);
    
    // Simple UV and color pass-through
    output.texCoord = input.texCoord * uvScale + uvOffset;
    output.color = input.color;
    
    return output;
}

// Fullscreen triangle vertex shader (for post-processing)
FullscreenVertexOutput FullscreenMain(uint vertexID : SV_VertexID)
{
    FullscreenVertexOutput output = (FullscreenVertexOutput) 0;
    
    // Generate fullscreen triangle
    output.texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.texCoord * 2.0f - 1.0f, 0.0f, 1.0f);
    output.position.y = -output.position.y; // Flip Y coordinate
    
    return output;
}

// Vertex shader for shadow map generation (optimized for depth-only rendering)
float4 ShadowMain(VertexInput input) : SV_POSITION
{
    // Minimal transformation for shadow mapping
    float4 worldPos = mul(float4(input.position, 1.0f), worldMatrix);
    return mul(worldPos, directionalLights[0].lightViewProjection);
}

// Vertex shader for depth-only rendering (Z-prepass)
float4 DepthOnlyMain(VertexInput input) : SV_POSITION
{
    // Minimal transformation for depth-only pass
    float4 worldPos = mul(float4(input.position, 1.0f), worldMatrix);
    return mul(worldPos, viewProjectionMatrix);
}