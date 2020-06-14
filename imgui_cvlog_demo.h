// ImGui CVLog, see LICENSE for Copyright information (permissive MIT).

#pragma once

#include "imgui_cvlog.h"

#include <memory>
#include <vector>

namespace ImGui
{
namespace CVLog
{

// Image

struct Image
{
    std::vector<uint8_t> data;
    int bytesPerRow;
    int width;
    int height;
};
using ImagePtr = std::shared_ptr<Image>;

void UpdateImage(const char* windowName,
                 const ImagePtr& image);

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
