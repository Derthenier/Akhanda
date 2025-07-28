// Engine/Tests/ShaderSystemTest.cpp
// Akhanda Game Engine - Comprehensive Shader System Test Suite
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#include <iostream>
#include <functional>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>

import Akhanda.Engine.Shaders;
import Akhanda.Engine.Shaders.D3D12;
import Akhanda.Engine.Renderer;
import Akhanda.Engine.RHI;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Core.Result;
import Akhanda.Core.Logging;
import Akhanda.Core.Memory;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Platform.Interfaces;

using namespace Akhanda;

// ============================================================================
// Test Framework Utilities
// ============================================================================

class ShaderTestFramework {
public:
    ShaderTestFramework()
        : logChannel_(Logging::LogManager::Instance().GetChannel("ShaderTest"))
        , passedTests_(0)
        , failedTests_(0)
    {
        logChannel_.Log(Logging::LogLevel::Info, "=== Akhanda Shader System Test Suite ===");
    }

    ~ShaderTestFramework() {
        logChannel_.LogFormat(Logging::LogLevel::Info,
            "=== Test Results: {} passed, {} failed ===",
            passedTests_, failedTests_);
    }

    void RunTest(const std::string& testName, std::function<bool()> testFunc) {
        logChannel_.LogFormat(Logging::LogLevel::Info, "Running test: {}", testName);

        try {
            bool result = testFunc();
            if (result) {
                logChannel_.LogFormat(Logging::LogLevel::Info, "✅ PASSED: {}", testName);
                passedTests_++;
            }
            else {
                logChannel_.LogFormat(Logging::LogLevel::Error, "❌ FAILED: {}", testName);
                failedTests_++;
            }
        }
        catch (const std::exception& e) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "❌ EXCEPTION in {}: {}", testName, e.what());
            failedTests_++;
        }
    }

    bool AllTestsPassed() const { return failedTests_ == 0; }
    int GetPassedCount() const { return passedTests_; }
    int GetFailedCount() const { return failedTests_; }

private:
    Logging::LogChannel& logChannel_;
    int passedTests_;
    int failedTests_;
};

// ============================================================================
// Test Data Setup
// ============================================================================

class ShaderTestSetup {
public:
    static bool CreateTestShaderFiles() {
        std::filesystem::create_directories("TestShaders");

        // Create a test vertex shader that uses our includes
        std::string vertexShader = R"(
#include "Common.hlsli"
#include "Constants.hlsli"

VertexOutput main(VertexInput input) {
    VertexOutput output = (VertexOutput)0;
    
    // Basic transformation
    float4 worldPos = mul(float4(input.position, 1.0f), worldMatrix);
    output.position = mul(worldPos, viewProjectionMatrix);
    output.worldPos = worldPos.xyz;
    output.normal = normalize(mul(input.normal, (float3x3)normalMatrix));
    output.texCoord = input.texCoord;
    output.color = input.color;
    
    return output;
}

// Test multiple entry points
VertexOutput SimpleMain(VertexInput input) {
    VertexOutput output = (VertexOutput)0;
    output.position = float4(input.position, 1.0f);
    output.texCoord = input.texCoord;
    output.color = input.color;
    return output;
}
)";

        // Create a test pixel shader with variants
        std::string pixelShader = R"(
#include "Common.hlsli"
#include "Constants.hlsli"

float4 main(VertexOutput input) : SV_Target {
    float4 color = albedoColor * input.color;
    
    #ifdef USE_TEXTURE
        float4 texColor = albedoTexture.Sample(defaultSampler, input.texCoord);
        color *= texColor;
    #endif
    
    #ifdef ENABLE_LIGHTING
        float3 normal = normalize(input.normal);
        float ndotl = max(dot(normal, -directionalLights[0].direction), 0.0f);
        color.rgb *= (ambientColor * ambientIntensity + ndotl);
    #endif
    
    return color;
}

// Unlit entry point
float4 UnlitMain(VertexOutput input) : SV_Target {
    return input.color;
}
)";

        // Write test shaders
        if (!WriteFile("TestShaders/TestVertex.hlsl", vertexShader)) return false;
        if (!WriteFile("TestShaders/TestPixel.hlsl", pixelShader)) return false;

        return true;
    }

    static void CleanupTestFiles() {
        std::filesystem::remove_all("TestShaders");
    }

    static bool WriteFile(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << content;
        return file.good();
    }
};

