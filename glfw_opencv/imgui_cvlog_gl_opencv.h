// ImGui CVLog, see LICENSE for Copyright information (permissive MIT).

#pragma once

#include "imgui_cvlog.h"

#include <memory>
#include <vector>

namespace cv {
    class Mat;
};

namespace ImGui
{
namespace CVLog
{

/// Only one per thread, or per application if you're not using a thread-local version of ImGui/ImPlot
class OpenCVGLWindow
{
public:
    OpenCVGLWindow ();
    ~OpenCVGLWindow ();
    
    /// Creates the ImGui, ImPlot and GLFW/OpenGL contexts.
    void initializeContexts (const std::string& title,
                             int windowWidth,
                             int windowHeight);
    
    /// Shutdown the created contexts.
    void shutDown ();
    
    /// Process the window events and rendering. Must be called from the main thread.
    void run ();
    void runOnce ();
    
    bool exitRequested() const;
    
private:
    struct Impl;
    friend struct Impl;
    std::unique_ptr<Impl> impl;
};
    
void UpdateImage(const char* windowName,
                 const cv::Mat& image);

// Plot

void AddPlotValue(const char* windowName,
                  const char* groupName,
                  double yValue,
                  double xValue,
                  const char* style = nullptr);

// Strings

void AddValue(const char* windowName,
              const char* name,
              const char* value);

} // CVLog
} // ImGui
