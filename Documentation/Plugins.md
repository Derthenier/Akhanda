# AI Plugin Development Guide

## Overview

This guide demonstrates building AI plugins for Akhanda Engine, using ONNX Runtime integration as the primary example. AI is a first-class citizen in Akhanda, with dedicated systems for machine learning inference, behavior trees, and neural network-driven gameplay.

## AI Plugin Architecture

### 1. Plugin Interface

```cpp
// Engine/Plugin/IAIPlugin.hpp
#pragma once
import Engine.Plugin;

export namespace AI {
    enum class ModelType {
        Classification,
        Regression,
        Reinforcement,
        Generative
    };
    
    struct InferenceRequest {
        std::string modelName;
        std::vector<float> inputData;
        std::vector<size_t> inputShape;
    };
    
    struct InferenceResult {
        std::vector<float> outputData;
        std::vector<size_t> outputShape;
        float confidence = 0.0f;
        bool success = false;
    };
    
    class IAIProvider {
    public:
        virtual ~IAIProvider() = default;
        
        // Model management
        virtual bool LoadModel(const std::string& path, const std::string& name) = 0;
        virtual void UnloadModel(const std::string& name) = 0;
        virtual bool IsModelLoaded(const std::string& name) const = 0;
        
        // Inference
        virtual InferenceResult RunInference(const InferenceRequest& request) = 0;
        virtual void RunInferenceAsync(const InferenceRequest& request, 
                                     std::function<void(InferenceResult)> callback) = 0;
        
        // Model information
        virtual ModelType GetModelType(const std::string& name) const = 0;
        virtual std::vector<size_t> GetInputShape(const std::string& name) const = 0;
        virtual std::vector<size_t> GetOutputShape(const std::string& name) const = 0;
    };
}
```

### 2. AI Manager Integration

```cpp
// Engine/AI/AIManager.hpp
#pragma once
import Engine.Plugin;
import Core.Containers;

export namespace AI {
    class AIManager {
    public:
        static AIManager& Instance();
        
        // Provider management
        void RegisterProvider(const std::string& name, std::unique_ptr<IAIProvider> provider);
        IAIProvider* GetProvider(const std::string& name);
        
        // Convenience methods
        InferenceResult RunInference(const std::string& provider, const InferenceRequest& request);
        void RunInferenceAsync(const std::string& provider, const InferenceRequest& request,
                             std::function<void(InferenceResult)> callback);
        
        // Global AI settings
        void SetMaxConcurrentInferences(size_t count);
        void EnableGPUAcceleration(bool enable);
        
    private:
        Containers::HashMap<std::string, std::unique_ptr<IAIProvider>> providers_;
        size_t maxConcurrentInferences_ = 4;
        bool gpuAcceleration_ = true;
    };
}
```

## Creating the ONNX Plugin

### 1. Project Setup

Create `Plugins/ONNXPlugin/ONNXPlugin.vcxproj`:

```xml
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|x64">
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x64">
      <Configuration>Release</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Profile|x64">
      <Configuration>Profile</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  
  <PropertyGroup Label="Globals">
    <VCProjectVersion>17.0</VCProjectVersion>
    <ProjectGuid>{7AC4DED5-F7CB-4D90-B7FC-6A91DA5D7A3D}</ProjectGuid>
    <RootNamespace>ONNXPlugin</RootNamespace>
    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
  </PropertyGroup>
  
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  
  <ImportGroup Label="PropertySheets">
    <Import Project="..\..\Build\Props\Plugin.props" />
    <Import Project="..\..\Build\Props\ThirdParty.props" />
  </ImportGroup>
  
  <ItemDefinitionGroup>
    <ClCompile>
      <PreprocessorDefinitions>ONNXPLUGIN_EXPORTS;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>$(ThirdPartyDir)onnxruntime\include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
    </ClCompile>
    <Link>
      <AdditionalDependencies>onnxruntime.lib;%(AdditionalDependencies)</AdditionalDependencies>
    </Link>
  </ItemDefinitionGroup>
  
  <ItemGroup>
    <ClCompile Include="Source\ONNXProvider.cpp" />
    <ClCompile Include="Source\Plugin.cpp" />
  </ItemGroup>
  
  <ItemGroup>
    <ClInclude Include="Source\ONNXProvider.hpp" />
  </ItemGroup>
  
  <ItemGroup>
    <ProjectReference Include="..\..\Engine\Engine.vcxproj">
      <Project>{6EBA6490-35F7-4578-B7E8-EEA36BA09570}</Project>
    </ProjectReference>
  </ItemGroup>
  
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
</Project>
```

