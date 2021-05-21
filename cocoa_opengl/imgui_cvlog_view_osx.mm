// ImGui CVLog, see LICENSE for Copyright information (permissive MIT).

#include "imgui_cvlog_view_osx.h"
#include "imgui_cvlog.h"

#include <imgui/backends/imgui_impl_osx.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <implot/implot.h>

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

#define GL_SILENCE_DEPRECATION 1

//-----------------------------------------------------------------------------------
// CVLogView
//-----------------------------------------------------------------------------------

@implementation CVLogView

-(void)animationTimerFired:(NSTimer*)timer
{
    [self setNeedsDisplay:YES];
}

-(void)prepareOpenGL
{
    [super prepareOpenGL];
    
#ifndef DEBUG
    GLint swapInterval = 1;
    [[self openGLContext] setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];
    if (swapInterval == 0)
        NSLog(@"Error: Cannot set swap interval.");
#endif
        }

-(void)updateAndDrawDemoView
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplOSX_NewFrame(self);
    ImGui::NewFrame();
    
    // Global data for the demo
    static bool show_demo_window = true;
    static bool show_implot_demo_window = true;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);
    
    if (show_implot_demo_window)
        ImPlot::ShowDemoWindow();
        
    ImGui::CVLog::Render();
    
    // Rendering
    ImGui::Render();
    [[self openGLContext] makeCurrentContext];
    
    ImDrawData* draw_data = ImGui::GetDrawData();
    GLsizei width  = (GLsizei)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    GLsizei height = (GLsizei)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    glViewport(0, 0, width, height);
    
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(draw_data);
    
    // Present
    [[self openGLContext] flushBuffer];
    
    if (!animationTimer)
        animationTimer = [NSTimer scheduledTimerWithTimeInterval:0.033 target:self selector:@selector(animationTimerFired:) userInfo:nil repeats:YES];
        }

-(void)reshape
{
    [[self openGLContext] update];
    [self updateAndDrawDemoView];
    [super reshape];
}

-(void)drawRect:(NSRect)bounds
{
    [self updateAndDrawDemoView];
}

-(BOOL)acceptsFirstResponder
{
    return (YES);
}

-(BOOL)becomeFirstResponder
{
    return (YES);
}

-(BOOL)resignFirstResponder
{
    return (YES);
}

-(void)dealloc
{
    animationTimer = nil;
}

// Forward Mouse/Keyboard events to dear imgui OSX back-end. It returns true when imgui is expecting to use the event.
-(void)keyUp:(NSEvent *)event             { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)keyDown:(NSEvent *)event           { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)flagsChanged:(NSEvent *)event      { ImGui_ImplOSX_HandleEvent(event, self); }

-(void)mouseDown:(NSEvent *)event         { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)mouseUp:(NSEvent *)event           { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)mouseMoved:(NSEvent *)event        { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)mouseDragged:(NSEvent *)event      { ImGui_ImplOSX_HandleEvent(event, self); }

-(void)rightMouseDown:(NSEvent *)event         { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)rightMouseUp:(NSEvent *)event           { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)rightMouseMoved:(NSEvent *)event        { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)rightMouseDragged:(NSEvent *)event      { ImGui_ImplOSX_HandleEvent(event, self); }

-(void)otherMouseDown:(NSEvent *)event         { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)otherMouseUp:(NSEvent *)event           { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)otherMouseMoved:(NSEvent *)event        { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)otherMouseDragged:(NSEvent *)event      { ImGui_ImplOSX_HandleEvent(event, self); }

-(void)scrollWheel:(NSEvent *)event     { ImGui_ImplOSX_HandleEvent(event, self); }

@end

namespace ImGui
{
namespace CVLog
{

void AddCVLogView(NSWindow* window)
{
    NSOpenGLPixelFormatAttribute attrs[] =
    {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize, 32,
        0
    };
    
    NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    CVLogView* view = [[CVLogView alloc] initWithFrame:window.frame pixelFormat:format];
    format = nil;
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
    if (floor(NSAppKitVersionNumber) > NSAppKitVersionNumber10_6)
        [view setWantsBestResolutionOpenGLSurface:YES];
#endif // MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
    [window setContentView:view];
    
    if ([view openGLContext] == nil)
        NSLog(@"No OpenGL Context!");
    
    // Make it current so ImGui_ImplOpenGL3_Init can use it.
    [[view openGLContext] makeCurrentContext];
}

} // ImGui
} // CVLog
