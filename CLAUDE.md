# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

### Primary Build Commands
- **Build entire solution**: `msbuild Akhanda.slnx /p:Configuration=Debug /p:Platform=x64`
- **Clean and rebuild**: `msbuild Akhanda.slnx /t:Clean,Build /p:Configuration=Debug /p:Platform=x64`
- **Build specific project**: `msbuild Engine/Engine.vcxproj /p:Configuration=Debug /p:Platform=x64`
- **Release build**: `msbuild Akhanda.slnx /p:Configuration=Release /p:Platform=x64`
- **Profile build**: `msbuild Akhanda.slnx /p:Configuration=Profile /p:Platform=x64`

### Test Commands
- **Run math tests**: `Build\Output\Bin\x64\Debug\Tests.Core.Math_Debug.exe`
- **Run all tests**: Navigate to test executable in `Build\Output\Bin\x64\Debug\` and run individual test executables

### Development Commands
- **Start Editor**: Press F5 in Visual Studio or run `Build\Output\Bin\x64\Debug\Editor_Debug.exe`
- **Start Game**: Run `Build\Output\Bin\x64\Debug\ThreadsOfKaliyuga_Debug.exe`

## High-Level Architecture

### Project Structure
Akhanda is a modular C++23 game engine with the following architecture:

```
Engine/               # Core engine (static library)
├── Core/            # Foundation systems
├── Renderer/        # D3D12 rendering system
├── Platform/        # Windows platform abstraction
└── Systems/         # Game systems (Physics, Audio, AI)

Plugins/             # Engine plugins (DLLs)
├── ImGuiPlugin/     # UI system
├── PhysXPlugin/     # Physics integration
└── ONNXPlugin/      # AI/ML integration

Editor/              # Standalone editor application
Game/                # Game project (ThreadsOfKaliyuga)
```

### Module System
The engine uses C++23 modules extensively. Key modules include:

- `Akhanda.Core.Math` - SIMD-optimized math library (Vector3, Matrix4, Quaternion)
- `Akhanda.Core.Memory` - Custom allocators with leak detection
- `Akhanda.Core.Threading` - Job system with work-stealing queues
- `Akhanda.Core.Logging` - Multi-threaded logging with editor integration
- `Akhanda.Engine.Renderer` - High-level renderer system
- `Akhanda.Engine.RHI` - D3D12 render hardware interface
- `Akhanda.Platform.Windows` - Windows platform abstraction

### Current Development Status
**Phase 2 - Basic Rendering (75% Complete)**
- ✅ Core foundation systems complete
- ✅ D3D12 infrastructure complete
- ✅ StandardRenderer implementation complete
- 🔄 Shader system in progress (HLSL compilation)
- 📋 Triangle rendering next milestone

### Build System Details
- Uses MSBuild with custom .props files for configuration
- Three build configurations: Debug, Release, Profile
- All projects use C++23 with modules enabled
- Output directory: `Build\Output\Bin\x64\$(Configuration)\`
- Intermediate files: `Build\Output\Intermediate\`

### Plugin Architecture
Plugins implement the `IPlugin` interface and export a `CreatePlugin()` function:
```cpp
class MyPlugin : public IPlugin {
    void OnLoad() override { /* initialization */ }
    void OnUnload() override { /* cleanup */ }
};

extern "C" __declspec(dllexport) IPlugin* CreatePlugin() {
    return new MyPlugin();
}
```

### Coding Conventions
- Modern C++23 with RAII and smart pointers
- Module interfaces (.ixx files) for public APIs
- Comprehensive error handling with custom Result<T> type
- SIMD-optimized math operations where applicable
- Memory tracking and leak detection in debug builds
- Multi-threaded design with job system integration

### Key Dependencies
- Windows SDK 10.0.22621.0+
- Visual Studio 2022 v17.5+
- D3D12 for rendering
- spdlog for logging
- DirectX 12 libraries

### Testing Strategy
- Unit tests for core systems (Math, Memory)
- Integration tests for renderer components
- Performance benchmarks for critical paths
- Test data located in `Tests/EngineTests/Tests.Core.Math/Data/`

### Development Workflow
1. Engine is built first as static library (Engine.lib)
2. Plugins are built as DLLs that link against Engine.lib
3. Applications (Editor, Game) link against Engine.lib and load plugins dynamically
4. All modules use consistent naming: Project.Module.Submodule format

### Important Configuration
- Preprocessor definitions per configuration:
  - Debug: `AKH_DEBUG`, `SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG`
  - Release: `AKH_RELEASE`, `SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN`
  - Profile: `AKH_PROFILE`, `SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG`
- Unicode character set throughout
- Multi-processor compilation enabled
- Warning level 4 with warnings as errors policy