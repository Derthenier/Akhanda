# <center> [ Akhanda ] Game Engine </center>

<div style="margin: 0 auto; height: 70vh; overflow:hidden; background:black; background: radial-gradient(ellipse at center, #01050b 0%, #000000 100%); display: flex; justify-content: center; align-items: center; background-repeat: no-repeat; background-attachment: fixed; background-size: cover;">
<img src="Akhanda.png" alt="Akhanda Game Engine" style="object-fit:cover; max-width:50%; height:auto;border-radius:30%">
</div>

---

A modular, plugin-based 3D game engine built with C++23 and designed for AAA-quality games. Features AI as a first-class system and includes an integrated editor.

**🚧 Current Status: Foundation Complete, Basic Rendering In Progress**

---

## Current Features (Implemented)

### ✅ **Solid Foundation**
- **Modern C++23** with module support and RAII design
- **Plugin Architecture** for runtime extensibility
- **D3D12 Infrastructure** with device initialization and resource management
- **Memory Management** with custom allocators and leak detection
- **Job System** for parallel execution with work-stealing queues
- **Logging System** with multi-threaded support and editor integration
- **Platform Abstraction** for Windows with Win32 integration

### 🔄 **In Development**
- **Basic Renderer** - StandardRenderer with frame management (architecture complete)
- **Shader System** - HLSL compilation infrastructure (implementation in progress)
- **Triangle Rendering** - First visible output (next milestone)

### 📋 **Planned Features**
- **Entity Component System** for game objects
- **Advanced Asset Pipeline** with hot-reload support
- **AI-First Design** with ONNX Runtime integration
- **Integrated Editor** built with Dear ImGui
- **Physics Integration** with Jolt Physics
- **Audio System** with 3D spatial audio

## Requirements

- Windows 10/11 (64-bit)
- Visual Studio 2022 (v17.5+)
- Windows SDK 10.0.22621.0+

## Quick Start

1. **Clone the repository**
   ```bash
   git clone https://github.com/Derthenier/Akhanda.git
   cd Akhanda
   ```

2. **Open solution**
   - Open `Akhanda.slnx` in Visual Studio 2022
   - Set `Editor` as startup project
   - Build and run (F5)

3. **Current Behavior**
   - Window creation with platform integration
   - D3D12 device initialization and validation
   - Plugin loading system demonstration
   - Basic editor framework (no 3D rendering yet)

## Project Structure

```
Akhanda/
├── Engine/          # Core engine (static library) - ✅ Foundation Complete
│   ├── Core/        # Math, memory, threading, containers - ✅ Complete  
│   ├── Platform/    # Windows platform layer - ✅ Complete
│   ├── Renderer/    # D3D12 RHI and StandardRenderer - 🔄 In Progress
│   └── Plugins/     # Plugin system interfaces - ✅ Complete
├── Plugins/         # Engine plugins (DLLs) - 📋 Planned
├── Editor/          # Standalone editor application - 🔄 Basic Framework
├── Game/            # Your game project - 📋 Template Ready
├── ThirdParty/      # External dependencies - ✅ Configured
└── Build/           # Build scripts and output - ✅ MSBuild Setup
```

## Architecture (Planned)

### Core Systems (✅ Implemented)
- **Memory**: Custom allocators with tracking and debug validation
- **Math**: SIMD-optimized math library with comprehensive coverage
- **Threading**: Job system with work-stealing queues and coroutine support
- **Platform**: Windows abstraction with event handling and input

### Engine Modules (Current Status)
```cpp
// ✅ Available Now
import Akhanda.Core.Math;         // Vectors, matrices, quaternions
import Akhanda.Core.Memory;       // Allocators and memory management  
import Akhanda.Core.Threading;    // Job system and work queues
import Akhanda.Platform.Windows;  // Platform abstraction

// 🔄 In Development  
import Akhanda.Renderer.RHI;      // D3D12 render hardware interface
import Akhanda.Renderer.Shaders;  // Shader compilation system

// 📋 Planned
import Akhanda.Engine.Resource;   // Asset management system
import Akhanda.Engine.ECS;        // Entity component system
import Akhanda.Engine.Plugin;     // Full plugin API
```

### Plugin System (Architecture Ready)
Plugin framework is implemented and tested:
```cpp
class MyPlugin : public IPlugin {
    void OnLoad() override {
        // Plugin initialization
    }
    void OnUnload() override {
        // Cleanup
    }
};
```

## Building from Source

### Debug Build
```bash
msbuild Akhanda.sln /p:Configuration=Debug /p:Platform=x64
```

### Release Build
```bash
msbuild Akhanda.sln /p:Configuration=Release /p:Platform=x64
```

