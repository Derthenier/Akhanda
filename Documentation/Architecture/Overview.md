# Akhanda Engine Architecture Deep-dive

## Core Design Principles

### 1. Modular Architecture
- **Static Engine Core** - Single .lib exposing all functionality
- **Dynamic Plugin System** - Runtime-loaded .dll extensions
- **Module Interfaces** - C++23 modules for clean API boundaries
- **Dependency Injection** - Loose coupling between systems

### 2. Performance-First Design
- **Data-Oriented Design** - Cache-friendly memory layouts
- **SIMD Optimization** - Vectorized math operations
- **Job System** - Work-stealing thread pool
- **Asset Streaming** - Async resource loading

## System Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                    Application Layer                    │
├─────────────────┬─────────────────┬─────────────────────┤
│      Editor     │      Game       │     Plugin APIs     │
├─────────────────┴─────────────────┴─────────────────────┤
│                  Engine Library                         │
├─────────────────┬─────────────────┬─────────────────────┤
│   Engine.RHI    │ Engine.Resource │   Engine.Plugin     │
├─────────────────┼─────────────────┼─────────────────────┤
│   Core.Math     │  Core.Memory    │  Core.Containers    │
├─────────────────┴─────────────────┴─────────────────────┤
│                Platform Abstraction                     │
└─────────────────────────────────────────────────────────┘
```

## Module System Design

### 1. Core Modules

**Core.Math** - Foundation mathematics
```cpp
export module Core.Math;

export namespace Akhanda::Math {
    // SIMD-optimized vector types
    struct Vector3 { float x, y, z; };
    struct Vector4 { float x, y, z, w; };
    struct Matrix4 { float m[16]; };
    struct Quaternion { float x, y, z, w; };
    
    // Common operations
    Vector3 Cross(const Vector3& a, const Vector3& b);
    float Dot(const Vector3& a, const Vector3& b);
    Matrix4 Perspective(float fov, float aspect, float near, float far);
}
```

**Core.Memory** - Memory management
```cpp
export module Core.Memory;

export namespace Memory {
    // Custom allocators
    class LinearAllocator;
    class PoolAllocator;
    class StackAllocator;
    
    // Smart pointers with custom deletion
    template<typename T>
    using UniquePtr = std::unique_ptr<T, CustomDeleter<T>>;
    
    // Memory tracking (Debug builds)
    void TrackAllocation(void* ptr, size_t size, const char* file, int line);
    void TrackDeallocation(void* ptr);
}
```

**Core.Containers** - STL-compatible containers
```cpp
export module Core.Containers;

export namespace Containers {
    // Memory-tracked containers
    template<typename T>
    using Vector = std::vector<T, CustomAllocator<T>>;
    
    template<typename K, typename V>
    using HashMap = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, CustomAllocator<std::pair<const K, V>>>;
    
    // Specialized containers
    template<typename T, size_t N>
    class RingBuffer;
    
    template<typename T>
    class ObjectPool;
}
```

### 2. Engine Modules

**Engine.RHI** - Render Hardware Interface
```cpp
export module Engine.RHI;
import Core.Math;

export namespace RHI {
    // Device abstraction
    class RenderDevice {
    public:
        virtual ~RenderDevice() = default;
        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;
        virtual void Present() = 0;
    };
    
    // Resource types
    class Buffer;
    class Texture;
    class Pipeline;
    class CommandList;
    
    // Factory function
    std::unique_ptr<RenderDevice> CreateD3D12Device();
}
```

**Engine.Resource** - Asset management
```cpp
export module Engine.Resource;

export namespace Resource {
    // Asset types
    enum class AssetType { Mesh, Texture, Material, Audio, AIModel };
    
    // Base asset class
    class Asset {
    public:
        virtual ~Asset() = default;
        virtual AssetType GetType() const = 0;
        virtual bool IsLoaded() const = 0;
    };
    
    // Resource manager
    class ResourceManager {
    public:
        template<typename T>
        std::shared_ptr<T> Load(const std::string& path);
        
        void Unload(const std::string& path);
        void ReloadAll();
    };
}
```

**Engine.Plugin** - Plugin system
```cpp
export module Engine.Plugin;

export namespace Plugin {
    // Plugin interface
    class IPlugin {
    public:
        virtual ~IPlugin() = default;
        virtual void OnLoad() = 0;
        virtual void OnUnload() = 0;
        virtual const char* GetName() const = 0;
        virtual const char* GetVersion() const { return "1.0.0"; }
    };
    
