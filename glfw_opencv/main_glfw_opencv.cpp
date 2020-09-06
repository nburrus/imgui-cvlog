// ImGui CVLog, see LICENSE for Copyright information (permissive MIT).

#include "imgui_cvlog_gl_opencv.h"
#include "imgui.h"
#include "implot.h"

#include "imgui/examples/imgui_impl_osx.h"
#include "imgui/examples/imgui_impl_opengl3.h"

#include <opencv2/core.hpp>

#include <thread>

void workerThread1()
{
    ImGui::CVLog::SetWindowProperties("VGAImage", "Images", "Image that is VGA", 640, 480);
    
    int i = 0;
    while (true)
    {
        CVLOG_FAST_VISIBLITY_CHECK(isVgaImageVisible, "VGAImage");
        if (isVgaImageVisible)
        {
            cv::Mat1b image (480, 640);
            for (int r = 0; r < image.rows; ++r)
            for (int c = 0; c < image.cols; ++c)
            {
                image(r,c) = (c+r+i*i)%255;
            }
            
            ImGui::CVLog::UpdateImage("VGAImage", image);
        }
        
        ImGui::CVLog::AddValue("ValueList",
                               "Thread1 Index",
                               std::to_string(i).c_str());
        
        for (int k = 0; k < 10; ++k)
        {
            ImGui::CVLog::AddPlotValue(("PlotN - " + std::to_string(k)).c_str(), "Line 1", log(i+1+k), i, "#00ff00ff");
            ImGui::CVLog::AddPlotValue(("PlotN - " + std::to_string(k)).c_str(), "Line 2", log(i+1+k)/2.f, i);
        }
        
        ++i;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

#define IdentityMacro(Code)

void workerThread2()
{
    ImGui::CVLog::SetWindowProperties("SmallImage with a very long name that won't fit", "Images", "Image that is small with an offset", 320, 270);
    
    int offset = 0; // could use atomic, but we don't care for quick&dirty tests.
    ImGui::CVLog::SetWindowPreRenderCallback("SmallImage with a very long name that won't fit", "ModifyOffset", [&offset]() {
        ImGui::SliderInt("Adjust offset", &offset, 0, 320);
    });
    
    ImGui::CVLog::AddValue("ValueList",
                           "Thread2 Status",
                           "Started");
    
    int i = 0;
    while (true)
    {
        if (ImGui::CVLog::WindowIsVisible("SmallImage with a very long name that won't fit"))
        {
            cv::Mat3b image (240, 320);
            for (int r = 0; r < image.rows; ++r)
            for (int c = 0; c < image.cols; ++c)
            {
                image(r,c) = cv::Vec3b((c+r+offset)%255, (c+2*r+offset)%255, (c*2+r+offset)%255);
                // image(r,c) = cv::Vec3b(255,0,0);
            }
        
            ImGui::CVLog::UpdateImage("SmallImage with a very long name that won't fit", image);
        }
        
        ImGui::CVLog::AddPlotValue("Plot1", "Line 1", log(i*i + 1), i);
        ImGui::CVLog::AddPlotValue("Plot1", "Line 2", log(i*i + 1) + 1, i);
        
        ImGui::CVLog::AddValue("ValueList",
                               "Thread2 Index",
                               std::to_string(i).c_str());
            
        ++i;
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
    
    ImGui::CVLog::SetWindowPreRenderCallback("SmallImage with a very long name that won't fit", "ModifyOffset", nullptr);
}

int main(int argc, const char* argv[])
{
    ImGui::CVLog::OpenCVGLWindow window;
    window.initializeContexts("CVLog + OpenCV Demo", 1280, 720);
    
    ImGui::CVLog::AddMenuBarCallback("AppMenu", []() {
        if (ImGui::BeginMenu("MyApp"))
        {
            if (ImGui::MenuItem("MyAction"))
            {
                fprintf(stderr, "MyAction triggered!\n");
            }
            
            if (ImGui::MenuItem("Clear All"))
            {
                ImGui::CVLog::ClearAll();
            }
            
            ImGui::EndMenu();
        }
    });
    
    // Hacky way to launch threads that will never finish.
    new std::thread([]() {
        workerThread1();
    });
    
    new std::thread([]() {
        workerThread2();
    });
    
    window.run();
    window.shutDown();
}
