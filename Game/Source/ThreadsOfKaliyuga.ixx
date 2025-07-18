// Game/Source/Main.cpp
// ThreadsOfKaliyuga Game - Platform Integration Demo
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.
module;

#include <format>
#include <chrono>

export module ThreadsOfKaliyuga.Game;

// Engine module imports
import Akhanda.Platform.Interfaces;
import Akhanda.Platform.Windows;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Core.Configuration.Manager;
import Akhanda.Core.Math;

import ThreadsOfKaliyuga.Logging;

using namespace Akhanda;
using namespace Akhanda::Platform;
using namespace Akhanda::Configuration;
using namespace ThreadsOfKaliyuga::Logging;

export namespace ThreadsOfKaliyuga {


    // ============================================================================
    // Performance Profiler for Debug
    // ============================================================================

    class FrameProfiler {
    private:
        std::chrono::high_resolution_clock::time_point frameStart_;
        std::chrono::high_resolution_clock::time_point lastProfileTime_;
        double eventTime_ = 0.0;
        double updateTime_ = 0.0;
        double renderTime_ = 0.0;
        uint32_t profileFrameCount_ = 0;

        constexpr static double TARGET_FRAME_TIME_MS = 1000.0 / 60.0; // 60 FPS target

    public:
        void BeginFrame() {
            frameStart_ = std::chrono::high_resolution_clock::now();
        }

        void BeginEvents() {
            lastProfileTime_ = std::chrono::high_resolution_clock::now();
        }

        void EndEvents() {
            auto now = std::chrono::high_resolution_clock::now();
            eventTime_ += std::chrono::duration<double, std::milli>(now - lastProfileTime_).count();
        }

        void BeginUpdate() {
            lastProfileTime_ = std::chrono::high_resolution_clock::now();
        }

        void EndUpdate() {
            auto now = std::chrono::high_resolution_clock::now();
            updateTime_ += std::chrono::duration<double, std::milli>(now - lastProfileTime_).count();
        }

        void BeginRender() {
            lastProfileTime_ = std::chrono::high_resolution_clock::now();
        }

        void EndRender() {
            auto now = std::chrono::high_resolution_clock::now();
            renderTime_ += std::chrono::duration<double, std::milli>(now - lastProfileTime_).count();
            profileFrameCount_++;
        }

        void PrintProfile() {
            if (profileFrameCount_ > 0) {
                double avgEvent = eventTime_ / profileFrameCount_;
                double avgUpdate = updateTime_ / profileFrameCount_;
                double avgRender = renderTime_ / profileFrameCount_;
                double avgTotal = avgEvent + avgUpdate + avgRender;

                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Performance Profile (avg per frame):");
                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  Events:  {:.2f}ms", avgEvent);
                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  Update:  {:.2f}ms", avgUpdate);
                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  Render:  {:.2f}ms", avgRender);
                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  Total:   {:.2f}ms", avgTotal);
                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  Target:  {:.2f}ms (60 FPS)", TARGET_FRAME_TIME_MS);

                // Reset counters
                eventTime_ = updateTime_ = renderTime_ = 0.0;
                profileFrameCount_ = 0;
            }
        }
    };
    
    
    // ============================================================================
    // Game Application Class
    // ============================================================================

    class Game {
    private:
        std::unique_ptr<IPlatformSystem> platformSystem_;
        std::shared_ptr<IPlatformWindow> mainWindow_;
        std::shared_ptr<IEventSystem> eventSystem_;
        bool isRunning_;
        double lastFrameTime_;
        uint32_t frameCount_;

        // Performance monitoring
        FrameProfiler profiler_;
        bool enableProfiling_;
        double lastProfilePrint_;

    public:
        Game()
            : isRunning_(false)
            , lastFrameTime_(0.0)
            , frameCount_(0)
            , enableProfiling_(true)
            , lastProfilePrint_(0.0) {
        }

        ~Game() {
            Shutdown();
        }

        bool Initialize() {
            ToKLogging::Initialize();

            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "=== ThreadsOfKaliyuga Game - Platform Demo ===");

            // Initialize configuration system
            if (!InitializeConfiguration()) {
                ToKLogging::Log(Akhanda::Logging::LogLevel::Error, "ERROR: Failed to initialize configuration system");
                return false;
            }

