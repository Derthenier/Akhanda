// Platform.Interfaces.ixx
// Akhanda Game Engine - Platform Abstraction Layer Interfaces
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

export module Akhanda.Platform.Interfaces;

import Akhanda.Core.Math;
import Akhanda.Core.Configuration.Rendering;

export namespace Akhanda::Platform {

    // ============================================================================
    // Forward Declarations
    // ============================================================================

    class IPlatformWindow;
    class IPlatformSurface;
    class IEventSystem;
    class IPlatformSystem;

    // ============================================================================
    // Event Type Definitions
    // ============================================================================

    enum class EventType : uint32_t {
        None = 0,

        // Window Events
        WindowCreated,
        WindowDestroyed,
        WindowResized,
        WindowMoved,
        WindowFocusGained,
        WindowFocusLost,
        WindowMinimized,
        WindowMaximized,
        WindowRestored,
        WindowCloseRequested,

        // Input Events
        KeyPressed,
        KeyReleased,
        KeyRepeated,
        MouseButtonPressed,
        MouseButtonReleased,
        MouseMoved,
        MouseScrolled,
        MouseEntered,
        MouseLeft,

        // System Events
        SystemShutdown,
        DisplayChanged,
        PowerStateChanged
    };

    // ============================================================================
    // Input Type Definitions
    // ============================================================================

    enum class KeyCode : uint32_t {
        Unknown = 0,

        // Letters
        A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        // Numbers
        Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

        // Function Keys
        F1 = 112, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

        // Special Keys
        Space = 32,
        Enter = 13,
        Escape = 27,
        Tab = 9,
        Backspace = 8,
        Delete = 46,
        Insert = 45,
        Home = 36,
        End = 35,
        PageUp = 33,
        PageDown = 34,

        // Arrow Keys
        Left = 37,
        Up = 38,
        Right = 39,
        Down = 40,

        // Modifier Keys
        LeftShift = 160,
        RightShift = 161,
        LeftControl = 162,
        RightControl = 163,
        LeftAlt = 164,
        RightAlt = 165,
        LeftWindows = 91,
        RightWindows = 92,

        // Numpad
        Numpad0 = 96, Numpad1, Numpad2, Numpad3, Numpad4, Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
        NumpadMultiply = 106,
        NumpadAdd = 107,
        NumpadSubtract = 109,
        NumpadDecimal = 110,
        NumpadDivide = 111
    };

    enum class MouseButton : uint32_t {
        Left = 0,
        Right = 1,
        Middle = 2,
        X1 = 3,
        X2 = 4
    };

    enum class KeyModifiers : uint32_t {
        None = 0,
        Shift = 1 << 0,
        Control = 1 << 1,
        Alt = 1 << 2,
        Super = 1 << 3  // Windows key
    };