### 2. ONNX Provider Implementation

```cpp
// Plugins/ONNXPlugin/Source/ONNXProvider.hpp
#pragma once
import Engine.Plugin;
import Core.Containers;
import Core.Memory;
#include <onnxruntime_cxx_api.h>

class ONNXProvider : public AI::IAIProvider {
public:
    ONNXProvider();
    ~ONNXProvider() override;
    
    // IAIProvider implementation
    bool LoadModel(const std::string& path, const std::string& name) override;
    void UnloadModel(const std::string& name) override;
    bool IsModelLoaded(const std::string& name) const override;
    
    AI::InferenceResult RunInference(const AI::InferenceRequest& request) override;
    void RunInferenceAsync(const AI::InferenceRequest& request, 
                          std::function<void(AI::InferenceResult)> callback) override;
    
    AI::ModelType GetModelType(const std::string& name) const override;
    std::vector<size_t> GetInputShape(const std::string& name) const override;
    std::vector<size_t> GetOutputShape(const std::string& name) const override;
    
private:
    struct ModelInfo {
        std::unique_ptr<Ort::Session> session;
        std::vector<std::string> inputNames;
        std::vector<std::string> outputNames;
        std::vector<std::vector<int64_t>> inputShapes;
        std::vector<std::vector<int64_t>> outputShapes;
        AI::ModelType type = AI::ModelType::Classification;
    };
    
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::SessionOptions> sessionOptions_;
    Containers::HashMap<std::string, std::unique_ptr<ModelInfo>> models_;
    
    AI::ModelType DetermineModelType(const ModelInfo& info) const;
    void ConfigureSessionOptions();
};
```