    // Plugin manager
    class PluginManager {
    public:
        bool LoadPlugin(const std::string& path);
        void UnloadPlugin(const std::string& name);
        IPlugin* GetPlugin(const std::string& name);
        std::vector<IPlugin*> GetAllPlugins();
    };
}
```

## Plugin Architecture

### 1. Plugin Lifecycle

```cpp
// Plugin loading sequence
1. LoadLibrary() -> Load DLL
2. GetProcAddress("CreatePlugin") -> Get entry point
3. CreatePlugin() -> Instantiate plugin
4. plugin->OnLoad() -> Initialize plugin
5. Register with engine systems
```

### 2. Plugin Categories

**Renderer Plugins**
- D3D12Plugin (built-in)
- VulkanPlugin (planned)
- MetalPlugin (planned)

**Physics Plugins**
- PhysXPlugin
- JoltPlugin (planned)
- BulletPlugin (planned)

**AI Plugins**
- ONNXPlugin (ONNX Runtime integration)
- TensorFlowPlugin (planned)
- PyTorchPlugin (planned)

**Audio Plugins**
- FMODPlugin (planned)
- WwisePlugin (planned)

**Tool Plugins**
- ImGuiPlugin (editor UI)
- AssetImporterPlugin (planned)

### 3. Plugin Communication

**Event System**
```cpp
// Plugins communicate via events
class Event {
public:
    virtual ~Event() = default;
    virtual const char* GetType() const = 0;
};

class EventDispatcher {
public:
    template<typename T>
    void Subscribe(std::function<void(const T&)> handler);
    
    template<typename T>
    void Publish(const T& event);
};
```

**Service Location**
```cpp
// Plugins register services
class ServiceLocator {
public:
    template<typename T>
    void RegisterService(std::unique_ptr<T> service);
    
    template<typename T>
    T* GetService();
};
```

## Memory Architecture

### 1. Allocator Hierarchy

```
Global Heap
├── Engine Allocator (64MB)
│   ├── Render Allocator (32MB)
│   ├── Resource Allocator (16MB)
│   └── Gameplay Allocator (16MB)
└── Plugin Allocators (per-plugin)
    ├── PhysX Allocator (8MB)
    ├── Audio Allocator (4MB)
    └── AI Allocator (16MB)
```

### 2. Memory Tracking

```cpp
// Debug builds track all allocations
#ifdef AKH_DEBUG
    #define AKH_NEW(size) TrackedAlloc(size, __FILE__, __LINE__)
    #define AKH_DELETE(ptr) TrackedFree(ptr, __FILE__, __LINE__)
#else
    #define AKH_NEW(size) malloc(size)
    #define AKH_DELETE(ptr) free(ptr)
#endif
```

## Threading Architecture

### 1. Job System

```cpp
// Work-stealing job system
class JobSystem {
public:
    template<typename F>
    auto SubmitJob(F&& func) -> std::future<decltype(func())>;
    
    void SubmitJobs(std::span<Job> jobs);
    void WaitForCompletion();
    
private:
    std::vector<WorkerThread> workers_;
    WorkStealingQueue job_queue_;
};
```

### 2. Thread Categories

- **Main Thread** - Game logic, plugin management
- **Render Thread** - Command list recording, GPU sync
- **Resource Thread** - Asset loading, file I/O
- **Worker Threads** - Job system workers (CPU count - 3)

### 3. Thread Safety

```cpp
// Thread-safe patterns
class ThreadSafeResourceManager {
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Asset>> assets_;
    
public:
    std::shared_ptr<Asset> GetAsset(const std::string& name) const {
        std::shared_lock lock(mutex_);
        auto it = assets_.find(name);
        return it != assets_.end() ? it->second : nullptr;
    }
};
```

## AI System Integration

### 1. AI as First-Class Citizen

**AI Manager**
```cpp
class AIManager {
public:
    void LoadModel(const std::string& path);
    void RunInference(const std::string& model, const std::vector<float>& input);
    void UpdateBehaviorTrees(float deltaTime);
    
private:
    std::unique_ptr<ONNXRuntime> onnx_runtime_;
    std::vector<BehaviorTree> behavior_trees_;
};
```

**AI Components**
```cpp
// ECS components for AI
struct AIBehaviorComponent {
    std::string behaviorTreeName;
    std::unordered_map<std::string, float> blackboard;
};