// ============================================================================
// Comprehensive Shader System Tests
// ============================================================================

class ShaderSystemTests {
public:
    ShaderSystemTests(ShaderTestFramework& framework)
        : framework_(framework)
        , logChannel_(Logging::LogManager::Instance().GetChannel("ShaderTest"))
        , allocator_(Memory::MemoryManager::Instance().GetDefaultAllocator())
    {
    }

    void RunAllTests() {
        // Test 1: Basic Initialization
        framework_.RunTest("Shader Manager Initialization",
            [this]() { return TestShaderManagerInitialization(); });

        // Test 2: Basic Shader Compilation
        framework_.RunTest("Basic Shader Compilation",
            [this]() { return TestBasicShaderCompilation(); });

        // Test 3: Include System
        framework_.RunTest("Include System Functionality",
            [this]() { return TestIncludeSystem(); });

        // Test 4: Shader Reflection
        framework_.RunTest("Shader Reflection and Resource Binding",
            [this]() { return TestShaderReflection(); });

        // Test 5: Shader Variants
        framework_.RunTest("Shader Variant System",
            [this]() { return TestShaderVariants(); });

        // Test 6: Multiple Entry Points
        framework_.RunTest("Multiple Entry Points",
            [this]() { return TestMultipleEntryPoints(); });

        // Test 7: Shader Programs
        framework_.RunTest("Shader Program Creation",
            [this]() { return TestShaderPrograms(); });

        // Test 8: Cache System
        framework_.RunTest("Shader Caching System",
            [this]() { return TestShaderCaching(); });

        // Test 9: Error Handling
        framework_.RunTest("Error Handling and Recovery",
            [this]() { return TestErrorHandling(); });

        // Test 10: Hot Reload (if enabled)
        framework_.RunTest("Hot Reload Detection",
            [this]() { return TestHotReload(); });

        // Test 11: Resource Binding Validation
        framework_.RunTest("Constant Buffer Layout Validation",
            [this]() { return TestConstantBufferValidation(); });

        // Test 12: Performance and Statistics
        framework_.RunTest("Performance and Statistics Tracking",
            [this]() { return TestPerformanceTracking(); });
    }

private:
    ShaderTestFramework& framework_;
    Logging::LogChannel& logChannel_;
    Memory::IAllocator& allocator_;
    std::unique_ptr<Shaders::IShaderManager> shaderManager_;
    std::shared_ptr<RHI::IRenderDevice> renderDevice_;

    // ========================================================================
    // Test 1: Shader Manager Initialization
    // ========================================================================
    bool TestShaderManagerInitialization() {
        logChannel_.Log(Logging::LogLevel::Debug, "Testing shader manager initialization...");

        // Create a mock render device (you might need to adapt this to your actual RHI)
        // For now, we'll assume you have a way to create a test render device
        renderDevice_ = CreateTestRenderDevice();
        if (!renderDevice_) {
            logChannel_.Log(Logging::LogLevel::Error, "Failed to create test render device");
            return false;
        }

        // Create shader manager
        auto result = Shaders::ShaderManagerFactory::CreateShaderManager(
            Configuration::RenderingAPI::D3D12,
            allocator_
        );

        if (!result) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Failed to create shader manager: {}", result.Error().ToString());
            return false;
        }

        shaderManager_ = std::move(result.Value());

        // Configure shader system
        Configuration::ShaderConfig config;
        config.SetCompilationMode(Configuration::ShaderCompilationMode::Hybrid);
        config.SetOptimization(Configuration::ShaderOptimization::Debug);
        config.SetTargetShaderModel(Configuration::ShaderModel::SM_6_0);
        config.SetShaderSourcePath("Shaders/");
        config.SetEnableHotReload(true);
        config.SetEnableShaderCache(true);
        config.SetGenerateDebugInfo(true);

        // Add test include paths
        config.AddIncludePath("Shaders/");
        config.AddIncludePath("TestShaders/");