```cpp
// Plugins/ONNXPlugin/Source/ONNXProvider.cpp
#include "ONNXProvider.hpp"
import Core.Memory;
#include <filesystem>
#include <future>

ONNXProvider::ONNXProvider() {
    try {
        env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "AkhandaONNX");
        sessionOptions_ = std::make_unique<Ort::SessionOptions>();
        ConfigureSessionOptions();
    }
    catch (const Ort::Exception& e) {
        // Log error
        env_.reset();
        sessionOptions_.reset();
    }
}

ONNXProvider::~ONNXProvider() {
    models_.clear();
    sessionOptions_.reset();
    env_.reset();
}

void ONNXProvider::ConfigureSessionOptions() {
    if (!sessionOptions_) return;
    
    // Enable GPU if available
    try {
        auto providers = Ort::GetAvailableProviders();
        bool hasCUDA = std::find(providers.begin(), providers.end(), "CUDAExecutionProvider") != providers.end();
        bool hasDML = std::find(providers.begin(), providers.end(), "DmlExecutionProvider") != providers.end();
        
        if (hasCUDA) {
            sessionOptions_->AppendExecutionProvider_CUDA({});
        }
        else if (hasDML) {
            sessionOptions_->AppendExecutionProvider_DML({});
        }
    }
    catch (const Ort::Exception&) {
        // Fall back to CPU
    }
    
    // Optimization settings
    sessionOptions_->SetIntraOpNumThreads(4);
    sessionOptions_->SetInterOpNumThreads(2);
    sessionOptions_->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
}

bool ONNXProvider::LoadModel(const std::string& path, const std::string& name) {
    if (!env_ || !sessionOptions_) {
        return false;
    }
    
    if (models_.find(name) != models_.end()) {
        // Model already loaded
        return true;
    }
    
    try {
        auto modelInfo = std::make_unique<ModelInfo>();
        
        // Load session
        std::wstring widePath;
        widePath.assign(path.begin(), path.end());
        modelInfo->session = std::make_unique<Ort::Session>(*env_, widePath.c_str(), *sessionOptions_);
        
        // Get input info
        size_t numInputs = modelInfo->session->GetInputCount();
        modelInfo->inputNames.reserve(numInputs);
        modelInfo->inputShapes.reserve(numInputs);
        
        Ort::AllocatorWithDefaultOptions allocator;
        for (size_t i = 0; i < numInputs; ++i) {
            auto inputName = modelInfo->session->GetInputNameAllocated(i, allocator);
            modelInfo->inputNames.emplace_back(inputName.get());
            
            auto inputTypeInfo = modelInfo->session->GetInputTypeInfo(i);
            auto tensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
            modelInfo->inputShapes.emplace_back(tensorInfo.GetShape());
        }
        
        // Get output info
        size_t numOutputs = modelInfo->session->GetOutputCount();
        modelInfo->outputNames.reserve(numOutputs);
        modelInfo->outputShapes.reserve(numOutputs);
        
        for (size_t i = 0; i < numOutputs; ++i) {
            auto outputName = modelInfo->session->GetOutputNameAllocated(i, allocator);
            modelInfo->outputNames.emplace_back(outputName.get());
            
            auto outputTypeInfo = modelInfo->session->GetOutputTypeInfo(i);
            auto tensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
            modelInfo->outputShapes.emplace_back(tensorInfo.GetShape());
        }
        
        // Determine model type
        modelInfo->type = DetermineModelType(*modelInfo);
        
        models_[name] = std::move(modelInfo);
        return true;
    }
    catch (const Ort::Exception& e) {
        // Log error: e.what()
        return false;
    }
}

void ONNXProvider::UnloadModel(const std::string& name) {
    auto it = models_.find(name);
    if (it != models_.end()) {
        models_.erase(it);
    }
}

bool ONNXProvider::IsModelLoaded(const std::string& name) const {
    return models_.find(name) != models_.end();
}

AI::InferenceResult ONNXProvider::RunInference(const AI::InferenceRequest& request) {
    AI::InferenceResult result;
    
    auto it = models_.find(request.modelName);
    if (it == models_.end()) {
        return result; // success = false
    }
    
    const auto& modelInfo = *it->second;
    
    try {
        // Prepare input tensor
        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
        
        std::vector<Ort::Value> inputTensors;
        std::vector<const char*> inputNames;
        
        // Convert input shape to int64_t
        std::vector<int64_t> inputShape;
        inputShape.reserve(request.inputShape.size());
        for (size_t dim : request.inputShape) {
            inputShape.push_back(static_cast<int64_t>(dim));
        }
        
        inputTensors.emplace_back(
            Ort::Value::CreateTensor<float>(
                memoryInfo, 
                const_cast<float*>(request.inputData.data()), 
                request.inputData.size(),
                inputShape.data(), 
                inputShape.size()
            )
        );
        
        for (const auto& name : modelInfo.inputNames) {
            inputNames.push_back(name.c_str());
        }
        
        // Prepare output names
        std::vector<const char*> outputNames;
        for (const auto& name : modelInfo.outputNames) {
            outputNames.push_back(name.c_str());
        }
        
        // Run inference
        auto outputTensors = modelInfo.session->Run(
            Ort::RunOptions{nullptr}, 
            inputNames.data(), 
            inputTensors.data(), 
            inputTensors.size(),
            outputNames.data(), 
            outputNames.size()
        );
        
        // Extract results
        if (!outputTensors.empty() && outputTensors[0].IsTensor()) {
            auto* floatData = outputTensors[0].GetTensorData<float>();
            auto tensorInfo = outputTensors[0].GetTensorTypeAndShapeInfo();
            auto shape = tensorInfo.GetShape();
            size_t elementCount = tensorInfo.GetElementCount();
            
            result.outputData.assign(floatData, floatData + elementCount);
            result.outputShape.reserve(shape.size());
            for (int64_t dim : shape) {
                result.outputShape.push_back(static_cast<size_t>(dim));
            }
            
            // Calculate confidence (for classification models)
            if (modelInfo.type == AI::ModelType::Classification && !result.outputData.empty()) {
                result.confidence = *std::max_element(result.outputData.begin(), result.outputData.end());
            }
            
            result.success = true;
        }
    }
    catch (const Ort::Exception& e) {
        // Log error: e.what()
        result.success = false;
    }
    
    return result;
}

void ONNXProvider::RunInferenceAsync(const AI::InferenceRequest& request, 
                                    std::function<void(AI::InferenceResult)> callback) {
    std::async(std::launch::async, [this, request, callback]() {
        auto result = RunInference(request);
        callback(result);
    });
}

AI::ModelType ONNXProvider::GetModelType(const std::string& name) const {
    auto it = models_.find(name);
    return it != models_.end() ? it->second->type : AI::ModelType::Classification;
}

std::vector<size_t> ONNXProvider::GetInputShape(const std::string& name) const {
    auto it = models_.find(name);
    if (it == models_.end() || it->second->inputShapes.empty()) {
        return {};
    }
    
    std::vector<size_t> shape;
    for (int64_t dim : it->second->inputShapes[0]) {
        shape.push_back(static_cast<size_t>(dim));
    }
    return shape;
}

std::vector<size_t> ONNXProvider::GetOutputShape(const std::string& name) const {
    auto it = models_.find(name);
    if (it == models_.end() || it->second->outputShapes.empty()) {
        return {};
    }
    
    std::vector<size_t> shape;
    for (int64_t dim : it->second->outputShapes[0]) {
        shape.push_back(static_cast<size_t>(dim));
    }
    return shape;
}

AI::ModelType ONNXProvider::DetermineModelType(const ModelInfo& info) const {
    // Heuristic: determine type based on output shape
    if (info.outputShapes.empty()) {
        return AI::ModelType::Classification;
    }
    
    const auto& outputShape = info.outputShapes[0];
    if (outputShape.size() == 2 && outputShape[1] > 1) {
        return AI::ModelType::Classification;
    }
    else if (outputShape.size() == 2 && outputShape[1] == 1) {
        return AI::ModelType::Regression;
    }
    
    return AI::ModelType::Classification;
}
```

