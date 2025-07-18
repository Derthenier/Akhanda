// Platform.Windows.ixx
// Akhanda Game Engine - Windows Platform Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <mmsystem.h>

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <chrono>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "winmm.lib")

export module Akhanda.Platform.Windows;

import Akhanda.Platform.Interfaces;
import Akhanda.Core.Math;
import Akhanda.Core.Configuration.Rendering;
import std;


// ============================================================================
// Windows-Specific Constants and Utilities
// ============================================================================

namespace {
    constexpr const wchar_t* WINDOW_CLASS_NAME = L"AkhandaWindowClass";
    constexpr DWORD WINDOWED_STYLE = WS_OVERLAPPEDWINDOW;
    constexpr DWORD WINDOWED_EX_STYLE = WS_EX_APPWINDOW;
    constexpr DWORD FULLSCREEN_STYLE = WS_POPUP;
    constexpr DWORD FULLSCREEN_EX_STYLE = WS_EX_APPWINDOW | WS_EX_TOPMOST;

    // Convert UTF-8 to wide string
    std::wstring Utf8ToWide(const std::string& utf8) {
        if (utf8.empty()) return {};

        int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        std::wstring result(size - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, result.data(), size);
        return result;
    }

    // Convert wide string to UTF-8
    std::string WideToUtf8(const std::wstring& wide) {
        if (wide.empty()) return {};

        int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string result(size - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, result.data(), size, nullptr, nullptr);
        return result;
    }

    // Key code mapping from Windows virtual keys to Akhanda key codes
    Akhanda::Platform::KeyCode VirtualKeyToKeyCode(WPARAM vkey) {
        switch (vkey) {
            // Letters
        case 'A': return Akhanda::Platform::KeyCode::A;
        case 'B': return Akhanda::Platform::KeyCode::B;
        case 'C': return Akhanda::Platform::KeyCode::C;
        case 'D': return Akhanda::Platform::KeyCode::D;
        case 'E': return Akhanda::Platform::KeyCode::E;
        case 'F': return Akhanda::Platform::KeyCode::F;
        case 'G': return Akhanda::Platform::KeyCode::G;
        case 'H': return Akhanda::Platform::KeyCode::H;
        case 'I': return Akhanda::Platform::KeyCode::I;
        case 'J': return Akhanda::Platform::KeyCode::J;
        case 'K': return Akhanda::Platform::KeyCode::K;
        case 'L': return Akhanda::Platform::KeyCode::L;
        case 'M': return Akhanda::Platform::KeyCode::M;
        case 'N': return Akhanda::Platform::KeyCode::N;
        case 'O': return Akhanda::Platform::KeyCode::O;
        case 'P': return Akhanda::Platform::KeyCode::P;
        case 'Q': return Akhanda::Platform::KeyCode::Q;
        case 'R': return Akhanda::Platform::KeyCode::R;
        case 'S': return Akhanda::Platform::KeyCode::S;
        case 'T': return Akhanda::Platform::KeyCode::T;
        case 'U': return Akhanda::Platform::KeyCode::U;
        case 'V': return Akhanda::Platform::KeyCode::V;
        case 'W': return Akhanda::Platform::KeyCode::W;
        case 'X': return Akhanda::Platform::KeyCode::X;
        case 'Y': return Akhanda::Platform::KeyCode::Y;
        case 'Z': return Akhanda::Platform::KeyCode::Z;

            // Numbers
        case '0': return Akhanda::Platform::KeyCode::Num0;
        case '1': return Akhanda::Platform::KeyCode::Num1;
        case '2': return Akhanda::Platform::KeyCode::Num2;
        case '3': return Akhanda::Platform::KeyCode::Num3;
        case '4': return Akhanda::Platform::KeyCode::Num4;
        case '5': return Akhanda::Platform::KeyCode::Num5;
        case '6': return Akhanda::Platform::KeyCode::Num6;
        case '7': return Akhanda::Platform::KeyCode::Num7;
        case '8': return Akhanda::Platform::KeyCode::Num8;
        case '9': return Akhanda::Platform::KeyCode::Num9;

            // Function keys
        case VK_F1: return Akhanda::Platform::KeyCode::F1;
        case VK_F2: return Akhanda::Platform::KeyCode::F2;
        case VK_F3: return Akhanda::Platform::KeyCode::F3;
        case VK_F4: return Akhanda::Platform::KeyCode::F4;
        case VK_F5: return Akhanda::Platform::KeyCode::F5;
        case VK_F6: return Akhanda::Platform::KeyCode::F6;
        case VK_F7: return Akhanda::Platform::KeyCode::F7;
        case VK_F8: return Akhanda::Platform::KeyCode::F8;
        case VK_F9: return Akhanda::Platform::KeyCode::F9;
        case VK_F10: return Akhanda::Platform::KeyCode::F10;
        case VK_F11: return Akhanda::Platform::KeyCode::F11;
        case VK_F12: return Akhanda::Platform::KeyCode::F12;

            // Special keys
        case VK_SPACE: return Akhanda::Platform::KeyCode::Space;
        case VK_RETURN: return Akhanda::Platform::KeyCode::Enter;
        case VK_ESCAPE: return Akhanda::Platform::KeyCode::Escape;
        case VK_TAB: return Akhanda::Platform::KeyCode::Tab;
        case VK_BACK: return Akhanda::Platform::KeyCode::Backspace;
        case VK_DELETE: return Akhanda::Platform::KeyCode::Delete;
        case VK_INSERT: return Akhanda::Platform::KeyCode::Insert;
        case VK_HOME: return Akhanda::Platform::KeyCode::Home;
        case VK_END: return Akhanda::Platform::KeyCode::End;
        case VK_PRIOR: return Akhanda::Platform::KeyCode::PageUp;
        case VK_NEXT: return Akhanda::Platform::KeyCode::PageDown;

            // Arrow keys
        case VK_LEFT: return Akhanda::Platform::KeyCode::Left;
        case VK_UP: return Akhanda::Platform::KeyCode::Up;
        case VK_RIGHT: return Akhanda::Platform::KeyCode::Right;
        case VK_DOWN: return Akhanda::Platform::KeyCode::Down;

            // Modifier keys
        case VK_LSHIFT: return Akhanda::Platform::KeyCode::LeftShift;
        case VK_RSHIFT: return Akhanda::Platform::KeyCode::RightShift;
        case VK_LCONTROL: return Akhanda::Platform::KeyCode::LeftControl;
        case VK_RCONTROL: return Akhanda::Platform::KeyCode::RightControl;
        case VK_LMENU: return Akhanda::Platform::KeyCode::LeftAlt;
        case VK_RMENU: return Akhanda::Platform::KeyCode::RightAlt;
        case VK_LWIN: return Akhanda::Platform::KeyCode::LeftWindows;
        case VK_RWIN: return Akhanda::Platform::KeyCode::RightWindows;

            // Numpad
        case VK_NUMPAD0: return Akhanda::Platform::KeyCode::Numpad0;
        case VK_NUMPAD1: return Akhanda::Platform::KeyCode::Numpad1;
        case VK_NUMPAD2: return Akhanda::Platform::KeyCode::Numpad2;
        case VK_NUMPAD3: return Akhanda::Platform::KeyCode::Numpad3;
        case VK_NUMPAD4: return Akhanda::Platform::KeyCode::Numpad4;
        case VK_NUMPAD5: return Akhanda::Platform::KeyCode::Numpad5;
        case VK_NUMPAD6: return Akhanda::Platform::KeyCode::Numpad6;
        case VK_NUMPAD7: return Akhanda::Platform::KeyCode::Numpad7;
        case VK_NUMPAD8: return Akhanda::Platform::KeyCode::Numpad8;
        case VK_NUMPAD9: return Akhanda::Platform::KeyCode::Numpad9;
        case VK_MULTIPLY: return Akhanda::Platform::KeyCode::NumpadMultiply;
        case VK_ADD: return Akhanda::Platform::KeyCode::NumpadAdd;
        case VK_SUBTRACT: return Akhanda::Platform::KeyCode::NumpadSubtract;
        case VK_DECIMAL: return Akhanda::Platform::KeyCode::NumpadDecimal;
        case VK_DIVIDE: return Akhanda::Platform::KeyCode::NumpadDivide;

        default: return Akhanda::Platform::KeyCode::Unknown;
        }
    }