            // Create platform system
            auto platformFactory = CreateWindowsPlatformFactory();
            if (!platformFactory) {
                ToKLogging::Log(Akhanda::Logging::LogLevel::Error, "ERROR: Failed to create platform factory");
                return false;
            }

            platformSystem_ = platformFactory->CreatePlatformSystem();
            if (!platformSystem_) {
                ToKLogging::Log(Akhanda::Logging::LogLevel::Error, "ERROR: Failed to create platform system");
                return false;
            }

            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Platform: {} {}", platformSystem_->GetPlatformName(), platformSystem_->GetPlatformVersion());

            // Initialize platform system
            if (!platformSystem_->Initialize()) {
                ToKLogging::Log(Akhanda::Logging::LogLevel::Error, "ERROR: Failed to initialize platform system");
                return false;
            }

            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Platform system initialized successfully");

            // Get event system
            eventSystem_ = platformSystem_->GetEventSystem();
            if (!eventSystem_) {
                ToKLogging::Log(Akhanda::Logging::LogLevel::Error, "ERROR: Failed to get event system");
                return false;
            }

            // Setup event handlers
            SetupEventHandlers();

            // Create main window
            if (!CreateMainWindow()) {
                ToKLogging::Log(Akhanda::Logging::LogLevel::Error, "ERROR: Failed to create main window");
                return false;
            }

            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Game initialized successfully!");

            if (enableProfiling_) {
                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Performance Mode: Profiling Enabled");
            }
            else {
                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Performance Mode: Profiling Disabled");
            }
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Controls:");
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  - ESC: Exit");
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  - F11: Toggle fullscreen");
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  - Alt+Enter: Toggle fullscreen (alternative)");
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  - Mouse: Move and click to test input");
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  - F2: Toggle profiling");

            isRunning_ = true;
            return true;
        }

        void Run() {
            if (!isRunning_) {
                ToKLogging::Log(Akhanda::Logging::LogLevel::Error, "Game not initialized!");
                return;
            }

            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Starting game loop...");


            lastFrameTime_ = platformSystem_->GetTime();
            lastProfilePrint_ = lastFrameTime_;

            // Main game loop
            while (isRunning_ && !mainWindow_->ShouldClose()) {
                if (enableProfiling_) {
                    profiler_.BeginFrame();
                }

                double currentTime = platformSystem_->GetTime();
                double deltaTime = currentTime - lastFrameTime_;
                lastFrameTime_ = currentTime;

                // Process platform events (with profiling)
                if (enableProfiling_) profiler_.BeginEvents();

                // Process platform events
                mainWindow_->PollEvents();
                platformSystem_->PollSystemEvents();

                if (enableProfiling_) profiler_.EndEvents();

                // Update game logic (with profiling)
                if (enableProfiling_) profiler_.BeginUpdate();

                // Update game logic
                Update(deltaTime);

                if (enableProfiling_) profiler_.EndUpdate();

                // Render frame (with profiling)
                if (enableProfiling_) profiler_.BeginRender();

                // Render frame
                Render();

                if (enableProfiling_) profiler_.EndRender();

                frameCount_++;

                // Print performance profile every 5 seconds
                if (enableProfiling_ && (currentTime - lastProfilePrint_) >= 5.0) {
                    profiler_.PrintProfile();
                    lastProfilePrint_ = currentTime;
                }
            }

            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Game loop ended. Frames rendered: {}", frameCount_);
        }

        void Shutdown() {
            if (platformSystem_) {
                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Shutting down game...");

                if (mainWindow_) {
                    mainWindow_.reset();
                }

                platformSystem_->Shutdown();
                platformSystem_.reset();

                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Game shutdown complete.");
            }

            ToKLogging::Shutdown();
        }

    private:
        bool InitializeConfiguration() {
            // Initialize configuration manager
            if (!ConfigManager::Instance().Initialize()) {
                return false;
            }

            // Register configuration sections
            RegisterRenderingConfig();

            // Load default configuration
            auto renderingConfig = GetRenderingConfig();
            if (!renderingConfig) {
                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "ERROR: Failed to get rendering configuration");
                return false;
            }

            // Set some game-specific defaults
            renderingConfig->SetResolution({ 1280, 720 });
            renderingConfig->SetFullscreen(false);
            renderingConfig->SetVsync(true);
            renderingConfig->SetRenderingAPI(RenderingAPI::D3D12);

            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Configuration initialized:");
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  Resolution: {}x{} ", renderingConfig->GetResolution().width, renderingConfig->GetResolution().height);
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  Fullscreen: {}", (renderingConfig->GetFullscreen() ? "Yes" : "No"));
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  VSync: {}", (renderingConfig->GetVsync() ? "Yes" : "No"));
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  Rendering API: {}", RenderingAPIToString(renderingConfig->GetRenderingAPI()));

