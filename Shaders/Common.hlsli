// Shaders/Common.hlsli
// Akhanda Game Engine - Common HLSL Utilities and Structures
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#ifndef AKHANDA_COMMON_HLSLI
#define AKHANDA_COMMON_HLSLI

// ============================================================================
// Engine Configuration and Version
// ============================================================================

#define AKHANDA_SHADER_VERSION_MAJOR 1
#define AKHANDA_SHADER_VERSION_MINOR 0
#define AKHANDA_SHADER_VERSION_PATCH 0

// Shader model requirements
#define AKHANDA_MIN_SHADER_MODEL 60  // Shader Model 6.0+

// Platform definitions
#ifdef __XBOX_ENABLE_WAVE32
#define AKHANDA_WAVE_SIZE 32
#else
#define AKHANDA_WAVE_SIZE 64
#endif

// ============================================================================
// Mathematical Constants
// ============================================================================

static const float PI = 3.14159265359f;
static const float TWO_PI = 6.28318530718f;
static const float HALF_PI = 1.57079632679f;
static const float INV_PI = 0.31830988618f;
static const float INV_TWO_PI = 0.15915494309f;
static const float E = 2.71828182846f;
static const float GOLDEN_RATIO = 1.61803398875f;

// Floating point precision constants
static const float EPSILON = 1e-7f;
static const float SMALL_NUMBER = 1e-8f;
static const float LARGE_NUMBER = 1e8f;

// Common numerical constants
static const float SQRT_2 = 1.41421356237f;
static const float SQRT_3 = 1.73205080757f;
static const float INV_SQRT_2 = 0.70710678119f;
static const float INV_SQRT_3 = 0.57735026919f;

// ============================================================================
// Vertex Input/Output Structures
// ============================================================================

// Standard vertex input structure
struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float4 color : COLOR;
};

// Standard vertex output structure (for graphics pipeline)
struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float4 color : COLOR;
    float4 shadowCoord : SHADOW_COORD; // For shadow mapping
    float3 viewDir : VIEW_DIRECTION; // World space view direction
    float depth : DEPTH; // Linear depth
    uint instanceID : INSTANCE_ID; // For instanced rendering
};

// Minimal vertex output for simple shaders
struct SimpleVertexOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
};

// Fullscreen triangle vertex output
struct FullscreenVertexOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

// ============================================================================
// Color Space and Utility Functions
// ============================================================================

// Accurate gamma correction (sRGB)
float3 LinearToSRGB(float3linear) {
    return pow(abs(linear), 1.0f / 2.2f);
}

float3 SRGBToLinear(float3 srgb)
{
    return pow(abs(srgb), 2.2f);
}

// More accurate sRGB conversion
float3 LinearToSRGBAccurate(float3linear) {
float3 low = 12.92f * linear;
float3 high = 1.055f * pow(abs(linear), 1.0f / 2.4f) - 0.055f;
    return lerp(low, high, step(0.0031308f, linear));
}

float3 SRGBToLinearAccurate(float3 srgb)
{
    float3 low = srgb / 12.92f;
    float3 high = pow(abs((srgb + 0.055f) / 1.055f), 2.4f);
    return lerp(low, high, step(0.04045f, srgb));
}

// HDR tone mapping
float3 ReinhardToneMapping(float3 color)
{
    return color / (1.0f + color);
}

float3 FilmicToneMapping(float3 color)
{
    float3 x = max(0.0f, color - 0.004f);
    return (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);
}

// ACES tone mapping (simplified)
float3 ACESToneMapping(float3 color)
{
    const float A = 2.51f;
    const float B = 0.03f;
    const float C = 2.43f;
    const float D = 0.59f;
    const float E = 0.14f;
    return saturate((color * (A * color + B)) / (color * (C * color + D) + E));
}

// Luminance calculation
float Luminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

// Color temperature conversion (simplified)
float3 ColorTemperatureToRGB(float temperature)
{
    temperature = clamp(temperature, 1000.0f, 40000.0f) / 100.0f;
    
    float3 color;
    
    // Red component
    if (temperature <= 66.0f)
    {
        color.r = 1.0f;
    }
    else
    {
        color.r = saturate((329.698727446f * pow(temperature - 60.0f, -0.1332047592f)) / 255.0f);
    }
    
    // Green component
    if (temperature <= 66.0f)
    {
        color.g = saturate((99.4708025861f * log(temperature) - 161.1195681661f) / 255.0f);
    }
    else
    {
        color.g = saturate((288.1221695283f * pow(temperature - 60.0f, -0.0755148492f)) / 255.0f);
    }
    
    // Blue component
    if (temperature >= 66.0f)
    {
        color.b = 1.0f;
    }
    else if (temperature <= 19.0f)
    {
        color.b = 0.0f;
    }
    else
    {
        color.b = saturate((138.5177312231f * log(temperature - 10.0f) - 305.0447927307f) / 255.0f);
    }
    
    return color;
}