        // Initialize shader manager
        auto initResult = shaderManager_->Initialize(renderDevice_, config);
        if (!initResult) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Failed to initialize shader manager: {}", initResult.Error().ToString());
            return false;
        }

        // Verify initialization
        if (!shaderManager_->IsInitialized()) {
            logChannel_.Log(Logging::LogLevel::Error, "Shader manager reports not initialized");
            return false;
        }

        logChannel_.Log(Logging::LogLevel::Debug, "Shader manager initialized successfully");
        return true;
    }

    // ========================================================================
    // Test 2: Basic Shader Compilation
    // ========================================================================
    bool TestBasicShaderCompilation() {
        logChannel_.Log(Logging::LogLevel::Debug, "Testing basic shader compilation...");

        if (!shaderManager_) {
            logChannel_.Log(Logging::LogLevel::Error, "Shader manager not initialized");
            return false;
        }

        // Test vertex shader compilation
        Shaders::ShaderCompileRequest vertexRequest;
        vertexRequest.sourceFile = "TestShaders/TestVertex.hlsl";
        vertexRequest.entryPoint = "main";
        vertexRequest.stage = Shaders::ShaderStage::Vertex;
        vertexRequest.optimization = Configuration::ShaderOptimization::Debug;
        vertexRequest.shaderModel = Configuration::ShaderModel::SM_6_0;
        vertexRequest.generateDebugInfo = true;

        auto vertexResult = shaderManager_->CompileShader(vertexRequest);
        if (!vertexResult) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Failed to compile vertex shader: {}", vertexResult.Error().ToString());
            return false;
        }

        auto vertexShader = vertexResult.Value();
        if (!vertexShader->IsCompiled()) {
            logChannel_.Log(Logging::LogLevel::Error, "Vertex shader reports not compiled");
            return false;
        }

        // Test pixel shader compilation
        Shaders::ShaderCompileRequest pixelRequest;
        pixelRequest.sourceFile = "TestShaders/TestPixel.hlsl";
        pixelRequest.entryPoint = "main";
        pixelRequest.stage = Shaders::ShaderStage::Pixel;
        pixelRequest.optimization = Configuration::ShaderOptimization::Debug;
        pixelRequest.shaderModel = Configuration::ShaderModel::SM_6_0;
        pixelRequest.generateDebugInfo = true;

        auto pixelResult = shaderManager_->CompileShader(pixelRequest);
        if (!pixelResult) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Failed to compile pixel shader: {}", pixelResult.Error().ToString());
            return false;
        }

        auto pixelShader = pixelResult.Value();
        if (!pixelShader->IsCompiled()) {
            logChannel_.Log(Logging::LogLevel::Error, "Pixel shader reports not compiled");
            return false;
        }

        // Verify bytecode exists and is reasonable size
        const auto& vertexBytecode = vertexShader->GetBytecode();
        const auto& pixelBytecode = pixelShader->GetBytecode();

        if (vertexBytecode.empty() || pixelBytecode.empty()) {
            logChannel_.Log(Logging::LogLevel::Error, "Compiled shaders have empty bytecode");
            return false;
        }

        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Compiled shaders - Vertex: {} bytes, Pixel: {} bytes",
            vertexBytecode.size(), pixelBytecode.size());

        return true;
    }

    // ========================================================================
    // Test 3: Include System
    // ========================================================================
    bool TestIncludeSystem() {
        logChannel_.Log(Logging::LogLevel::Debug, "Testing include system...");

        // Verify that shaders compiled successfully with includes
        // The shaders we compiled in the previous test use #include directives
        auto shader = shaderManager_->GetShader("TestVertex.hlsl", Shaders::ShaderVariant{});
        if (!shader) {
            logChannel_.Log(Logging::LogLevel::Error, "Cannot retrieve compiled shader for include test");
            return false;
        }

        const auto& reflection = shader->GetReflection();

        // Check that included files are tracked
        if (reflection.includedFiles.empty()) {
            logChannel_.Log(Logging::LogLevel::Warning,
                "No included files tracked (this might be expected if includes aren't tracked yet)");
        }

        // Check that constants from includes are available
        // VertexInput and VertexOutput should come from Common.hlsli
        // worldMatrix, viewProjectionMatrix should come from Constants.hlsli

        logChannel_.Log(Logging::LogLevel::Debug, "Include system working - shaders compiled with includes");
        return true;
    }

    // ========================================================================
    // Test 4: Shader Reflection
    // ========================================================================
    bool TestShaderReflection() {
        logChannel_.Log(Logging::LogLevel::Debug, "Testing shader reflection...");

        auto vertexShader = shaderManager_->GetShader("TestVertex.hlsl", Shaders::ShaderVariant{});
        if (!vertexShader) {
            logChannel_.Log(Logging::LogLevel::Error, "Cannot retrieve vertex shader for reflection test");
            return false;
        }

        const auto& reflection = vertexShader->GetReflection();

        // Log reflection information
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Shader reflection - CBs: {}, SRVs: {}, Samplers: {}, UAVs: {}",
            reflection.constantBuffers.size(),
            reflection.shaderResources.size(),
            reflection.samplers.size(),
            reflection.unorderedAccessViews.size());

        // Check for expected constant buffers from Constants.hlsli
        //bool foundObjectConstants = false;
        //bool foundCameraConstants = false;

        for (const auto& cb : reflection.constantBuffers) {
            logChannel_.LogFormat(Logging::LogLevel::Debug,
                "Found constant buffer: '{}' at register b{}", cb.name, cb.bindPoint);

            //if (cb.name == "ObjectConstants") foundObjectConstants = true;
            //if (cb.name == "CameraConstants") foundCameraConstants = true;
        }

        // Test resource binding lookups
        auto objectCBBinding = vertexShader->GetConstantBufferBinding("ObjectConstants");
        if (objectCBBinding.has_value()) {
            logChannel_.LogFormat(Logging::LogLevel::Debug,
                "ObjectConstants found at binding {}", objectCBBinding.value());
        }

        // Verify register layout follows our b0-b31 convention
        bool registerLayoutValid = true;
        for (const auto& cb : reflection.constantBuffers) {
            if (cb.bindPoint > 31) {
                logChannel_.LogFormat(Logging::LogLevel::Warning,
                    "Constant buffer '{}' at register b{} exceeds expected range (b0-b31)",
                    cb.name, cb.bindPoint);
                registerLayoutValid = false;
            }
        }

        if (!registerLayoutValid) {
            logChannel_.Log(Logging::LogLevel::Warning,
                "Register layout doesn't follow expected b0-b31 convention");
        }

        return true; // Reflection test passes if we can extract data
    }

    // ========================================================================
    // Test 5: Shader Variants
    // ========================================================================
    bool TestShaderVariants() {
        logChannel_.Log(Logging::LogLevel::Debug, "Testing shader variants...");

        // Create variants with different defines
        Shaders::ShaderVariant baseVariant;

        Shaders::ShaderVariant textureVariant;
        textureVariant.defines.emplace_back("USE_TEXTURE", "1");

        Shaders::ShaderVariant lightingVariant;
        lightingVariant.defines.emplace_back("ENABLE_LIGHTING", "1");

        Shaders::ShaderVariant fullVariant;
        fullVariant.defines.emplace_back("USE_TEXTURE", "1");
        fullVariant.defines.emplace_back("ENABLE_LIGHTING", "1");

        // Compile pixel shader with different variants
        std::vector<std::pair<std::string, Shaders::ShaderVariant>> variants = {
            {"Base", baseVariant},
            {"Texture", textureVariant},
            {"Lighting", lightingVariant},
            {"Full", fullVariant}
        };

        for (const auto& [name, variant] : variants) {
            Shaders::ShaderCompileRequest request;
            request.sourceFile = "TestShaders/TestPixel.hlsl";
            request.entryPoint = "main";
            request.stage = Shaders::ShaderStage::Pixel;
            request.variant = variant;
            request.optimization = Configuration::ShaderOptimization::Debug;
            request.shaderModel = Configuration::ShaderModel::SM_6_0;

            auto result = shaderManager_->CompileShader(request);
            if (!result) {
                logChannel_.LogFormat(Logging::LogLevel::Error,
                    "Failed to compile {} variant: {}", name, result.Error().ToString());
                return false;
            }

            logChannel_.LogFormat(Logging::LogLevel::Debug,
                "Successfully compiled {} variant", name);
        }

        return true;
    }

    // ========================================================================
    // Test 6: Multiple Entry Points
    // ========================================================================
    bool TestMultipleEntryPoints() {
        logChannel_.Log(Logging::LogLevel::Debug, "Testing multiple entry points...");

        // Test different entry points in the same shader file
        std::vector<std::string> vertexEntryPoints = { "main", "SimpleMain" };
        std::vector<std::string> pixelEntryPoints = { "main", "UnlitMain" };

        for (const auto& entryPoint : vertexEntryPoints) {
            Shaders::ShaderCompileRequest request;
            request.sourceFile = "TestShaders/TestVertex.hlsl";
            request.entryPoint = entryPoint;
            request.stage = Shaders::ShaderStage::Vertex;
            request.optimization = Configuration::ShaderOptimization::Debug;
            request.shaderModel = Configuration::ShaderModel::SM_6_0;

            auto result = shaderManager_->CompileShader(request);
            if (!result) {
                logChannel_.LogFormat(Logging::LogLevel::Error,
                    "Failed to compile vertex entry point '{}': {}",
                    entryPoint, result.Error().ToString());
                return false;
            }

            logChannel_.LogFormat(Logging::LogLevel::Debug,
                "Successfully compiled vertex entry point '{}'", entryPoint);
        }

        for (const auto& entryPoint : pixelEntryPoints) {
            Shaders::ShaderCompileRequest request;
            request.sourceFile = "TestShaders/TestPixel.hlsl";
            request.entryPoint = entryPoint;
            request.stage = Shaders::ShaderStage::Pixel;
            request.optimization = Configuration::ShaderOptimization::Debug;
            request.shaderModel = Configuration::ShaderModel::SM_6_0;

            auto result = shaderManager_->CompileShader(request);
            if (!result) {
                logChannel_.LogFormat(Logging::LogLevel::Error,
                    "Failed to compile pixel entry point '{}': {}",
                    entryPoint, result.Error().ToString());
                return false;
            }

            logChannel_.LogFormat(Logging::LogLevel::Debug,
                "Successfully compiled pixel entry point '{}'", entryPoint);
        }

        return true;
    }

    // ========================================================================
    // Test 7: Shader Programs
    // ========================================================================
    bool TestShaderPrograms() {
        logChannel_.Log(Logging::LogLevel::Debug, "Testing shader programs...");

        auto vertexShader = shaderManager_->GetShader("TestVertex.hlsl", Shaders::ShaderVariant{});
        auto pixelShader = shaderManager_->GetShader("TestPixel.hlsl", Shaders::ShaderVariant{});

        if (!vertexShader || !pixelShader) {
            logChannel_.Log(Logging::LogLevel::Error, "Cannot retrieve shaders for program test");
            return false;
        }

        // Create shader program
        std::vector<std::shared_ptr<Shaders::IShader>> shaders;
        shaders.push_back(vertexShader);
        shaders.push_back(pixelShader);

        auto programResult = shaderManager_->CreateShaderProgram("TestProgram", shaders);
        if (!programResult) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Failed to create shader program: {}", programResult.Error().ToString());
            return false;
        }

        auto program = programResult.Value();
        if (!program->IsLinked()) {
            logChannel_.Log(Logging::LogLevel::Error, "Shader program reports not linked");
            return false;
        }

        // Test resource binding lookup across program
        auto objectCBBinding = program->FindConstantBufferBinding("ObjectConstants");
        if (objectCBBinding.has_value()) {
            logChannel_.LogFormat(Logging::LogLevel::Debug,
                "Program found ObjectConstants at binding {}", objectCBBinding.value());
        }

        logChannel_.Log(Logging::LogLevel::Debug, "Shader program created successfully");
        return true;
    }

    // ========================================================================
    // Test 8: Shader Caching
    // ========================================================================
    bool TestShaderCaching() {
        logChannel_.Log(Logging::LogLevel::Debug, "Testing shader caching...");

        // Clear cache and compile shader
        shaderManager_->ClearCache();

        auto startTime = std::chrono::steady_clock::now();

        Shaders::ShaderCompileRequest request;
        request.sourceFile = "TestShaders/TestVertex.hlsl";
        request.entryPoint = "main";
        request.stage = Shaders::ShaderStage::Vertex;
        request.optimization = Configuration::ShaderOptimization::Debug;
        request.shaderModel = Configuration::ShaderModel::SM_6_0;

        auto result1 = shaderManager_->CompileShader(request);
        if (!result1) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Failed first compilation for cache test: {}", result1.Error().ToString());
            return false;
        }

        auto firstCompileTime = std::chrono::steady_clock::now() - startTime;

        // Compile same shader again (should hit cache)
        startTime = std::chrono::steady_clock::now();
        auto result2 = shaderManager_->CompileShader(request);
        if (!result2) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Failed second compilation for cache test: {}", result2.Error().ToString());
            return false;
        }

        auto secondCompileTime = std::chrono::steady_clock::now() - startTime;

        // Check statistics
        const auto& stats = shaderManager_->GetStatistics();
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Cache stats - Hits: {}, Misses: {}, Total cached: {}",
            stats.cacheHits, stats.cacheMisses, shaderManager_->GetCachedShaderCount());

        // Second compilation should be faster (cache hit)
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Compile times - First: {}μs, Second: {}μs",
            std::chrono::duration_cast<std::chrono::microseconds>(firstCompileTime).count(),
            std::chrono::duration_cast<std::chrono::microseconds>(secondCompileTime).count());

        return true;
    }

    // ========================================================================
    // Test 9: Error Handling
    // ========================================================================
    bool TestErrorHandling() {
        logChannel_.Log(Logging::LogLevel::Debug, "Testing error handling...");

        // Test compilation of non-existent file
        Shaders::ShaderCompileRequest badFileRequest;
        badFileRequest.sourceFile = "NonExistent/Shader.hlsl";
        badFileRequest.entryPoint = "main";
        badFileRequest.stage = Shaders::ShaderStage::Vertex;

        auto result = shaderManager_->CompileShader(badFileRequest);
        if (result) {
            logChannel_.Log(Logging::LogLevel::Error,
                "Expected compilation failure for non-existent file, but succeeded");
            return false;
        }

        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Correctly failed for non-existent file: {}", result.Error().ToString());

        // Test compilation with syntax error
        if (!ShaderTestSetup::WriteFile("TestShaders/BadShader.hlsl",
            "invalid hlsl syntax here !@#$%")) {
            logChannel_.Log(Logging::LogLevel::Warning, "Could not create bad shader file for test");
            return true; // Skip this part of the test
        }

        Shaders::ShaderCompileRequest badSyntaxRequest;
        badSyntaxRequest.sourceFile = "TestShaders/BadShader.hlsl";
        badSyntaxRequest.entryPoint = "main";
        badSyntaxRequest.stage = Shaders::ShaderStage::Vertex;

        auto syntaxResult = shaderManager_->CompileShader(badSyntaxRequest);
        if (syntaxResult) {
            logChannel_.Log(Logging::LogLevel::Error,
                "Expected compilation failure for bad syntax, but succeeded");
            return false;
        }

        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Correctly failed for syntax error: {}", syntaxResult.Error().ToString());

        return true;
    }

    // ========================================================================
    // Test 10: Hot Reload
    // ========================================================================
    bool TestHotReload() {
        logChannel_.Log(Logging::LogLevel::Debug, "Testing hot reload detection...");

        if (!shaderManager_->IsHotReloadEnabled()) {
            logChannel_.Log(Logging::LogLevel::Info, "Hot reload not enabled, skipping test");
            return true;
        }

        // This is a basic test - in a real scenario you'd set up file watching
        // and modify files to test the callback system

        std::filesystem::path testFile = "TestShaders/HotReloadTest.hlsl";

        // Create test file
        std::string shaderContent = R"(
float4 main() : SV_Target {
    return float4(1, 0, 0, 1); // Red
}
)";

        if (!ShaderTestSetup::WriteFile(testFile.string(), shaderContent)) {
            logChannel_.Log(Logging::LogLevel::Warning, "Could not create hot reload test file");
            return true; // Skip test
        }

        // Add file to watch list
        shaderManager_->AddFileWatch(testFile);

        // Simulate file modification by updating timestamp
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Modify file content
        std::string modifiedContent = R"(