    // Get current key modifiers
    Akhanda::Platform::KeyModifiers GetCurrentModifiers() {
        Akhanda::Platform::KeyModifiers modifiers = Akhanda::Platform::KeyModifiers::None;

        if (GetKeyState(VK_SHIFT) & 0x8000)
            modifiers = modifiers | Akhanda::Platform::KeyModifiers::Shift;
        if (GetKeyState(VK_CONTROL) & 0x8000)
            modifiers = modifiers | Akhanda::Platform::KeyModifiers::Control;
        if (GetKeyState(VK_MENU) & 0x8000)
            modifiers = modifiers | Akhanda::Platform::KeyModifiers::Alt;
        if ((GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000))
            modifiers = modifiers | Akhanda::Platform::KeyModifiers::Super;

        return modifiers;
    }

    // Convert Windows mouse button to Akhanda mouse button
    Akhanda::Platform::MouseButton WindowsButtonToMouseButton(WPARAM wParam) {
        if (wParam & MK_LBUTTON) return Akhanda::Platform::MouseButton::Left;
        if (wParam & MK_RBUTTON) return Akhanda::Platform::MouseButton::Right;
        if (wParam & MK_MBUTTON) return Akhanda::Platform::MouseButton::Middle;
        if (wParam & MK_XBUTTON1) return Akhanda::Platform::MouseButton::X1;
        if (wParam & MK_XBUTTON2) return Akhanda::Platform::MouseButton::X2;
        return Akhanda::Platform::MouseButton::Left; // Default fallback
    }
}


export namespace Akhanda::Platform::Windows {

    // ============================================================================
    // Forward Declarations
    // ============================================================================

    class WindowsEventSystem;
    class WindowsPlatformWindow;
    class WindowsPlatformSurface;
    class WindowsPlatformSystem;

    // ============================================================================
    // Windows Event System Implementation
    // ============================================================================

    class WindowsEventSystem : public IEventSystem {
    private:
        struct EventSubscription {
            EventSubscriberID id;
            EventCallback callback;
        };

        std::unordered_map<EventType, std::vector<EventSubscription>> subscribers_;
        std::unordered_map<EventType, bool> eventFilters_;
        std::atomic<EventSubscriberID> nextSubscriberID_{ 1 };
        mutable std::mutex subscriberMutex_;

        std::queue<std::unique_ptr<Event>> eventQueue_;
        mutable std::mutex eventQueueMutex_;