### Profile Build (with profiling markers)
```bash
msbuild Akhanda.sln /p:Configuration=Profile /p:Platform=x64
```

**Expected Build Outputs:**
```
Build/Output/Bin/x64/Debug/
├── Engine_Debug.lib      # ✅ Core engine library
├── Editor_Debug.exe      # 🔄 Basic editor (window + D3D12 init)
└── ThreadsOfKaliyuga_Debug.exe  # 📋 Game template
```

## Current Development Status

### ✅ **What Works Right Now**
1. **Window Creation** - Fully functional Win32 integration
2. **D3D12 Initialization** - Device, queues, and swap chain setup
3. **Memory System** - Production-ready allocators with leak detection
4. **Job System** - Parallel task execution with work-stealing
5. **Plugin Loading** - Dynamic library system with proper lifecycle
6. **Logging** - Multi-threaded logging with editor integration

### 🔄 **What's In Progress**
1. **Shader Compilation** - HLSL to bytecode pipeline (75% complete)
2. **Basic Rendering** - Triangle rendering implementation (next milestone)
3. **Editor Framework** - ImGui integration (basic structure ready)

### 📋 **Next Milestones**
1. **First Triangle** - Visible rendering output (2-3 weeks)
2. **Basic 3D Rendering** - Meshes and textures (1-2 months)
3. **Asset Pipeline** - Resource loading system (2-3 months)
4. **Entity System** - Game object management (3-4 months)

## Development Workflow

### For Engine Development
```cpp
// 1. Start with foundation systems (available now)
import Akhanda.Core.Math;
Vector3 position{1.0f, 2.0f, 3.0f};
Matrix4 transform = Matrix4::Translation(position);

// 2. Work with rendering infrastructure  
// StandardRenderer interfaces are ready for implementation
auto renderer = std::make_unique<StandardRenderer>();
renderer->Initialize(config, surfaceInfo);

// 3. Plugin development
// Create plugins using IPlugin interface
class MyGamePlugin : public IPlugin { /*...*/ };
```

### For Game Development (Future)
```cpp
// Coming soon - Entity Component System
// auto entity = world.CreateEntity();
// entity.AddComponent<TransformComponent>(transform);
// entity.AddComponent<MeshComponent>(meshId);

// Coming soon - Asset loading
// auto mesh = AssetManager::LoadMesh("character.fbx");
// auto texture = AssetManager::LoadTexture("diffuse.png");
```

## Third-Party Libraries

### ✅ **Integrated**
- **Rendering**: D3D12 (Windows SDK)
- **Math**: Custom SIMD implementation
- **Logging**: spdlog integration
- **Build**: MSBuild with custom .props

### 📋 **Planned Integration**
- **Physics**: Jolt Physics
- **UI**: Dear ImGui
- **AI/ML**: ONNX Runtime  
- **Audio**: FMOD/Wwise
- **Scripting**: Lua (sol3)

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Coding Standards
- Follow C++ Core Guidelines
- Use modules for new interfaces
- Maintain 100% warning-free compilation
- Add unit tests for new features (when test framework is ready)

### Current Development Priorities
1. **Complete shader compilation system** (Phase 2.2)
2. **Implement triangle rendering** (Phase 2.3)
3. **Add basic mesh and texture support** (Phase 3.1)

## Documentation

- [Getting Started Guide](Documentation/GettingStarted.md)
- [Architecture Overview](Documentation/Architecture/Overview.md) *(Planned features)*
- [Development Roadmap](Documentation/Roadmap.md) *(Current progress tracking)*
- [Plugin Development](Documentation/Plugins.md) *(Coming soon)*

## Engine Vision

**Target Use Case**: Development of "Threads of Kaliyuga" - A 3D RPG with AI-driven narratives

**Design Goals**:
- **AI-First**: Machine learning and AI systems are core engine features, not afterthoughts
- **Modular**: Plugin architecture enables easy extension and third-party integration  
- **Performance**: Data-oriented design with modern C++23 for maximum efficiency
- **Developer-Friendly**: Comprehensive tooling and hot-reload support for rapid iteration

**Timeline**: 
- **MVP Engine**: 4-5 months (basic 3D rendering + ECS + editor)
- **Production Ready**: 8-10 months (AI integration + advanced features)

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file for details.

## Acknowledgments

- Inspired by Unreal Engine, Unity, and Godot architectures
- Built for the game "Threads of Kaliyuga"
- Thanks to the modern C++ community for best practices and design patterns

---

**Development Status**: Active development with solid foundation ✅  
**Next Milestone**: First triangle rendering 🎯  
**Contact**: [Your contact information]

**Note**: This engine is under active development. Core architecture is stable, but rendering APIs will evolve as implementation progresses.

**Note**: Copyright © 2025. Aditya Vennelakant. All rights reserved.