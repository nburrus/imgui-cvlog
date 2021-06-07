// ImGui CVLog, see LICENSE for Copyright information (permissive MIT).

#include "imgui_cvlog_demo_gl.h"
#include "imgui.h"
#include "implot.h"

#include "imgui_cvlog_view_osx.h"
#include "imgui/backends/imgui_impl_osx.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include <thread>

//-----------------------------------------------------------------------------------
// CVLogAppDelegate
//-----------------------------------------------------------------------------------

@interface CVLogAppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, readonly) NSWindow* window;
@end

@implementation CVLogAppDelegate
@synthesize window = _window;

-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
    return YES;
}

-(NSWindow*)window
{
    if (_window != nil)
        return (_window);

    NSRect viewRect = NSMakeRect(100.0, 100.0, 100.0 + 1280.0, 100 + 720.0);

    _window = [[NSWindow alloc] initWithContentRect:viewRect styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable|NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:YES];
    [_window setTitle:@"ImGui::CVLog Example"];
    [_window setAcceptsMouseMovedEvents:YES];
    [_window setOpaque:YES];
    [_window makeKeyAndOrderFront:NSApp];

    return (_window);
}

-(void)setupMenu
{
    NSMenu* mainMenuBar = [[NSMenu alloc] init];
    NSMenu* appMenu;
    NSMenuItem* menuItem;

    appMenu = [[NSMenu alloc] initWithTitle:@"Imgui CVLog"];
    menuItem = [appMenu addItemWithTitle:@"Imgui CVLog" action:@selector(terminate:) keyEquivalent:@"q"];
    [menuItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];

    menuItem = [[NSMenuItem alloc] init];
    [menuItem setSubmenu:appMenu];

    [mainMenuBar addItem:menuItem];

    appMenu = nil;
    [NSApp setMainMenu:mainMenuBar];
}

-(void)dealloc
{
    _window = nil;
}

void workerThread1()
{
    ImGui::CVLog::SetWindowProperties("VGAImage", "Images", "Image that is VGA", 640, 480);
    
    int i = 0;
    while (true)
    {
        CVLOG_FAST_VISIBLITY_CHECK(isVgaImageVisible, "VGAImage");
        if (isVgaImageVisible)
        {
            auto imagePtr = std::make_shared<ImGui::CVLog::Image>();
            imagePtr->width = 640;
            imagePtr->height = 480;
            imagePtr->bytesPerRow = imagePtr->width;
            imagePtr->data.resize (imagePtr->bytesPerRow * imagePtr->height);
            for (int r = 0; r < imagePtr->height; ++r)
                for (int c = 0; c < imagePtr->width; ++c)
                {
                    const int idx = r*imagePtr->bytesPerRow + c;
                    imagePtr->data[idx] = (c+r+i*i)%255;
                }
            
            ImGui::CVLog::UpdateImage("VGAImage", imagePtr);
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
            auto imagePtr = std::make_shared<ImGui::CVLog::Image>();
            imagePtr->width = 320;
            imagePtr->height = 240;
            imagePtr->bytesPerRow = imagePtr->width;
            imagePtr->data.resize (imagePtr->bytesPerRow * imagePtr->height);
            for (int r = 0; r < imagePtr->height; ++r)
                for (int c = 0; c < imagePtr->width; ++c)
                {
                    const int idx = r*imagePtr->bytesPerRow + c;
                    imagePtr->data[idx] = (c+r+offset)%255;
                }
        
            ImGui::CVLog::UpdateImage("SmallImage with a very long name that won't fit", imagePtr);
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

-(void)applicationWillTerminate:(NSNotification *)notification
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplOSX_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}

-(void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Make the application a foreground application (else it won't receive keyboard events)
    ProcessSerialNumber psn = {0, kCurrentProcess};
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);

    // Menu
    [self setupMenu];
    
    // FIXME: do not require 2 calls..
    ImGui::CVLog::AddCVLogView(self.window);

    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer bindings
    ImGui_ImplOSX_Init();
    ImGui_ImplOpenGL3_Init("#version 150"); // the default 130 fails for me.
    
    // Initial the CVLog settings for the current thread.
    ImGui::CVLog::Init();
    
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
}

@end

int main(int argc, const char* argv[])
{
    @autoreleasepool
    {
        NSApp = [NSApplication sharedApplication];
        CVLogAppDelegate* delegate = [[CVLogAppDelegate alloc] init];
        [[NSApplication sharedApplication] setDelegate:delegate];
        [NSApp run];
    }
    return NSApplicationMain(argc, argv);
}