    public:
        WindowsEventSystem() {
            // Initialize all event filters as enabled
            for (uint32_t i = 0; i <= static_cast<uint32_t>(EventType::PowerStateChanged); ++i) {
                eventFilters_[static_cast<EventType>(i)] = true;
            }
        }

        ~WindowsEventSystem() override = default;

        EventSubscriberID Subscribe(EventType eventType, EventCallback callback) override {
            std::lock_guard<std::mutex> lock(subscriberMutex_);

            EventSubscriberID id = nextSubscriberID_++;
            subscribers_[eventType].push_back({ id, std::move(callback) });

            return id;
        }

        void Unsubscribe(EventSubscriberID subscriberID) override {
            std::lock_guard<std::mutex> lock(subscriberMutex_);

            for (auto& [eventType, subscriptions] : subscribers_) {
                auto it = std::remove_if(subscriptions.begin(), subscriptions.end(),
                    [subscriberID](const EventSubscription& sub) {
                        return sub.id == subscriberID;
                    });
                subscriptions.erase(it, subscriptions.end());
            }
        }

        void UnsubscribeAll(EventType eventType) override {
            std::lock_guard<std::mutex> lock(subscriberMutex_);
            subscribers_[eventType].clear();
        }

        void Dispatch(const Event& event) override {
            if (!IsEventFiltered(event.type)) {
                return; // Event is filtered out
            }

            std::lock_guard<std::mutex> lock(subscriberMutex_);

            auto it = subscribers_.find(event.type);
            if (it != subscribers_.end()) {
                for (const auto& subscription : it->second) {
                    try {
                        subscription.callback(event);
                        if (event.handled) {
                            break; // Stop processing if event was handled
                        }
                    }
                    catch (const std::exception& e) {
                        // Log error but continue processing other subscribers
                        // TODO: Integrate with logging system when available
                        OutputDebugStringA(std::format("Event callback exception: {}\n", e.what()).c_str());
                    }
                }
            }
        }

        void ProcessEvents() override {
            std::queue<std::unique_ptr<Event>> eventsToProcess;

            // Move events from queue to local queue to minimize lock time
            {
                std::lock_guard<std::mutex> lock(eventQueueMutex_);
                eventsToProcess = std::move(eventQueue_);
                // eventQueue_ is now empty
            }

            // Process events without holding the queue lock
            while (!eventsToProcess.empty()) {
                auto event = std::move(eventsToProcess.front());
                eventsToProcess.pop();
                Dispatch(*event);
            }
        }

        void SetEventFilter(EventType eventType, bool enabled) override {
            std::lock_guard<std::mutex> lock(subscriberMutex_);
            eventFilters_[eventType] = enabled;
        }

        bool IsEventFiltered(EventType eventType) const override {
            std::lock_guard<std::mutex> lock(subscriberMutex_);
            auto it = eventFilters_.find(eventType);
            return it != eventFilters_.end() ? it->second : true;
        }

        size_t GetSubscriberCount(EventType eventType) const override {
            std::lock_guard<std::mutex> lock(subscriberMutex_);
            auto it = subscribers_.find(eventType);
            return it != subscribers_.end() ? it->second.size() : 0;
        }

        size_t GetTotalSubscriberCount() const override {
            std::lock_guard<std::mutex> lock(subscriberMutex_);
            size_t total = 0;
            for (const auto& [eventType, subscriptions] : subscribers_) {
                total += subscriptions.size();
            }
            return total;
        }

        void ClearEventQueue() override {
            std::lock_guard<std::mutex> lock(eventQueueMutex_);
            while (!eventQueue_.empty()) {
                eventQueue_.pop();
            }
        }

        // Windows-specific method to queue events from window procedure
        void QueueEvent(std::unique_ptr<Event> event) {
            std::lock_guard<std::mutex> lock(eventQueueMutex_);
            eventQueue_.push(std::move(event));
        }
    };

    // ============================================================================
    // Windows Platform Surface Implementation
    // ============================================================================

    class WindowsPlatformSurface : public IPlatformSurface {
    private:
        HWND hwnd_;
        HDC hdc_;
        SurfaceInfo surfaceInfo_;
        bool valid_;

    public:
        explicit WindowsPlatformSurface(HWND hwnd)
            : hwnd_(hwnd), hdc_(nullptr), valid_(false) {

            if (hwnd_) {
                hdc_ = GetDC(hwnd_);
                if (hdc_) {
                    UpdateSurfaceInfo();
                    valid_ = true;
                }
            }
        }

        ~WindowsPlatformSurface() override {
            if (hdc_ && hwnd_) {
                ReleaseDC(hwnd_, hdc_);
            }
        }

        SurfaceInfo GetSurfaceInfo() const override {
            return surfaceInfo_;
        }

        bool IsValid() const override {
            return valid_ && IsWindow(hwnd_);
        }

        void Invalidate() override {
            valid_ = false;
        }

        bool Resize(uint32_t width, uint32_t height) override {
            if (!IsValid()) return false;

            surfaceInfo_.width = width;
            surfaceInfo_.height = height;

            // Invalidate any cached surface data
            InvalidateRect(hwnd_, nullptr, FALSE);

            return true;
        }

        bool Present() override {
            if (!IsValid()) return false;

            // For basic implementation without actual rendering,
            // just validate the window and return success
            // This will be enhanced when RHI integration is added

            // Optional: Invalidate client area for basic refresh
            // InvalidateRect(hwnd_, nullptr, FALSE);

            return true;
        }