struct MLInferenceComponent {
    std::string modelName;
    std::vector<float> inputData;
    std::vector<float> outputData;
    bool needsUpdate = true;
};
```

### 2. AI Plugin Architecture

```cpp
// AI plugins implement this interface
class IAIProvider {
public:
    virtual ~IAIProvider() = default;
    virtual bool LoadModel(const std::string& path) = 0;
    virtual std::vector<float> RunInference(const std::vector<float>& input) = 0;
    virtual void UpdateModel(const std::string& path) = 0;
};
```

## Editor Integration

### 1. Editor Architecture

```cpp
class Editor {
public:
    void Initialize();
    void Update(float deltaTime);
    void Render();
    void Shutdown();
    
private:
    std::unique_ptr<EditorWindow> scene_view_;
    std::unique_ptr<EditorWindow> inspector_;
    std::unique_ptr<EditorWindow> asset_browser_;
    std::unique_ptr<EditorWindow> console_;
};
```

### 2. Editor Plugin System

```cpp
// Editor plugins extend functionality
class IEditorPlugin : public IPlugin {
public:
    virtual void OnEditorInitialize() = 0;
    virtual void OnEditorUpdate(float deltaTime) = 0;
    virtual void OnEditorRender() = 0;
};
```

## Build System Integration

### 1. MSBuild Integration

**Directory.Build.props** - Global settings
```xml
<Project>
  <PropertyGroup>
    <SolutionDir>$(MSBuildThisFileDirectory)</SolutionDir>
    <OutputPath>$(SolutionDir)Build\Output\Bin\$(Platform)\$(Configuration)\</OutputPath>
    <CppLanguageStandard>c++latest</CppLanguageStandard>
    <EnableModules>true</EnableModules>
  </PropertyGroup>
</Project>
```

### 2. Module Compilation Order

1. **Core modules** (no dependencies)
2. **Engine modules** (depend on Core)
3. **Plugin implementations** (depend on Engine)
4. **Applications** (depend on all)

## Error Handling

### 1. Error Categories

```cpp
enum class ErrorCode {
    Success = 0,
    // Core errors (1-99)
    OutOfMemory = 1,
    InvalidParameter = 2,
    // Engine errors (100-199)
    RenderDeviceFailure = 100,
    AssetLoadFailure = 101,
    // Plugin errors (200-299)
    PluginLoadFailure = 200,
    PluginInitFailure = 201
};
```

### 2. Exception Safety

```cpp
// RAII for automatic cleanup
class ScopedRenderTarget {
    RenderDevice* device_;
public:
    ScopedRenderTarget(RenderDevice* device, RenderTarget* target)
        : device_(device) {
        device_->SetRenderTarget(target);
    }
    
    ~ScopedRenderTarget() {
        device_->SetRenderTarget(nullptr);
    }
};
```

## Performance Considerations

### 1. Hot Paths

- **Render loop** - Minimize allocations, cache command lists
- **Physics update** - Use fixed timestep, spatial partitioning  
- **AI inference** - Batch operations, async execution
- **Asset streaming** - Background threads, LRU caching

### 2. Profiling Integration

```cpp
// Built-in profiling macros
#ifdef AKH_PROFILE
    #define PROFILE_SCOPE(name) ScopedProfiler _prof(name)
    #define PROFILE_GPU(name) ScopedGPUProfiler _gpu_prof(name)
#else
    #define PROFILE_SCOPE(name)
    #define PROFILE_GPU(name)
#endif
```

## Troubleshooting Architecture Issues

### Module Issues

**Symptom:** Module interface not found
```
Solutions:
1. Check module is exported with 'export module'
2. Verify correct import statement
3. Ensure proper build dependencies
```

**Symptom:** Circular module dependencies
```
Solutions:
1. Move shared types to separate module
2. Use forward declarations in interfaces
3. Apply dependency inversion principle
```

### Plugin Issues

**Symptom:** Plugin crashes on load
```
Debugging:
1. Check plugin exports CreatePlugin() function
2. Verify ABI compatibility (same compiler/settings)
3. Use plugin in isolation first
```

**Symptom:** Plugin services not available
```
Solutions:
1. Ensure plugin registers services in OnLoad()
2. Check service registration order
3. Verify service lifetime management
```

### Performance Issues

**Symptom:** Frame rate drops
```
Profiling:
1. Use PROFILE_SCOPE macros
2. Check GPU utilization with debug layers
3. Monitor memory allocations per frame
```

**Symptom:** Long loading times
```
Optimization:
1. Implement async asset loading
2. Use asset streaming and LOD systems
3. Profile disk I/O bottlenecks
```

## Design Patterns Used

### 1. Architectural Patterns
- **Plugin Architecture** - Runtime extensibility
- **Service Locator** - Loose coupling between systems
- **Entity Component System** - Data-oriented game objects
- **Command Pattern** - Render command recording

### 2. Memory Patterns
- **Object Pool** - Reduce allocation overhead
- **RAII** - Automatic resource management
- **Custom Allocators** - Domain-specific memory layouts

### 3. Threading Patterns
- **Work Stealing** - Efficient job distribution
- **Producer-Consumer** - Asset loading pipeline
- **Read-Write Locks** - Thread-safe resource access

This architecture provides a solid foundation for building AAA-quality games while maintaining modularity and extensibility through the plugin system.