// ============================================================================
// Mathematical Utility Functions
// ============================================================================

// Safe normalize (avoids division by zero)
float3 SafeNormalize(float3 v)
{
    float len = length(v);
    return len > EPSILON ? v / len : float3(0.0f, 0.0f, 1.0f);
}

float2 SafeNormalize(float2 v)
{
    float len = length(v);
    return len > EPSILON ? v / len : float2(1.0f, 0.0f);
}

// Remap function
float Remap(float value, float inputMin, float inputMax, float outputMin, float outputMax)
{
    return outputMin + (outputMax - outputMin) * saturate((value - inputMin) / (inputMax - inputMin));
}

float2 Remap(float2 value, float2 inputMin, float2 inputMax, float2 outputMin, float2 outputMax)
{
    return outputMin + (outputMax - outputMin) * saturate((value - inputMin) / (inputMax - inputMin));
}

float3 Remap(float3 value, float3 inputMin, float3 inputMax, float3 outputMin, float3 outputMax)
{
    return outputMin + (outputMax - outputMin) * saturate((value - inputMin) / (inputMax - inputMin));
}

// Smooth step variations
float SmoothStep01(float x)
{
    return x * x * (3.0f - 2.0f * x);
}

float SmootherStep01(float x)
{
    return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}

// Bias and gain functions
float Bias(float value, float bias)
{
    return pow(abs(value), log(bias) / log(0.5f));
}

float Gain(float value, float gain)
{
    return value < 0.5f ?
        Bias(2.0f * value, 1.0f - gain) * 0.5f :
        1.0f - Bias(2.0f - 2.0f * value, 1.0f - gain) * 0.5f;
}

// Spherical coordinates conversion
float3 SphericalToCartesian(float2 spherical)
{
    float sinPhi = sin(spherical.y);
    return float3(
        sinPhi * cos(spherical.x),
        cos(spherical.y),
        sinPhi * sin(spherical.x)
    );
}

float2 CartesianToSpherical(float3 cartesian)
{
    float theta = atan2(cartesian.z, cartesian.x);
    float phi = acos(cartesian.y);
    return float2(theta, phi);
}

// ============================================================================
// Matrix and Transform Utilities
// ============================================================================

// Extract scale from matrix
float3 ExtractScale(float4x4matrix) {
    return float3(
        length(matrix._11_12_13),
        length(matrix._21_22_23),
        length(matrix._31_32_33)
    );
}

// Create TBN matrix from vertex data
float3x3 CreateTBNMatrix(float3 normal, float3 tangent, float3 bitangent)
{
    return transpose(float3x3(
        SafeNormalize(tangent),
        SafeNormalize(bitangent),
        SafeNormalize(normal)
    ));
}

// Transform normal from tangent space to world space
float3 TangentToWorld(float3 tangentNormal, float3x3 tbnMatrix)
{
    return normalize(mul(tangentNormal, tbnMatrix));
}

// Transform direction from world space to tangent space
float3 WorldToTangent(float3 worldDirection, float3x3 tbnMatrix)
{
    return normalize(mul(tbnMatrix, worldDirection));
}

// ============================================================================
// Sampling and Noise Utilities
// ============================================================================

// Hash functions for procedural generation
float Hash(float2 p)
{
    return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453123f);
}

float Hash(float3 p)
{
    return frac(sin(dot(p, float3(127.1f, 311.7f, 74.7f))) * 43758.5453123f);
}

float2 Hash22(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.xx + p3.yz) * p3.zy);
}

// Simple 2D noise
float Noise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    
    float a = Hash(i);
    float b = Hash(i + float2(1.0f, 0.0f));
    float c = Hash(i + float2(0.0f, 1.0f));
    float d = Hash(i + float2(1.0f, 1.0f));
    
    float2 u = f * f * (3.0f - 2.0f * f);
    
    return lerp(a, b, u.x) + (c - a) * u.y * (1.0f - u.x) + (d - b) * u.x * u.y;
}