        void WaitForPresent() override {
            if (IsValid()) {
                // For basic implementation, just ensure window messages are processed
                // This will be enhanced when RHI integration is added

                // Optional: Use UpdateWindow for immediate refresh
                // UpdateWindow(hwnd_);
            }
        }

        void* GetNativeHandle() const override {
            return hwnd_;
        }

        void* GetNativeDisplay() const override {
            return hdc_;
        }

        uint32_t GetNativeFormat() const override {
            // Return basic RGB format - will be enhanced for D3D12
            return 0; // Placeholder
        }

    private:
        void UpdateSurfaceInfo() {
            if (!hwnd_) return;

            RECT clientRect;
            GetClientRect(hwnd_, &clientRect);

            surfaceInfo_.nativeHandle = hwnd_;
            surfaceInfo_.nativeDisplay = hdc_;
            surfaceInfo_.width = clientRect.right - clientRect.left;
            surfaceInfo_.height = clientRect.bottom - clientRect.top;
            surfaceInfo_.format = 0; // Will be set properly with D3D12 integration
            surfaceInfo_.supportsPresent = true;
            surfaceInfo_.sampleCount = 1;
            surfaceInfo_.qualityLevels = 0;
        }
    };

    // ============================================================================
    // Windows Platform Window Implementation
    // ============================================================================

    class WindowsPlatformWindow : public IPlatformWindow {
    private:
        HWND hwnd_;
        WindowProperties properties_;
        std::shared_ptr<WindowsPlatformSurface> surface_;
        std::shared_ptr<WindowsEventSystem> eventSystem_;
        WindowState currentState_;
        CursorMode cursorMode_;
        bool shouldClose_;
        POINT lastCursorPos_;

        // Fullscreen state tracking
        DWORD savedStyle_;
        DWORD savedExStyle_;
        RECT savedRect_;
        bool wasMaximized_;

    public:
        WindowsPlatformWindow(std::shared_ptr<WindowsEventSystem> eventSystem)
            : hwnd_(nullptr)
            , eventSystem_(eventSystem)
            , currentState_(WindowState::Normal)
            , cursorMode_(CursorMode::Normal)
            , shouldClose_(false)
            , savedStyle_(0)
            , savedExStyle_(0)
            , savedRect_{}
            , wasMaximized_(false) {

            lastCursorPos_.x = 0;
            lastCursorPos_.y = 0;
        }

        ~WindowsPlatformWindow() override {
            Destroy();
        }

        bool Initialize(const WindowProperties& properties) override {
            properties_ = properties;

            // Get the window class
            HINSTANCE hInstance = GetModuleHandle(nullptr);

            // Calculate window size including decorations
            DWORD style = properties_.fullscreen ? FULLSCREEN_STYLE : WINDOWED_STYLE;
            DWORD exStyle = properties_.fullscreen ? FULLSCREEN_EX_STYLE : WINDOWED_EX_STYLE;

            if (!properties_.resizable && !properties_.fullscreen) {
                style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
            }

            if (!properties_.decorated && !properties_.fullscreen) {
                style = WS_POPUP;
            }

            RECT windowRect = { 0, 0, static_cast<LONG>(properties_.width), static_cast<LONG>(properties_.height) };
            AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);

            int windowWidth = windowRect.right - windowRect.left;
            int windowHeight = windowRect.bottom - windowRect.top;

            // Calculate position
            int x = properties_.x;
            int y = properties_.y;

            if (x == -1 || y == -1) {
                // Center the window
                int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                int screenHeight = GetSystemMetrics(SM_CYSCREEN);
                x = (screenWidth - windowWidth) / 2;
                y = (screenHeight - windowHeight) / 2;
            }

            // Convert title to wide string
            std::wstring wideTitle = Utf8ToWide(properties_.title);

            // Create the window
            hwnd_ = CreateWindowExW(
                exStyle,
                WINDOW_CLASS_NAME,
                wideTitle.c_str(),
                style,
                x, y,
                windowWidth, windowHeight,
                nullptr,
                nullptr,
                hInstance,
                this  // Pass this pointer to WndProc
            );

            if (!hwnd_) {
                return false;
            }

            // Create surface
            surface_ = std::make_shared<WindowsPlatformSurface>(hwnd_);

            // Set initial state
            if (properties_.maximized && !properties_.fullscreen) {
                ShowWindow(hwnd_, SW_SHOWMAXIMIZED);
                currentState_ = WindowState::Maximized;
            }
            else if (properties_.minimized) {
                ShowWindow(hwnd_, SW_SHOWMINIMIZED);
                currentState_ = WindowState::Minimized;
            }
            else if (properties_.visible) {
                ShowWindow(hwnd_, SW_SHOW);
                currentState_ = properties_.fullscreen ? WindowState::Fullscreen : WindowState::Normal;
            }
            else {
                currentState_ = WindowState::Hidden;
            }

            if (properties_.focused && properties_.visible) {
                SetFocus(hwnd_);
            }

            UpdateWindow(hwnd_);

            return true;
        }

        void Destroy() override {
            if (hwnd_) {
                DestroyWindow(hwnd_);
                hwnd_ = nullptr;
            }
            surface_.reset();
        }

        bool IsValid() const override {
            return hwnd_ != nullptr && IsWindow(hwnd_);
        }