### 3. Plugin Entry Point

```cpp
// Plugins/ONNXPlugin/Source/Plugin.cpp
import Engine.Plugin;
#include "ONNXProvider.hpp"

class ONNXPlugin : public IPlugin {
public:
    void OnLoad() override {
        // Register AI provider
        auto provider = std::make_unique<ONNXProvider>();
        AI::AIManager::Instance().RegisterProvider("ONNX", std::move(provider));
        
        // Log success
        #ifdef AKH_DEBUG
            OutputDebugStringA("ONNXPlugin loaded successfully\n");
        #endif
    }
    
    void OnUnload() override {
        // Provider will be automatically cleaned up by AIManager
        #ifdef AKH_DEBUG
            OutputDebugStringA("ONNXPlugin unloaded\n");
        #endif
    }
    
    const char* GetName() const override {
        return "ONNXPlugin";
    }
    
    const char* GetVersion() const override {
        return "1.0.0";
    }
};

// Plugin entry point
extern "C" __declspec(dllexport) IPlugin* CreatePlugin() {
    return new ONNXPlugin();
}
```

## ECS Integration for AI

### 1. AI Components

```cpp
// Engine/AI/Components.hpp
#pragma once
import Core.Containers;

export namespace AI {
    // ML inference component
    struct MLInferenceComponent {
        std::string providerName = "ONNX";
        std::string modelName;
        std::vector<float> inputData;
        std::vector<float> outputData;
        bool needsUpdate = true;
        bool asyncInference = true;
        float updateInterval = 0.1f; // seconds
        float lastUpdateTime = 0.0f;
    };
    
    // Behavior tree component
    struct BehaviorTreeComponent {
        std::string treeName;
        Containers::HashMap<std::string, float> blackboard;
        enum class State { Running, Success, Failure } currentState = State::Running;
        float updateInterval = 0.033f; // 30 FPS
        float lastUpdateTime = 0.0f;
    };
    
    // Decision making component
    struct DecisionComponent {
        std::string decisionModel; // Name of ML model for decisions
        std::vector<float> stateVector; // Current game state as input
        std::vector<float> actionProbabilities; // Model output
        int selectedAction = -1;
        float decisionConfidence = 0.0f;
    };
    
    // Learning component (for reinforcement learning)
    struct LearningComponent {
        std::string agentModel;
        std::vector<float> experienceBuffer;
        size_t maxExperiences = 10000;
        float learningRate = 0.001f;
        bool isTraining = false;
    };
}
```