// Fractal noise (FBM)
float FractalNoise(float2 p, int octaves = 4)
{
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    
    for (int i = 0; i < octaves; i++)
    {
        value += amplitude * Noise(p * frequency);
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    
    return value;
}

// ============================================================================
// Geometric Utilities
// ============================================================================

// Distance from point to plane
float DistanceToPlane(float3point, float4 plane)
{
    return dot(plane.xyz, point) + plane.w;
}

// Project point onto plane
float3 ProjectOnPlane(float3point, float4 plane)
{
    float distance = DistanceToPlane(point,
    plane);
    return point
    -distance * plane.xyz;
}

// Reflect vector across plane
float3 ReflectAcrossPlane(float3vector, float3 planeNormal)
{
    return 
    vector
    -2.0f * dot(
    vector, planeNormal) * planeNormal;
}

// Calculate barycentric coordinates
float3 BarycentricCoordinates(float3 p, float3 a, float3 b, float3 c)
{
    float3 v0 = b - a;
    float3 v1 = c - a;
    float3 v2 = p - a;
    
    float dot00 = dot(v0, v0);
    float dot01 = dot(v0, v1);
    float dot02 = dot(v0, v2);
    float dot11 = dot(v1, v1);
    float dot12 = dot(v1, v2);
    
    float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    
    return float3(1.0f - u - v, u, v);
}

// ============================================================================
// Debug and Visualization Utilities
// ============================================================================

// Convert normal to color for visualization
float3 NormalToColor(float3 normal)
{
    return normal * 0.5f + 0.5f;
}

// Debug color based on value range
float3 DebugColorRamp(float value)
{
    value = saturate(value);
    
    if (value < 0.25f)
    {
        return lerp(float3(0, 0, 1), float3(0, 1, 1), value * 4.0f); // Blue to Cyan
    }
    else if (value < 0.5f)
    {
        return lerp(float3(0, 1, 1), float3(0, 1, 0), (value - 0.25f) * 4.0f); // Cyan to Green
    }
    else if (value < 0.75f)
    {
        return lerp(float3(0, 1, 0), float3(1, 1, 0), (value - 0.5f) * 4.0f); // Green to Yellow
    }
    else
    {
        return lerp(float3(1, 1, 0), float3(1, 0, 0), (value - 0.75f) * 4.0f); // Yellow to Red
    }
}

// Heat map visualization
float3 HeatMap(float value)
{
    value = saturate(value);
    return float3(
        smoothstep(0.5f, 1.0f, value),
        smoothstep(0.0f, 0.5f, value) * smoothstep(1.0f, 0.5f, value),
        smoothstep(1.0f, 0.5f, value)
    );
}

// Checkerboard pattern
float Checkerboard(float2 uv, float scale)
{
    float2 grid = floor(uv * scale);
    return fmod(grid.x + grid.y, 2.0f);
}

// ============================================================================
// Utility Macros
// ============================================================================

// Safe division (avoids division by zero)
#define SAFE_DIVIDE(numerator, denominator) ((abs(denominator) > EPSILON) ? (numerator) / (denominator) : 0.0f)

// Squared length (more efficient than length when only comparison is needed)
#define SQUARED_LENGTH(v) dot(v, v)

// Check if value is approximately zero
#define IS_ZERO(value) (abs(value) < EPSILON)

// Check if two values are approximately equal
#define IS_EQUAL(a, b) (abs((a) - (b)) < EPSILON)

// Clamp to [0, 1] range
#define SATURATE(x) saturate(x)

// ============================================================================
// Platform-Specific Utilities
// ============================================================================

// Wave/Warp intrinsics helpers (when available)
#if AKHANDA_MIN_SHADER_MODEL >= 60
#define WAVE_IS_FIRST_LANE() WaveIsFirstLane()
#define WAVE_READ_LANE_FIRST(expr) WaveReadLaneFirst(expr)
#define WAVE_ALL_TRUE(expr) WaveActiveAllTrue(expr)
#define WAVE_ANY_TRUE(expr) WaveActiveAnyTrue(expr)
#define WAVE_ACTIVE_SUM(expr) WaveActiveSum(expr)
#define WAVE_ACTIVE_MIN(expr) WaveActiveMin(expr)
#define WAVE_ACTIVE_MAX(expr) WaveActiveMax(expr)
#else
    // Fallback for older shader models
#define WAVE_IS_FIRST_LANE() true
#define WAVE_READ_LANE_FIRST(expr) (expr)
#define WAVE_ALL_TRUE(expr) (expr)
#define WAVE_ANY_TRUE(expr) (expr)
#define WAVE_ACTIVE_SUM(expr) (expr)
#define WAVE_ACTIVE_MIN(expr) (expr)
#define WAVE_ACTIVE_MAX(expr) (expr)
#endif

// ============================================================================
// Version and Compatibility Checks
// ============================================================================

// Compile-time shader model validation
#if SHADER_MODEL < AKHANDA_MIN_SHADER_MODEL
    #error "Akhanda shaders require Shader Model 6.0 or higher"
#endif

// Feature availability checks
#define AKHANDA_HAS_WAVE_INTRINSICS (SHADER_MODEL >= 60)
#define AKHANDA_HAS_16BIT_TYPES (SHADER_MODEL >= 62)
#define AKHANDA_HAS_RAYTRACING (SHADER_MODEL >= 63)
#define AKHANDA_HAS_MESH_SHADERS (SHADER_MODEL >= 65)
#define AKHANDA_HAS_VARIABLE_RATE_SHADING (SHADER_MODEL >= 61)

#endif // AKHANDA_COMMON_HLSLI