// ImGui CVLog, see LICENSE for Copyright information (permissive MIT).

#pragma once

#import <Cocoa/Cocoa.h>

@interface CVLogView : NSOpenGLView
{
    NSTimer*    animationTimer;
}
@end

namespace ImGui
{
namespace CVLog
{

void AddCVLogView(NSWindow* window);

}
}