### 2. AI Systems

```cpp
// Engine/AI/Systems.hpp
#pragma once
import Engine.ECS;
import Engine.AI;

export namespace AI {
    class MLInferenceSystem : public ECS::System {
    public:
        void Update(float deltaTime) override {
            auto& aiManager = AIManager::Instance();
            
            for (auto entity : GetEntities<MLInferenceComponent>()) {
                auto& mlComp = GetComponent<MLInferenceComponent>(entity);
                
                mlComp.lastUpdateTime += deltaTime;
                if (mlComp.lastUpdateTime >= mlComp.updateInterval && mlComp.needsUpdate) {
                    InferenceRequest request{
                        .modelName = mlComp.modelName,
                        .inputData = mlComp.inputData
                    };
                    
                    if (mlComp.asyncInference) {
                        aiManager.RunInferenceAsync(mlComp.providerName, request,
                            [&mlComp](InferenceResult result) {
                                if (result.success) {
                                    mlComp.outputData = std::move(result.outputData);
                                }
                                mlComp.needsUpdate = false;
                            });
                    }
                    else {
                        auto result = aiManager.RunInference(mlComp.providerName, request);
                        if (result.success) {
                            mlComp.outputData = std::move(result.outputData);
                        }
                        mlComp.needsUpdate = false;
                    }
                    
                    mlComp.lastUpdateTime = 0.0f;
                }
            }
        }
    };
    
    class DecisionSystem : public ECS::System {
    public:
        void Update(float deltaTime) override {
            for (auto entity : GetEntities<DecisionComponent, MLInferenceComponent>()) {
                auto& decision = GetComponent<DecisionComponent>(entity);
                auto& mlComp = GetComponent<MLInferenceComponent>(entity);
                
                // Update ML component with current state
                mlComp.inputData = decision.stateVector;
                mlComp.needsUpdate = true;
                
                // Process ML results
                if (!mlComp.outputData.empty()) {
                    decision.actionProbabilities = mlComp.outputData;
                    
                    // Select action with highest probability
                    auto maxIt = std::max_element(decision.actionProbabilities.begin(), 
                                                decision.actionProbabilities.end());
                    decision.selectedAction = static_cast<int>(
                        std::distance(decision.actionProbabilities.begin(), maxIt));
                    decision.decisionConfidence = *maxIt;
                }
            }
        }
    };
}
```

## Game Integration Example

### 1. AI-Driven NPC