    // Bitwise operators for KeyModifiers
    constexpr KeyModifiers operator|(KeyModifiers a, KeyModifiers b) noexcept {
        return static_cast<KeyModifiers>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    constexpr KeyModifiers operator&(KeyModifiers a, KeyModifiers b) noexcept {
        return static_cast<KeyModifiers>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    constexpr bool HasModifier(KeyModifiers modifiers, KeyModifiers check) noexcept {
        return (modifiers & check) == check;
    }

    // ============================================================================
    // Event Structure Definitions
    // ============================================================================

    struct Event {
        EventType type = EventType::None;
        std::chrono::steady_clock::time_point timestamp;
        bool handled = false;

        Event() : timestamp(std::chrono::steady_clock::now()) {}
        explicit Event(EventType eventType) : type(eventType), timestamp(std::chrono::steady_clock::now()) {}
        virtual ~Event() = default;
    };

    // Window Events
    struct WindowEvent : Event {
        IPlatformWindow* window = nullptr;

        inline WindowEvent(EventType type, IPlatformWindow* win)
            : Event(type), window(win) {
        }
    };

    struct WindowResizeEvent : WindowEvent {
        uint32_t width, height;

        inline WindowResizeEvent(IPlatformWindow* win, uint32_t w, uint32_t h)
            : WindowEvent(EventType::WindowResized, win), width(w), height(h) {
        }
    };

    struct WindowMoveEvent : WindowEvent {
        int32_t x, y;

        inline WindowMoveEvent(IPlatformWindow* win, int32_t posX, int32_t posY)
            : WindowEvent(EventType::WindowMoved, win), x(posX), y(posY) {
        }
    };

    // Input Events
    struct KeyEvent : Event {
        KeyCode key = KeyCode::Unknown;
        KeyModifiers modifiers = KeyModifiers::None;

        inline KeyEvent(EventType type, KeyCode keyCode, KeyModifiers mods)
            : Event(type), key(keyCode), modifiers(mods) {
        }
    };

    struct MouseButtonEvent : Event {
        MouseButton button = MouseButton::Left;
        int32_t x, y;
        KeyModifiers modifiers = KeyModifiers::None;

        inline MouseButtonEvent(EventType type, MouseButton btn, int32_t posX, int32_t posY, KeyModifiers mods)
            : Event(type), button(btn), x(posX), y(posY), modifiers(mods) {
        }
    };

    struct MouseMoveEvent : Event {
        int32_t x, y;
        int32_t deltaX, deltaY;

        inline MouseMoveEvent(int32_t posX, int32_t posY, int32_t dX, int32_t dY)
            : Event(EventType::MouseMoved), x(posX), y(posY), deltaX(dX), deltaY(dY) {
        }
    };

    struct MouseScrollEvent : Event {
        float deltaX, deltaY;
        int32_t x, y;  // Mouse position when scrolled

        inline MouseScrollEvent(float dX, float dY, int32_t posX, int32_t posY)
            : Event(EventType::MouseScrolled), deltaX(dX), deltaY(dY), x(posX), y(posY) {
        }
    };

    // ============================================================================
    // Window Properties and Settings
    // ============================================================================

    struct WindowProperties {
        std::string title = "Akhanda Engine";
        uint32_t width = 1280;
        uint32_t height = 720;
        int32_t x = -1;  // -1 means center
        int32_t y = -1;  // -1 means center
        bool fullscreen = false;
        bool resizable = true;
        bool visible = true;
        bool decorated = true;  // Title bar and borders
        bool focused = true;
        bool maximized = false;
        bool minimized = false;
        bool alwaysOnTop = false;
        bool vsync = false;

        // Additional properties
        uint32_t minWidth = 320;
        uint32_t minHeight = 240;
        uint32_t maxWidth = UINT32_MAX;
        uint32_t maxHeight = UINT32_MAX;

        // Icon path (optional)
        std::string iconPath;

        // Apply settings from configuration
        inline void ApplyFromConfig(const Configuration::RenderingConfig& config) {
            // Apply resolution settings
            const auto& resolution = config.GetResolution();
            width = resolution.width;
            height = resolution.height;

            // Apply fullscreen setting
            fullscreen = config.GetFullscreen();

            // Apply VSync setting
            vsync = config.GetVsync();

            // Ensure minimum window size constraints
            if (width < minWidth) width = minWidth;
            if (height < minHeight) height = minHeight;

            // Ensure maximum window size constraints
            if (width > maxWidth) width = maxWidth;
            if (height > maxHeight) height = maxHeight;

            // If fullscreen is enabled, ensure window is not resizable initially
            if (fullscreen) {
                resizable = false;
                decorated = false;  // Remove title bar in fullscreen
                alwaysOnTop = true; // Fullscreen windows should be on top
            }
        }
    };

    enum class WindowState : uint32_t {
        Normal = 0,
        Minimized,
        Maximized,
        Fullscreen,
        Hidden
    };

    enum class CursorMode : uint32_t {
        Normal = 0,
        Hidden,
        Disabled  // For FPS games
    };

    // ============================================================================
    // Surface Information for RHI Integration
    // ============================================================================

    struct SurfaceInfo {
        // Platform-specific surface data (opaque to RHI)
        void* nativeHandle = nullptr;
        void* nativeDisplay = nullptr;

        // Surface properties
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t format = 0;  // Platform-specific format
        bool supportsPresent = true;

        // Multi-sampling info
        uint32_t sampleCount = 1;
        uint32_t qualityLevels = 0;
    };

    // ============================================================================
    // Event System Interface
    // ============================================================================

    using EventCallback = std::function<void(const Event&)>;
    using EventSubscriberID = uint64_t;

    class IEventSystem {
    public:
        virtual ~IEventSystem() = default;

        // Event subscription management
        virtual EventSubscriberID Subscribe(EventType eventType, EventCallback callback) = 0;
        virtual void Unsubscribe(EventSubscriberID subscriberID) = 0;
        virtual void UnsubscribeAll(EventType eventType) = 0;

        // Event dispatching
        virtual void Dispatch(const Event& event) = 0;
        virtual void ProcessEvents() = 0;

        // Event filtering and priority
        virtual void SetEventFilter(EventType eventType, bool enabled) = 0;
        virtual bool IsEventFiltered(EventType eventType) const = 0;

        // Statistics and debugging
        virtual size_t GetSubscriberCount(EventType eventType) const = 0;
        virtual size_t GetTotalSubscriberCount() const = 0;
        virtual void ClearEventQueue() = 0;
    };

    // ============================================================================
    // Platform Surface Interface
    // ============================================================================

    class IPlatformSurface {
    public:
        virtual ~IPlatformSurface() = default;

        // Surface properties
        virtual SurfaceInfo GetSurfaceInfo() const = 0;
        virtual bool IsValid() const = 0;
        virtual void Invalidate() = 0;

        // Surface operations
        virtual bool Resize(uint32_t width, uint32_t height) = 0;
        virtual bool Present() = 0;
        virtual void WaitForPresent() = 0;

        // RHI integration
        virtual void* GetNativeHandle() const = 0;
        virtual void* GetNativeDisplay() const = 0;
        virtual uint32_t GetNativeFormat() const = 0;
    };

    // ============================================================================
    // Platform Window Interface
    // ============================================================================

    class IPlatformWindow {
    public:
        virtual ~IPlatformWindow() = default;

        // Window lifecycle
        virtual bool Initialize(const WindowProperties& properties) = 0;
        virtual void Destroy() = 0;
        virtual bool IsValid() const = 0;

        // Window properties
        virtual WindowProperties GetProperties() const = 0;
        virtual void SetTitle(const std::string& title) = 0;
        virtual void SetSize(uint32_t width, uint32_t height) = 0;
        virtual void SetPosition(int32_t x, int32_t y) = 0;
        virtual void SetVisible(bool visible) = 0;
        virtual void SetResizable(bool resizable) = 0;
        virtual void SetDecorated(bool decorated) = 0;
        virtual void SetAlwaysOnTop(bool alwaysOnTop) = 0;

        // Window state
        virtual WindowState GetState() const = 0;
        virtual void SetState(WindowState state) = 0;
        virtual void Minimize() = 0;
        virtual void Maximize() = 0;
        virtual void Restore() = 0;
        virtual void SetFullscreen(bool fullscreen) = 0;
        virtual void Focus() = 0;

        // Input handling
        virtual void SetCursorMode(CursorMode mode) = 0;
        virtual CursorMode GetCursorMode() const = 0;
        virtual Math::Vector2 GetCursorPosition() const = 0;
        virtual void SetCursorPosition(const Math::Vector2& position) = 0;

        // Surface access
        virtual std::shared_ptr<IPlatformSurface> GetSurface() const = 0;
        virtual void* GetNativeHandle() const = 0;

        // Event handling
        virtual void PollEvents() = 0;
        virtual bool ShouldClose() const = 0;
        virtual void SetShouldClose(bool shouldClose) = 0;

        // Utility
        virtual void SwapBuffers() = 0;
        virtual void WaitEvents() = 0;
        virtual double GetTime() const = 0;
    };

    // ============================================================================
    // Platform System Interface
    // ============================================================================

    class IPlatformSystem {
    public:
        virtual ~IPlatformSystem() = default;

        // System initialization
        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;
        virtual bool IsInitialized() const = 0;

        // Window management
        virtual std::shared_ptr<IPlatformWindow> CreateAWindow(const WindowProperties& properties) = 0;
        virtual void DestroyWindow(std::shared_ptr<IPlatformWindow> window) = 0;
        virtual std::vector<std::shared_ptr<IPlatformWindow>> GetWindows() const = 0;
        virtual std::shared_ptr<IPlatformWindow> GetMainWindow() const = 0;

        // Event system access
        virtual std::shared_ptr<IEventSystem> GetEventSystem() const = 0;

        // System information
        virtual std::string GetPlatformName() const = 0;
        virtual std::string GetPlatformVersion() const = 0;
        virtual uint32_t GetDisplayCount() const = 0;
        virtual Math::Vector2 GetDisplaySize(uint32_t displayIndex = 0) const = 0;
        virtual Math::Vector2 GetDisplayDPI(uint32_t displayIndex = 0) const = 0;

        // System events
        virtual void PollSystemEvents() = 0;
        virtual void WaitForEvents() = 0;
        virtual void PostEmptyEvent() = 0;  // Wake up event waiting

        // Utility
        virtual double GetTime() const = 0;
        virtual void Sleep(double seconds) = 0;
        virtual std::string GetClipboardText() const = 0;
        virtual void SetClipboardText(const std::string& text) = 0;
    };

    // ============================================================================
    // Platform Factory Interface
    // ============================================================================

    class IPlatformFactory {
    public:
        virtual ~IPlatformFactory() = default;
        virtual std::unique_ptr<IPlatformSystem> CreatePlatformSystem() = 0;
        virtual std::string GetPlatformName() const = 0;
        virtual bool IsSupported() const = 0;
    };

    // ============================================================================
    // Utility Functions
    // ============================================================================

    // Convert event type to string for debugging
    inline std::string EventTypeToString(EventType type) {
        switch (type) {
        case EventType::WindowCreated: return "WindowCreated";
        case EventType::WindowDestroyed: return "WindowDestroyed";
        case EventType::WindowResized: return "WindowResized";
        case EventType::WindowMoved: return "WindowMoved";
        case EventType::WindowFocusGained: return "WindowFocusGained";
        case EventType::WindowFocusLost: return "WindowFocusLost";
        case EventType::WindowMinimized: return "WindowMinimized";
        case EventType::WindowMaximized: return "WindowMaximized";
        case EventType::WindowRestored: return "WindowRestored";
        case EventType::WindowCloseRequested: return "WindowCloseRequested";
        case EventType::KeyPressed: return "KeyPressed";
        case EventType::KeyReleased: return "KeyReleased";
        case EventType::KeyRepeated: return "KeyRepeated";
        case EventType::MouseButtonPressed: return "MouseButtonPressed";
        case EventType::MouseButtonReleased: return "MouseButtonReleased";
        case EventType::MouseMoved: return "MouseMoved";
        case EventType::MouseScrolled: return "MouseScrolled";
        case EventType::MouseEntered: return "MouseEntered";
        case EventType::MouseLeft: return "MouseLeft";
        case EventType::SystemShutdown: return "SystemShutdown";
        case EventType::DisplayChanged: return "DisplayChanged";
        case EventType::PowerStateChanged: return "PowerStateChanged";
        default: return "Unknown";
        }
    }

    // Convert key code to string for debugging
    inline std::string KeyCodeToString(KeyCode key) {
        if (key >= KeyCode::A && key <= KeyCode::Z) {
            return std::string(1, static_cast<char>('A' + (static_cast<uint32_t>(key) - static_cast<uint32_t>(KeyCode::A))));
        }
        if (key >= KeyCode::Num0 && key <= KeyCode::Num9) {
            return std::string(1, static_cast<char>('0' + (static_cast<uint32_t>(key) - static_cast<uint32_t>(KeyCode::Num0))));
        }
        if (key >= KeyCode::F1 && key <= KeyCode::F12) {
            return std::format("F{}", static_cast<uint32_t>(key) - static_cast<uint32_t>(KeyCode::F1) + 1);
        }

        switch (key) {
        case KeyCode::Space: return "Space";
        case KeyCode::Enter: return "Enter";
        case KeyCode::Escape: return "Escape";
        case KeyCode::Tab: return "Tab";
        case KeyCode::Backspace: return "Backspace";
        case KeyCode::Delete: return "Delete";
        case KeyCode::Insert: return "Insert";
        case KeyCode::Home: return "Home";
        case KeyCode::End: return "End";
        case KeyCode::PageUp: return "PageUp";
        case KeyCode::PageDown: return "PageDown";
        case KeyCode::Left: return "Left";
        case KeyCode::Up: return "Up";
        case KeyCode::Right: return "Right";
        case KeyCode::Down: return "Down";
        case KeyCode::LeftShift: return "LeftShift";
        case KeyCode::RightShift: return "RightShift";
        case KeyCode::LeftControl: return "LeftControl";
        case KeyCode::RightControl: return "RightControl";
        case KeyCode::LeftAlt: return "LeftAlt";
        case KeyCode::RightAlt: return "RightAlt";
        case KeyCode::LeftWindows: return "LeftWindows";
        case KeyCode::RightWindows: return "RightWindows";
        default: return "Unknown";
        }
    }

} // namespace Akhanda::Platform