            return true;
        }

        bool CreateMainWindow() {
            auto renderingConfig = GetRenderingConfig();
            if (!renderingConfig) {
                return false;
            }

            // Setup window properties
            WindowProperties windowProps;
            windowProps.title = "Threads of Kaliyuga - Game Window";
            windowProps.ApplyFromConfig(*renderingConfig);

            // Additional game-specific properties
            windowProps.resizable = true;
            windowProps.visible = true;
            windowProps.focused = true;
            windowProps.iconPath = ""; // TODO: Add game icon

            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Creating main window: {} x {}", windowProps.width, windowProps.height);

            // Create the window
            mainWindow_ = platformSystem_->CreateAWindow(windowProps);
            if (!mainWindow_) {
                return false;
            }

            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Main window created successfully");

            // Get display information
            uint32_t displayCount = platformSystem_->GetDisplayCount();
            auto displaySize = platformSystem_->GetDisplaySize(0);
            auto displayDPI = platformSystem_->GetDisplayDPI(0);

            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Display Information:");
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  Display count: {}", displayCount);
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  Primary display size: {} x {}", displaySize.x, displaySize.y);
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "  Primary display DPI: {} x {}", displayDPI.x, displayDPI.y);

            return true;
        }

        void SetupEventHandlers() {
            // Window events
            eventSystem_->Subscribe(EventType::WindowCloseRequested,
                [this](const Event& event) {
                    ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Window close requested");
                    isRunning_ = false;
                });

            eventSystem_->Subscribe(EventType::WindowResized,
                [this](const Event& event) {
                    const auto& resizeEvent = static_cast<const WindowResizeEvent&>(event);
                    
                    // Only log significant resize events to avoid spam
                    static uint32_t lastWidth = 0, lastHeight = 0;
                    if (Math::Abs(static_cast<int>(resizeEvent.width - lastWidth)) > 10 || Math::Abs(static_cast<int>(resizeEvent.height - lastHeight)) > 10) {
                        ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Window resized: {}x{}", resizeEvent.width, resizeEvent.height);
                        lastWidth = resizeEvent.width;
                        lastHeight = resizeEvent.height;
                    }
                });

            eventSystem_->Subscribe(EventType::WindowFocusGained,
                [this](const Event& event) {
                    ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Window gained focus");
                });

            eventSystem_->Subscribe(EventType::WindowFocusLost,
                [this](const Event& event) {
                    ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Window lost focus");
                });

            // Key events
            eventSystem_->Subscribe(EventType::KeyPressed,
                [this](const Event& event) {
                    const auto& keyEvent = static_cast<const KeyEvent&>(event);
                    OnKeyPressed(keyEvent.key, keyEvent.modifiers);
                });

            eventSystem_->Subscribe(EventType::KeyReleased,
                [this](const Event& event) {
                    const auto& keyEvent = static_cast<const KeyEvent&>(event);
                    OnKeyReleased(keyEvent.key, keyEvent.modifiers);
                });

            // Mouse events
            eventSystem_->Subscribe(EventType::MouseButtonPressed,
                [this](const Event& event) {
                    const auto& mouseEvent = static_cast<const MouseButtonEvent&>(event);
                    OnMouseButtonPressed(mouseEvent.button, mouseEvent.x, mouseEvent.y, mouseEvent.modifiers);
                });

            eventSystem_->Subscribe(EventType::MouseMoved,
                [this](const Event& event) {
                    const auto& mouseEvent = static_cast<const MouseMoveEvent&>(event);
                    OnMouseMoved(mouseEvent.x, mouseEvent.y, mouseEvent.deltaX, mouseEvent.deltaY);
                });

            eventSystem_->Subscribe(EventType::MouseScrolled,
                [this](const Event& event) {
                    const auto& scrollEvent = static_cast<const MouseScrollEvent&>(event);
                    OnMouseScrolled(scrollEvent.deltaX, scrollEvent.deltaY);
                });

            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Event handlers registered");
        }

        void OnKeyPressed(KeyCode key, KeyModifiers modifiers) {
            std::string keyName = KeyCodeToString(key);
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Key pressed: {} ", keyName);

            if (HasModifier(modifiers, KeyModifiers::Control)) ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Ctrl+");
            if (HasModifier(modifiers, KeyModifiers::Alt)) ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Alt+");
            if (HasModifier(modifiers, KeyModifiers::Shift)) ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Shift+");
            if (HasModifier(modifiers, KeyModifiers::Super)) ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Win+");

            // Handle special keys
            switch (key) {
            case KeyCode::Escape:
                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "ESC pressed - Exiting game");
                isRunning_ = false;
                break;

            case KeyCode::F11:
                ToggleFullscreen();
                break;

            case KeyCode::Enter:
                if (HasModifier(modifiers, KeyModifiers::Alt)) {
                    ToggleFullscreen();
                }
                break;

            case KeyCode::F1:
                if (enableProfiling_) {
                    profiler_.PrintProfile();
                }
                else {
                    ShowHelp();
                }
                break;

            default:
                break;
            }
        }

        void OnKeyReleased(KeyCode key, KeyModifiers modifiers) {
            // Only log important key releases to avoid spam
            if (key == KeyCode::Space || key == KeyCode::Enter) {
                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Key released: {}", KeyCodeToString(key));
            }
        }

        void OnMouseButtonPressed(MouseButton button, int32_t x, int32_t y, KeyModifiers modifiers) {
            std::string buttonName;
            switch (button) {
            case MouseButton::Left: buttonName = "Left"; break;
            case MouseButton::Right: buttonName = "Right"; break;
            case MouseButton::Middle: buttonName = "Middle"; break;
            case MouseButton::X1: buttonName = "X1"; break;
            case MouseButton::X2: buttonName = "X2"; break;
            }

            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Mouse {} button pressed at ({}, {})", buttonName, x, y);
        }

        void OnMouseMoved(int32_t x, int32_t y, int32_t deltaX, int32_t deltaY) {
            // Only log mouse movement if there's significant delta to avoid spam
            if (Math::Abs(deltaX) > 5 || Math::Abs(deltaY) > 5) {
                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Mouse moved to ({}, {}) delta: ({}, {})", x, y, deltaX, deltaY);
            }
        }

        void OnMouseScrolled(float deltaX, float deltaY) {
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Mouse scrolled: X={:.2f}, Y={:.2f}", deltaX, deltaY);
        }

        void ToggleFullscreen() {
            if (!mainWindow_) return;

            auto properties = mainWindow_->GetProperties();
            bool isFullscreen = properties.fullscreen;

            if (isFullscreen) {
                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Switching to windowed mode");
            } else {
                ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Switching to fullscreen mode");
            }

            mainWindow_->SetFullscreen(!isFullscreen);
        }

        void ShowHelp() {
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "=== ThreadsOfKaliyuga Controls ===");
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "ESC          - Exit game");
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "F1           - Show this help");
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "F11          - Toggle fullscreen");
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Alt+Enter    - Toggle fullscreen");
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Mouse        - Move and click to test input");
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Mouse Wheel  - Scroll to test scroll input");
            ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "=================================");
        }

        void Update(double deltaTime) {
            // Optimized update logic
            static double accumulatedTime = 0.0;
            static double lastFPSUpdate = 0.0;
            accumulatedTime += deltaTime;

            // Update FPS display less frequently for performance
            if (accumulatedTime - lastFPSUpdate >= 0.5) {  // Update every 0.5 seconds instead of every frame
                double fps = frameCount_ / accumulatedTime;

                // Only print FPS every 2 seconds to reduce console spam
                static double lastFPSPrint = 0.0;
                if (accumulatedTime - lastFPSPrint >= 2.0) {
                    ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "FPS: {:.1f}", fps);
                    lastFPSPrint = accumulatedTime;
                }

                // Update window title less frequently
                std::string title = std::format("Threads of Kaliyuga - FPS: {:.1f}", fps);
                mainWindow_->SetTitle(title);
            
                lastFPSUpdate = accumulatedTime;
            }

            // Reset counters every 10 seconds to prevent overflow
            if (accumulatedTime >= 10.0) {
                accumulatedTime = 0.0;
                frameCount_ = 0;
            }
        }

        void Render() {
            // Basic rendering - just swap buffers for now
            // This will be enhanced when we add the RHI layer
            if (mainWindow_) {
                mainWindow_->SwapBuffers();
            }
        }
    };
}