```cpp
// Game/Source/NPCController.cpp
import Engine.ECS;
import Engine.AI;
import Core.Math;

class NPCController : public ECS::Component {
public:
    void Initialize(ECS::Entity entity) {
        // Add AI components
        auto& mlComp = AddComponent<AI::MLInferenceComponent>(entity);
        mlComp.modelName = "npc_behavior";
        mlComp.providerName = "ONNX";
        mlComp.updateInterval = 0.5f; // Update every 500ms
        
        auto& decision = AddComponent<AI::DecisionComponent>(entity);
        decision.decisionModel = "npc_behavior";
        
        // Add behavior tree for complex behaviors
        auto& btComp = AddComponent<AI::BehaviorTreeComponent>(entity);
        btComp.treeName = "npc_combat_tree";
    }
    
    void UpdateStateVector(ECS::Entity entity, const Akhanda::Math::Vector3& playerPos, float health) {
        auto& decision = GetComponent<AI::DecisionComponent>(entity);
        auto& transform = GetComponent<TransformComponent>(entity);
        
        // Build state vector for ML model
        decision.stateVector = {
            transform.position.x, transform.position.y, transform.position.z, // NPC position
            playerPos.x, playerPos.y, playerPos.z,                           // Player position
            health,                                                           // NPC health
            Akhanda::Math::Distance(transform.position, playerPos),                   // Distance to player
            // Add more features as needed
        };
    }
    
    void ExecuteAction(ECS::Entity entity) {
        auto& decision = GetComponent<AI::DecisionComponent>(entity);
        
        switch (decision.selectedAction) {
            case 0: // Move towards player
                MoveTowardsPlayer(entity);
                break;
            case 1: // Attack player
                AttackPlayer(entity);
                break;
            case 2: // Retreat
                Retreat(entity);
                break;
            case 3: // Use special ability
                UseSpecialAbility(entity);
                break;
        }
    }
    
private:
    void MoveTowardsPlayer(ECS::Entity entity) { /* Implementation */ }
    void AttackPlayer(ECS::Entity entity) { /* Implementation */ }
    void Retreat(ECS::Entity entity) { /* Implementation */ }
    void UseSpecialAbility(ECS::Entity entity) { /* Implementation */ }
};
```

### 2. Dynamic Difficulty Adjustment

```cpp
// Game/Source/DifficultyAI.cpp
import Engine.AI;

class DifficultyAI {
public:
    void Initialize() {
        // Load difficulty adjustment model
        auto& aiManager = AI::AIManager::Instance();
        auto* provider = aiManager.GetProvider("ONNX");
        
        if (provider && !provider->IsModelLoaded("difficulty_model")) {
            provider->LoadModel("Assets/AI/difficulty_adjustment.onnx", "difficulty_model");
        }
    }
    
    void UpdateDifficulty(float playerPerformance, float gameTime) {
        auto& aiManager = AI::AIManager::Instance();
        
        AI::InferenceRequest request{
            .modelName = "difficulty_model",
            .inputData = {
                playerPerformance,  // Win rate, accuracy, etc.
                gameTime,          // Time in current level
                currentDifficulty_, // Current difficulty setting
                playerStress_      // Heart rate, input frequency, etc.
            }
        };
        
        aiManager.RunInferenceAsync("ONNX", request, 
            [this](AI::InferenceResult result) {
                if (result.success && !result.outputData.empty()) {
                    float newDifficulty = result.outputData[0];
                    AdjustGameDifficulty(newDifficulty);
                }
            });
    }
    
private:
    float currentDifficulty_ = 0.5f;
    float playerStress_ = 0.0f;
    
    void AdjustGameDifficulty(float difficulty) {
        currentDifficulty_ = std::clamp(difficulty, 0.0f, 1.0f);
        
        // Apply difficulty changes
        GameSettings::SetEnemyHealth(1.0f + currentDifficulty_);
        GameSettings::SetEnemyDamage(0.8f + currentDifficulty_ * 0.4f);
        GameSettings::SetSpawnRate(1.0f + currentDifficulty_ * 0.5f);
    }
};
```

## Advanced AI Features

### 1. Procedural Content Generation

