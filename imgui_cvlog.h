// ImGui CVLog, see LICENSE for Copyright information (permissive MIT).

#pragma once

#include "imgui.h"

#include <functional>

namespace ImGui
{
namespace CVLog
{

/*!
 Call this once per ImGui context.
 Required to create the settings handler.
 
 - Thread safety: only from the ImGui Thread.
 */
void Init();

/*!
 Call this once per ImGui frame to render all the windows.
 
 - Thread safety: only from the ImGui thread.
 */
void Render();

/*!
Clear all the current windows.

- Thread safety: any thread.
*/
void ClearAll();

/*!
Clear the given window.

- Thread safety: any thread.
*/
void ClearWindow(const char* name);

/*!
 Run arbitrary ImGui code for each frame (e.g add some extra windows).

- Thread safety: any thread.
*/
void SetPerFrameCallback(const char* callbackName,
                         const std::function<void(void)>& callback);

/*!
 Run arbitrary ImGui code for a given window (e.g add some UI elements).
 
- Thread safety: any thread.
*/
void SetWindowPreRenderCallback(const char* windowName,
                                const char* callbackName,
                                const std::function<void(void)>& callback);

/*!
 Use this to add custom menu items to the CVLog window.
 
 - Thread safety: any thread.
 */
void AddMenuBarCallback(const char* name, const std::function<void(void)>& callback);

/*!
 Set the properties of a window. The window does not have to exist yet.
 
- Thread safety: any thread.
*/
void SetWindowProperties(const char* windowName,
                         const char* categoryName, /* = nullptr for no change */
                         const char* helpString, /* = nullptr for no change */
                         int preferredWidth = -1, /* -1 for no change */
                         int preferredHeight = -1 /* -1 for no change */);

/*!
 Run arbitrary code in the ImGui thread.
 
 Mostly useful when implementing custom windows.
 
- Thread safety: any thread.
*/
void RunOnceInImGuiThread(const std::function<void(void)>& f);

// API to implement custom window types

class WindowData;
class Window
{
public:
    virtual ~Window () {}
    
    /// Clear past data, called by CVLog::Clear()
    virtual void Clear () = 0;
    
    /// Override this to pass custom flags.
    virtual bool Begin(bool* closed) { return ImGui::Begin(name(), closed); }
    
    /// Implement ImGui rendering here. Called once per frame.
    virtual void Render() = 0;
    
    const char* name() const;
    bool isVisible() const;
    
public:
    // Only access it from the ImGui thread, unless otherwise specified.
    WindowData* imGuiData = nullptr; // will get filled once added to the context.
};

/*!
 Returns whether a window is visible or not.
 
 See CVLOG_FAST_VISIBLITY_CHECK for a fast direct access.

- Thread safety: any thread.
*/
bool WindowIsVisible(const char* windowName, bool** persistentAddressOfFlag = nullptr);

/*!
 Creates a static boolean with the direct address of the visible flag.
 
 This is faster than calling WindowIsVisible since it does not need to compute
 a hash nor grab a mutex to retrieve the window data.
*/
#define CVLOG_FAST_VISIBLITY_CHECK(BoolName, WindowName) \
    static bool* BoolName##DirectPointer = nullptr; \
    const bool BoolName = (BoolName##DirectPointer ? *BoolName##DirectPointer : ImGui::CVLog::WindowIsVisible(WindowName, &BoolName##DirectPointer));

/*!
- Thread safety: any thread.
*/
Window* FindWindow(const char* windowName);

/*!
- Thread safety: any thread.
*/
template <class WindowType>
WindowType* FindWindow (const char* name) { return dynamic_cast<WindowType*>(FindWindow(name)); }

/*!
- Thread safety: only from the ImGui thread.
*/
Window* FindOrCreateWindow(const char* windowName, const std::function<Window*(void)>& createWindowFunc);

/*!
- Thread safety: only from the ImGui thread.
*/
template <class WindowType>
WindowType* FindOrCreateWindow (const char* windowName)
{
    Window* window = FindOrCreateWindow(windowName, []() {
        return (Window*)new WindowType();
    });
    WindowType* concreteWindow = dynamic_cast<WindowType*>(window);
    IM_ASSERT (concreteWindow != nullptr);
    return concreteWindow;
}

} // CVLog
} // ImGui
