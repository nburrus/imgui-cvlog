# ImGui :: CVLog

Visual Logging tool for [Dear ImGui](https://github.com/ocornut/imgui), targetting computer vision applications where visualization lots of intermediate images and plots are frequently needed. All the ImGui windows are managed in a central OS window with tools to save presets, hide/show groups of windows, re-arrange them, etc.

While the rendering needs to always happen in the same thread, values and images to display can be sent from anywhere in the code, and from any thread. The overhead is meant to be kept minimal, especially when a window is not visible.

Example of window types are provided for images, numerical plots and value lists, but it is very easy to extend to new types.

# Integration

All you really need is `imgui_cvlog.h/cpp` to get started, + import and modify the window types that you need from `imgui_cvlog_demo.h/cpp`. The plotting example is based on [implot](https://github.com/epezent/implot).

# Examples

You can write expression like this from anywhere in the code:
```
ImGui::CVLog::UpdateImage("MyImage", imagePtr);
```

This will automatically create or update a window "MyImage", and show the content for the image there.

Similarly, you can plot values from anywhere in the code and have a window automatically created and displayed.

```
ImGui::CVLog::AddPlotValue("Plot1", "Line 1", yValue, xValue);
```

The last example will be populate a list of values:

```
ImGui::CVLog::AddValue("ValueList Window", "MyValue", std::to_string(42).c_str());
```

There is currently a sample project for macOS included.

# Screenshot

This is an example of output:

![sample output](https://user-images.githubusercontent.com/541507/84603920-98c92a00-ae92-11ea-8baf-44fb087f6b28.png)