```cpp
// Engine/AI/ProceduralGeneration.hpp
#pragma once
import Engine.AI;

export namespace AI {
    class ProceduralGenerator {
    public:
        struct GenerationRequest {
            std::string generatorType; // "level", "quest", "dialogue"
            std::vector<float> constraints;
            std::vector<float> playerPreferences;
        };
        
        void LoadGenerator(const std::string& type, const std::string& modelPath) {
            auto& aiManager = AIManager::Instance();
            auto* provider = aiManager.GetProvider("ONNX");
            
            if (provider) {
                provider->LoadModel(modelPath, type + "_generator");
            }
        }
        
        std::vector<float> GenerateContent(const GenerationRequest& request) {
            auto& aiManager = AIManager::Instance();
            
            AI::InferenceRequest inferenceRequest{
                .modelName = request.generatorType + "_generator",
                .inputData = request.constraints
            };
            
            // Append player preferences
            inferenceRequest.inputData.insert(
                inferenceRequest.inputData.end(),
                request.playerPreferences.begin(),
                request.playerPreferences.end()
            );
            
            auto result = aiManager.RunInference("ONNX", inferenceRequest);
            return result.success ? result.outputData : std::vector<float>{};
        }
    };
}
```

### 2. Adaptive Audio System

```cpp
// Plugins/AudioAI/Source/AdaptiveAudio.cpp
import Engine.AI;
import Engine.Audio;

class AdaptiveAudioSystem {
public:
    void UpdateMusic(float tension, float excitement, float exploration) {
        AI::InferenceRequest request{
            .modelName = "music_adaptation",
            .inputData = {tension, excitement, exploration, currentMoodState_}
        };
        
        auto& aiManager = AI::AIManager::Instance();
        aiManager.RunInferenceAsync("ONNX", request,
            [this](AI::InferenceResult result) {
                if (result.success && result.outputData.size() >= 3) {
                    float tempo = result.outputData[0];
                    float intensity = result.outputData[1];
                    float harmonyComplexity = result.outputData[2];
                    
                    ApplyMusicParameters(tempo, intensity, harmonyComplexity);
                }
            });
    }
    
private:
    float currentMoodState_ = 0.5f;
    
    void ApplyMusicParameters(float tempo, float intensity, float harmony) {
        // Apply to audio engine
        Audio::SetMusicTempo(tempo);
        Audio::SetMusicIntensity(intensity);
        Audio::SetHarmonyComplexity(harmony);
    }
};
```

## Testing AI Plugins

### 1. Unit Tests

```cpp
// Tests/AI/ONNXPluginTests.cpp
#include <gtest/gtest.h>
import Engine.AI;

class ONNXPluginTest : public ::testing::Test {
protected:
    void SetUp() override {
        aiManager_ = &AI::AIManager::Instance();
        // Load test plugin
        plugin_ = std::make_unique<ONNXPlugin>();
        plugin_->OnLoad();
    }
    
    void TearDown() override {
        plugin_->OnUnload();
        plugin_.reset();
    }
    
    AI::AIManager* aiManager_;
    std::unique_ptr<ONNXPlugin> plugin_;
};

TEST_F(ONNXPluginTest, LoadSimpleModel) {
    auto* provider = aiManager_->GetProvider("ONNX");
    ASSERT_NE(provider, nullptr);
    
    bool loaded = provider->LoadModel("TestAssets/simple_classifier.onnx", "test_model");
    EXPECT_TRUE(loaded);
    EXPECT_TRUE(provider->IsModelLoaded("test_model"));
}

TEST_F(ONNXPluginTest, RunInference) {
    auto* provider = aiManager_->GetProvider("ONNX");
    provider->LoadModel("TestAssets/simple_classifier.onnx", "test_model");
    
    AI::InferenceRequest request{
        .modelName = "test_model",
        .inputData = {1.0f, 2.0f, 3.0f, 4.0f},
        .inputShape = {1, 4}
    };
    
    auto result = provider->RunInference(request);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.outputData.empty());
    EXPECT_GT(result.confidence, 0.0f);
}
```