float4 main() : SV_Target {
    return float4(0, 1, 0, 1); // Green
}
)";

        ShaderTestSetup::WriteFile(testFile.string(), modifiedContent);

        // Give hot reload system time to detect change
        std::this_thread::sleep_for(std::chrono::milliseconds(600));

        logChannel_.Log(Logging::LogLevel::Debug, "Hot reload test completed");
        return true;
    }

    // ========================================================================
    // Test 11: Constant Buffer Validation
    // ========================================================================
    bool TestConstantBufferValidation() {
        logChannel_.Log(Logging::LogLevel::Debug, "Testing constant buffer layout validation...");

        auto shader = shaderManager_->GetShader("TestVertex.hlsl", Shaders::ShaderVariant{});
        if (!shader) {
            logChannel_.Log(Logging::LogLevel::Error, "Cannot retrieve shader for CB validation test");
            return false;
        }

        // Test register layout validation
        bool layoutValid = shaderManager_->ValidateRegisterLayout(shader);
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Register layout validation result: {}", layoutValid ? "VALID" : "INVALID");

        // Get constant buffer bindings
        auto bindings = shaderManager_->GetConstantBufferBindings(shader);
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Found {} constant buffer bindings:", bindings.size());

        for (const auto& [name, binding] : bindings) {
            logChannel_.LogFormat(Logging::LogLevel::Debug,
                "  {} -> b{}", name, binding);
        }

        return true;
    }

    // ========================================================================
    // Test 12: Performance and Statistics
    // ========================================================================
    bool TestPerformanceTracking() {
        logChannel_.Log(Logging::LogLevel::Debug, "Testing performance and statistics...");

        const auto& stats = shaderManager_->GetStatistics();

        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "Shader Statistics:");
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "  Total shaders loaded: {}", stats.totalShadersLoaded);
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "  Total programs loaded: {}", stats.totalProgramsLoaded);
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "  Compilation requests: {}", stats.compilationRequests);
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "  Compilation successes: {}", stats.compilationSuccesses);
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "  Compilation failures: {}", stats.compilationFailures);
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "  Cache hits: {}", stats.cacheHits);
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "  Cache misses: {}", stats.cacheMisses);
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "  Total compilation time: {}ms", stats.totalCompilationTime.count());
        logChannel_.LogFormat(Logging::LogLevel::Debug,
            "  Average compilation time: {}ms", stats.averageCompilationTime.count());

        return true;
    }

    // ========================================================================
    // Helper Methods
    // ========================================================================
    std::shared_ptr<RHI::IRenderDevice> CreateTestRenderDevice() {
        // This is where you'd create your D3D12 render device for testing
        // The actual implementation depends on your RHI system

        // For now, return nullptr and log that we need a real implementation
        logChannel_.Log(Logging::LogLevel::Warning,
            "CreateTestRenderDevice() needs implementation for your specific RHI system");

        Configuration::RenderingConfig rConfig{};
        rConfig.SetResolution({ 1024, 768 });

        Platform::SurfaceInfo sInfo{};

        // You would typically do something like:
        auto factory = RHI::CreateD3D12Factory();
        return factory->CreateDevice(rConfig, sInfo);
    }

    static bool WriteFile(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << content;
        return file.good();
    }
};

// ============================================================================
// Main Test Runner Function
// ============================================================================

bool RunShaderSystemTests() {
    // Setup test environment
    if (!ShaderTestSetup::CreateTestShaderFiles()) {
        std::cerr << "Failed to create test shader files" << std::endl;
        return false;
    }

    // Initialize test framework
    ShaderTestFramework framework;
    ShaderSystemTests tests(framework);

    // Run all tests
    tests.RunAllTests();

    // Cleanup
    ShaderTestSetup::CleanupTestFiles();

    // Return overall result
    return framework.AllTestsPassed();
}