        WindowProperties GetProperties() const override {
            return properties_;
        }

        void SetTitle(const std::string& title) override {
            if (!IsValid()) return;

            properties_.title = title;
            std::wstring wideTitle = Utf8ToWide(title);
            SetWindowTextW(hwnd_, wideTitle.c_str());
        }

        void SetSize(uint32_t width, uint32_t height) override {
            if (!IsValid() || properties_.fullscreen) return;

            properties_.width = width;
            properties_.height = height;

            DWORD style = GetWindowLong(hwnd_, GWL_STYLE);
            DWORD exStyle = GetWindowLong(hwnd_, GWL_EXSTYLE);

            RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
            AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);

            int windowWidth = windowRect.right - windowRect.left;
            int windowHeight = windowRect.bottom - windowRect.top;

            SetWindowPos(hwnd_, nullptr, 0, 0, windowWidth, windowHeight,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        void SetPosition(int32_t x, int32_t y) override {
            if (!IsValid() || properties_.fullscreen) return;

            properties_.x = x;
            properties_.y = y;

            SetWindowPos(hwnd_, nullptr, x, y, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        void SetVisible(bool visible) override {
            if (!IsValid()) return;

            properties_.visible = visible;
            ShowWindow(hwnd_, visible ? SW_SHOW : SW_HIDE);

            if (visible) {
                currentState_ = properties_.fullscreen ? WindowState::Fullscreen : WindowState::Normal;
            }
            else {
                currentState_ = WindowState::Hidden;
            }
        }

        void SetResizable(bool resizable) override {
            if (!IsValid() || properties_.fullscreen) return;

            properties_.resizable = resizable;

            DWORD style = GetWindowLong(hwnd_, GWL_STYLE);
            if (resizable) {
                style |= (WS_THICKFRAME | WS_MAXIMIZEBOX);
            }
            else {
                style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
            }
            SetWindowLong(hwnd_, GWL_STYLE, style);

            // Force window to redraw with new style
            SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }

        void SetDecorated(bool decorated) override {
            if (!IsValid() || properties_.fullscreen) return;

            properties_.decorated = decorated;

            DWORD style = GetWindowLong(hwnd_, GWL_STYLE);
            if (decorated) {
                style = WINDOWED_STYLE;
            }
            else {
                style = WS_POPUP;
            }
            SetWindowLong(hwnd_, GWL_STYLE, style);

            SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }

        void SetAlwaysOnTop(bool alwaysOnTop) override {
            if (!IsValid()) return;

            properties_.alwaysOnTop = alwaysOnTop;

            HWND insertAfter = alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST;
            SetWindowPos(hwnd_, insertAfter, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }

        WindowState GetState() const override {
            return currentState_;
        }

        void SetState(WindowState state) override {
            if (!IsValid()) return;

            switch (state) {
            case WindowState::Normal:
                Restore();
                break;
            case WindowState::Minimized:
                Minimize();
                break;
            case WindowState::Maximized:
                Maximize();
                break;
            case WindowState::Fullscreen:
                SetFullscreen(true);
                break;
            case WindowState::Hidden:
                SetVisible(false);
                break;
            }
        }

        void Minimize() override {
            if (!IsValid()) return;

            ShowWindow(hwnd_, SW_MINIMIZE);
            currentState_ = WindowState::Minimized;
        }

        void Maximize() override {
            if (!IsValid() || properties_.fullscreen) return;

            ShowWindow(hwnd_, SW_MAXIMIZE);
            currentState_ = WindowState::Maximized;
        }

        void Restore() override {
            if (!IsValid()) return;

            if (properties_.fullscreen) {
                SetFullscreen(false);
            }
            else {
                ShowWindow(hwnd_, SW_RESTORE);
                currentState_ = WindowState::Normal;
            }
        }

        void SetFullscreen(bool fullscreen) override {
            if (!IsValid() || properties_.fullscreen == fullscreen) return;

            if (fullscreen) {
                // Save current window state
                savedStyle_ = GetWindowLong(hwnd_, GWL_STYLE);
                savedExStyle_ = GetWindowLong(hwnd_, GWL_EXSTYLE);
                GetWindowRect(hwnd_, &savedRect_);
                wasMaximized_ = (currentState_ == WindowState::Maximized);

                // Get monitor info for fullscreen
                HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTOPRIMARY);
                MONITORINFO monitorInfo;
                monitorInfo.cbSize = sizeof(MONITORINFO);
                GetMonitorInfo(monitor, &monitorInfo);

                // Set fullscreen style
                SetWindowLong(hwnd_, GWL_STYLE, FULLSCREEN_STYLE);
                SetWindowLong(hwnd_, GWL_EXSTYLE, FULLSCREEN_EX_STYLE);

                // Set fullscreen position and size
                SetWindowPos(hwnd_, HWND_TOP,
                    monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                    monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                    monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

                currentState_ = WindowState::Fullscreen;
            }
            else {
                // Restore windowed mode
                SetWindowLong(hwnd_, GWL_STYLE, savedStyle_);
                SetWindowLong(hwnd_, GWL_EXSTYLE, savedExStyle_);

                SetWindowPos(hwnd_, nullptr,
                    savedRect_.left, savedRect_.top,
                    savedRect_.right - savedRect_.left,
                    savedRect_.bottom - savedRect_.top,
                    SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

                if (wasMaximized_) {
                    ShowWindow(hwnd_, SW_MAXIMIZE);
                    currentState_ = WindowState::Maximized;
                }
                else {
                    ShowWindow(hwnd_, SW_NORMAL);
                    currentState_ = WindowState::Normal;
                }
            }

            properties_.fullscreen = fullscreen;
        }

        void Focus() override {
            if (!IsValid()) return;

            SetForegroundWindow(hwnd_);
            SetFocus(hwnd_);
        }

        void SetCursorMode(CursorMode mode) override {
            cursorMode_ = mode;

            switch (mode) {
            case CursorMode::Normal:
                ShowCursor(TRUE);
                ClipCursor(nullptr);
                break;
            case CursorMode::Hidden:
                ShowCursor(FALSE);
                ClipCursor(nullptr);
                break;
            case CursorMode::Disabled:
                ShowCursor(FALSE);
                if (IsValid()) {
                    RECT clipRect;
                    GetClientRect(hwnd_, &clipRect);
                    ClientToScreen(hwnd_, reinterpret_cast<POINT*>(&clipRect.left));
                    ClientToScreen(hwnd_, reinterpret_cast<POINT*>(&clipRect.right));
                    ClipCursor(&clipRect);
                }
                break;
            }
        }

        CursorMode GetCursorMode() const override {
            return cursorMode_;
        }

        Math::Vector2 GetCursorPosition() const override {
            POINT cursorPos;
            GetCursorPos(&cursorPos);
            ScreenToClient(hwnd_, &cursorPos);
            return Math::Vector2(static_cast<float>(cursorPos.x), static_cast<float>(cursorPos.y));
        }

        void SetCursorPosition(const Math::Vector2& position) override {
            if (!IsValid()) return;

            POINT point = { static_cast<LONG>(position.x), static_cast<LONG>(position.y) };
            ClientToScreen(hwnd_, &point);
            SetCursorPos(point.x, point.y);
        }

        std::shared_ptr<IPlatformSurface> GetSurface() const override {
            return surface_;
        }

        void* GetNativeHandle() const override {
            return hwnd_;
        }

        void PollEvents() override {
            MSG msg;
            while (PeekMessage(&msg, hwnd_, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        bool ShouldClose() const override {
            return shouldClose_;
        }

        void SetShouldClose(bool shouldClose) override {
            shouldClose_ = shouldClose;
        }

        void SwapBuffers() override {
            if (surface_) {
                surface_->Present();
            }
        }

        void WaitEvents() override {
            WaitMessage();
        }

        double GetTime() const override {
            static auto start = std::chrono::high_resolution_clock::now();
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start);
            return duration.count() / 1000000.0;
        }

        // Windows-specific method to handle window messages
        LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
            if (!eventSystem_) {
                return DefWindowProc(hwnd_, uMsg, wParam, lParam);
            }

            switch (uMsg) {
            case WM_CLOSE:
            {
                auto event = std::make_unique<WindowEvent>(EventType::WindowCloseRequested, this);
                eventSystem_->QueueEvent(std::move(event));
                shouldClose_ = true;
                return 0;
            }

            case WM_SIZE:
            {
                uint32_t width = LOWORD(lParam);
                uint32_t height = HIWORD(lParam);

                properties_.width = width;
                properties_.height = height;

                if (surface_) {
                    surface_->Resize(width, height);
                }

                auto event = std::make_unique<WindowResizeEvent>(this, width, height);
                eventSystem_->QueueEvent(std::move(event));

                // Update window state based on wParam
                switch (wParam) {
                case SIZE_MINIMIZED:
                    currentState_ = WindowState::Minimized;
                    eventSystem_->QueueEvent(std::make_unique<WindowEvent>(EventType::WindowMinimized, this));
                    break;
                case SIZE_MAXIMIZED:
                    currentState_ = WindowState::Maximized;
                    eventSystem_->QueueEvent(std::make_unique<WindowEvent>(EventType::WindowMaximized, this));
                    break;
                case SIZE_RESTORED:
                    currentState_ = properties_.fullscreen ? WindowState::Fullscreen : WindowState::Normal;
                    eventSystem_->QueueEvent(std::make_unique<WindowEvent>(EventType::WindowRestored, this));
                    break;
                }
                return 0;
            }

            case WM_MOVE:
            {
                int32_t x = static_cast<int32_t>(LOWORD(lParam));
                int32_t y = static_cast<int32_t>(HIWORD(lParam));

                properties_.x = x;
                properties_.y = y;

                auto event = std::make_unique<WindowMoveEvent>(this, x, y);
                eventSystem_->QueueEvent(std::move(event));
                return 0;
            }

            case WM_SETFOCUS:
            {
                auto event = std::make_unique<WindowEvent>(EventType::WindowFocusGained, this);
                eventSystem_->QueueEvent(std::move(event));
                return 0;
            }

            case WM_KILLFOCUS:
            {
                auto event = std::make_unique<WindowEvent>(EventType::WindowFocusLost, this);
                eventSystem_->QueueEvent(std::move(event));
                return 0;
            }

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                KeyCode key = VirtualKeyToKeyCode(wParam);
                KeyModifiers modifiers = GetCurrentModifiers();

                EventType eventType = (lParam & 0x40000000) ? EventType::KeyRepeated : EventType::KeyPressed;
                auto event = std::make_unique<KeyEvent>(eventType, key, modifiers);
                eventSystem_->QueueEvent(std::move(event));

                if (uMsg == WM_SYSKEYDOWN) {
                    return DefWindowProc(hwnd_, uMsg, wParam, lParam);
                }
                return 0;
            }

            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                KeyCode key = VirtualKeyToKeyCode(wParam);
                KeyModifiers modifiers = GetCurrentModifiers();

                auto event = std::make_unique<KeyEvent>(EventType::KeyReleased, key, modifiers);
                eventSystem_->QueueEvent(std::move(event));

                if (uMsg == WM_SYSKEYUP) {
                    return DefWindowProc(hwnd_, uMsg, wParam, lParam);
                }
                return 0;
            }

            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_XBUTTONDOWN:
            {
                MouseButton button = MouseButton::Left;
                switch (uMsg) {
                case WM_LBUTTONDOWN: button = MouseButton::Left; break;
                case WM_RBUTTONDOWN: button = MouseButton::Right; break;
                case WM_MBUTTONDOWN: button = MouseButton::Middle; break;
                case WM_XBUTTONDOWN:
                    button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? MouseButton::X1 : MouseButton::X2;
                    break;
                }

                int32_t x = GET_X_LPARAM(lParam);
                int32_t y = GET_Y_LPARAM(lParam);
                KeyModifiers modifiers = GetCurrentModifiers();

                auto event = std::make_unique<MouseButtonEvent>(EventType::MouseButtonPressed, button, x, y, modifiers);
                eventSystem_->QueueEvent(std::move(event));

                SetCapture(hwnd_);
                return 0;
            }

            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
            case WM_XBUTTONUP:
            {
                MouseButton button = MouseButton::Left;
                switch (uMsg) {
                case WM_LBUTTONUP: button = MouseButton::Left; break;
                case WM_RBUTTONUP: button = MouseButton::Right; break;
                case WM_MBUTTONUP: button = MouseButton::Middle; break;
                case WM_XBUTTONUP:
                    button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? MouseButton::X1 : MouseButton::X2;
                    break;
                }

                int32_t x = GET_X_LPARAM(lParam);
                int32_t y = GET_Y_LPARAM(lParam);
                KeyModifiers modifiers = GetCurrentModifiers();

                auto event = std::make_unique<MouseButtonEvent>(EventType::MouseButtonReleased, button, x, y, modifiers);
                eventSystem_->QueueEvent(std::move(event));

                ReleaseCapture();
                return 0;
            }

            case WM_MOUSEMOVE:
            {
                int32_t x = GET_X_LPARAM(lParam);
                int32_t y = GET_Y_LPARAM(lParam);
                int32_t deltaX = x - lastCursorPos_.x;
                int32_t deltaY = y - lastCursorPos_.y;

                lastCursorPos_.x = x;
                lastCursorPos_.y = y;

                auto event = std::make_unique<MouseMoveEvent>(x, y, deltaX, deltaY);
                eventSystem_->QueueEvent(std::move(event));
                return 0;
            }

            case WM_MOUSEWHEEL:
            {
                float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
                POINT cursorPos;
                GetCursorPos(&cursorPos);
                ScreenToClient(hwnd_, &cursorPos);

                auto event = std::make_unique<MouseScrollEvent>(0.0f, delta, cursorPos.x, cursorPos.y);
                eventSystem_->QueueEvent(std::move(event));
                return 0;
            }

            case WM_MOUSEHWHEEL:
            {
                float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
                POINT cursorPos;
                GetCursorPos(&cursorPos);
                ScreenToClient(hwnd_, &cursorPos);

                auto event = std::make_unique<MouseScrollEvent>(delta, 0.0f, cursorPos.x, cursorPos.y);
                eventSystem_->QueueEvent(std::move(event));
                return 0;
            }

            default:
                return DefWindowProc(hwnd_, uMsg, wParam, lParam);
            }
        }
    };

    // ============================================================================
    // Windows Platform System Implementation
    // ============================================================================

    class WindowsPlatformSystem : public IPlatformSystem {
    private:
        std::shared_ptr<WindowsEventSystem> eventSystem_;
        std::vector<std::shared_ptr<WindowsPlatformWindow>> windows_;
        std::shared_ptr<WindowsPlatformWindow> mainWindow_;
        bool initialized_;
        HINSTANCE hInstance_;

    public:
        WindowsPlatformSystem()
            : initialized_(false), hInstance_(GetModuleHandle(nullptr)) {
        }

        ~WindowsPlatformSystem() override {
            Shutdown();
        }

        bool Initialize() override {
            if (initialized_) return true;

            // Register window class
            WNDCLASSEXW wc = {};
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            wc.lpfnWndProc = WindowProc;
            wc.hInstance = hInstance_;
            wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            wc.lpszClassName = WINDOW_CLASS_NAME;
            wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

            if (!RegisterClassExW(&wc)) {
                return false;
            }

            // Initialize event system
            eventSystem_ = std::make_shared<WindowsEventSystem>();

            // Initialize high-resolution timer
            timeBeginPeriod(1);

            initialized_ = true;
            return true;
        }

        void Shutdown() override {
            if (!initialized_) return;

            // Destroy all windows
            windows_.clear();
            mainWindow_.reset();

            // Cleanup timer
            timeEndPeriod(1);

            // Unregister window class
            UnregisterClassW(WINDOW_CLASS_NAME, hInstance_);

            eventSystem_.reset();
            initialized_ = false;
        }

        bool IsInitialized() const override {
            return initialized_;
        }

        std::shared_ptr<IPlatformWindow> CreateAWindow(const WindowProperties& properties) override {
            if (!initialized_) return nullptr;

            auto window = std::make_shared<WindowsPlatformWindow>(eventSystem_);

            if (!window->Initialize(properties)) {
                return nullptr;
            }

            windows_.push_back(window);

            if (!mainWindow_) {
                mainWindow_ = window;
            }

            return window;
        }

        void DestroyWindow(std::shared_ptr<IPlatformWindow> window) override {
            auto it = std::find_if(windows_.begin(), windows_.end(),
                [&window](const std::weak_ptr<WindowsPlatformWindow>& w) {
                    return w.lock() == window;
                });

            if (it != windows_.end()) {
                windows_.erase(it);
            }

            if (mainWindow_ == window) {
                mainWindow_ = windows_.empty() ? nullptr : windows_[0];
            }
        }

        std::vector<std::shared_ptr<IPlatformWindow>> GetWindows() const override {
            std::vector<std::shared_ptr<IPlatformWindow>> result;
            result.reserve(windows_.size());

            for (const auto& window : windows_) {
                result.push_back(window);
            }

            return result;
        }

        std::shared_ptr<IPlatformWindow> GetMainWindow() const override {
            return mainWindow_;
        }

        std::shared_ptr<IEventSystem> GetEventSystem() const override {
            return eventSystem_;
        }

        std::string GetPlatformName() const override {
            return "Windows";
        }

        std::string GetPlatformVersion() const override {
            OSVERSIONINFOEX osvi = {};
            osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

            if (GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&osvi))) {
                return std::format("{}.{}.{}", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
            }

            return "Unknown";
        }

        uint32_t GetDisplayCount() const override {
            return static_cast<uint32_t>(GetSystemMetrics(SM_CMONITORS));
        }

        Math::Vector2 GetDisplaySize(uint32_t displayIndex) const override {
            if (displayIndex == 0) {
                return Math::Vector2(
                    static_cast<float>(GetSystemMetrics(SM_CXSCREEN)),
                    static_cast<float>(GetSystemMetrics(SM_CYSCREEN))
                );
            }

            // For multiple displays, would need to enumerate monitors
            // For now, return primary display size
            return GetDisplaySize(0);
        }

        Math::Vector2 GetDisplayDPI(uint32_t displayIndex) const override {
            HDC hdc = GetDC(nullptr);
            if (hdc) {
                float dpiX = static_cast<float>(GetDeviceCaps(hdc, LOGPIXELSX));
                float dpiY = static_cast<float>(GetDeviceCaps(hdc, LOGPIXELSY));
                ReleaseDC(nullptr, hdc);
                return Math::Vector2(dpiX, dpiY);
            }

            return Math::Vector2(96.0f, 96.0f); // Default DPI
        }

        void PollSystemEvents() override {
            if (eventSystem_) {
                eventSystem_->ProcessEvents();
            }
        }

        void WaitForEvents() override {
            WaitMessage();
        }

        void PostEmptyEvent() override {
            PostMessage(nullptr, WM_NULL, 0, 0);
        }

        double GetTime() const override {
            static auto start = std::chrono::high_resolution_clock::now();
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start);
            return duration.count() / 1000000.0;
        }

        void Sleep(double seconds) override {
            std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
        }

        std::string GetClipboardText() const override {
            if (!OpenClipboard(nullptr)) return {};

            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (!hData) {
                CloseClipboard();
                return {};
            }

            wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
            if (!pszText) {
                CloseClipboard();
                return {};
            }

            std::string result = WideToUtf8(pszText);

            GlobalUnlock(hData);
            CloseClipboard();

            return result;
        }

        void SetClipboardText(const std::string& text) override {
            std::wstring wideText = Utf8ToWide(text);

            if (!OpenClipboard(nullptr)) return;

            EmptyClipboard();

            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (wideText.length() + 1) * sizeof(wchar_t));
            if (hMem) {
                wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
                if (pMem) {
                    wcscpy_s(pMem, wideText.length() + 1, wideText.c_str());
                    GlobalUnlock(hMem);
                    SetClipboardData(CF_UNICODETEXT, hMem);
                }
            }

            CloseClipboard();
        }

    private:
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
            if (uMsg == WM_CREATE) {
                CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
                WindowsPlatformWindow* window = reinterpret_cast<WindowsPlatformWindow*>(pCreate->lpCreateParams);
                SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
                return 0;
            }

            WindowsPlatformWindow* window = reinterpret_cast<WindowsPlatformWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (window) {
                return window->HandleMessage(uMsg, wParam, lParam);
            }

            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    };

    // ============================================================================
    // Windows Platform Factory Implementation
    // ============================================================================

    class WindowsPlatformFactory : public IPlatformFactory {
    public:
        std::unique_ptr<IPlatformSystem> CreatePlatformSystem() override {
            return std::make_unique<WindowsPlatformSystem>();
        }

        std::string GetPlatformName() const override {
            return "Windows";
        }

        bool IsSupported() const override {
            return true; // Always supported on Windows
        }
    };

} // namespace Akhanda::Platform::Windows

// ============================================================================
// Factory Function Export
// ============================================================================

export namespace Akhanda::Platform {
    inline std::unique_ptr<IPlatformFactory> CreateWindowsPlatformFactory() {
        return std::make_unique<Windows::WindowsPlatformFactory>();
    }
}