### 2. Integration Tests

```cpp
// Tests/AI/AISystemIntegrationTest.cpp
#include <gtest/gtest.h>
import Engine.ECS;
import Engine.AI;

TEST(AISystemIntegration, MLInferenceSystemUpdate) {
    ECS::World world;
    AI::MLInferenceSystem mlSystem;
    
    // Create test entity
    auto entity = world.CreateEntity();
    auto& mlComp = world.AddComponent<AI::MLInferenceComponent>(entity);
    mlComp.modelName = "test_model";
    mlComp.inputData = {1.0f, 2.0f, 3.0f};
    mlComp.updateInterval = 0.1f;
    
    // Update system
    mlSystem.Update(0.15f); // Should trigger update
    
    // Verify results
    EXPECT_FALSE(mlComp.needsUpdate);
    EXPECT_FALSE(mlComp.outputData.empty());
}
```

## Performance Optimization

### 1. Model Batching

```cpp
// Engine/AI/BatchInference.hpp
export namespace AI {
    class BatchInferenceManager {
    public:
        void QueueInference(const InferenceRequest& request, std::function<void(InferenceResult)> callback);
        void ProcessBatch();
        
    private:
        struct BatchedRequest {
            InferenceRequest request;
            std::function<void(InferenceResult)> callback;
        };
        
        std::vector<BatchedRequest> pendingRequests_;
        size_t maxBatchSize_ = 32;
    };
}
```

### 2. Model Quantization

```cpp
// Configure ONNX session for optimized inference
void ONNXProvider::ConfigureSessionOptions() {
    if (!sessionOptions_) return;
    
    // Enable optimizations
    sessionOptions_->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    
    // Use quantized models when available
    sessionOptions_->AddConfigEntry("session.use_quantized_weights", "1");
    
    // Memory optimization
    sessionOptions_->AddConfigEntry("session.memory.enable_memory_pattern", "1");
}
```

## Troubleshooting

### Common Issues

**Issue: Model loading fails**
```
Symptoms: LoadModel returns false, no error logging
Solutions:
1. Verify ONNX model file exists and is readable
2. Check model compatibility with ONNX Runtime version
3. Ensure sufficient memory for model loading
4. Validate model with ONNX tools before loading
```

**Issue: Inference returns incorrect results**
```
Symptoms: InferenceResult.success = true but outputs are wrong
Solutions:
1. Verify input data format matches model expectations
2. Check input/output shapes are correct
3. Ensure proper data preprocessing
4. Validate model with known test cases
```

**Issue: Performance degradation**
```
Symptoms: Frame rate drops when AI systems are active
Solutions:
1. Use async inference for non-critical AI
2. Implement inference batching
3. Reduce inference frequency for background AI
4. Profile GPU/CPU usage to identify bottlenecks
```

**Issue: Plugin loading fails**
```
Symptoms: Plugin DLL not found or CreatePlugin entry point missing
Solutions:
1. Verify ONNX Runtime DLLs are in output directory
2. Check plugin exports CreatePlugin() function correctly
3. Ensure proper linkage against Engine.lib
4. Use Dependency Walker to check DLL dependencies
```

### Debugging Tips

**Enable verbose logging:**
```cpp
// In ONNXProvider constructor
env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_VERBOSE, "AkhandaONNX");
```

**Profile inference time:**
```cpp
auto start = std::chrono::high_resolution_clock::now();
auto result = provider->RunInference(request);
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
// Log duration.count()
```

**Memory usage monitoring:**
```cpp
#ifdef AKH_DEBUG
class AIMemoryTracker {
public:
    static void LogMemoryUsage() {
        // Log current memory usage
        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
        // Log pmc.WorkingSetSize
    }
};
#endif
```

This comprehensive guide provides everything needed to build AI plugins for Akhanda Engine, from basic ONNX integration to advanced features like procedural generation and adaptive systems.