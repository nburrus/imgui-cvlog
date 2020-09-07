// ImGui CVLog, see LICENSE for Copyright information (permissive MIT).

#include "imgui_cvlog.h"

#include <imgui/imgui_internal.h>

#include <map>
#include <set>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <mutex>

namespace fs = std::filesystem;

namespace ImGui
{
namespace CVLog
{

class Window;

class WindowData
{
public:
    static const char* defaultCategoryName() { return "Default"; }
    
public:
    WindowData(const char* windowName)
    {
        _name = windowName;
        _id = ImHashStr(windowName);
    }
    
    const std::string& name () const { return _name; }
    ImGuiID id () const { return _id; }
        
    // Here we hackily rely on a special window settings to avoid having to write
    // our own settings handler and have persistent data.
    bool isVisible() const { return _isVisible; };
    bool& isVisibleRef() { return _isVisible; };
    
    bool isDocked() const { return _isDocked; }
    bool& isDockedRef() { return _isDocked; }

public:
    // Can be nullptr if no window was yet created, but properties were specified.
    Window* window = nullptr;
    
    std::string category = defaultCategoryName();
    
    ImVec2 preferredContentSize = ImVec2(320,240);

    float extraWindowHeight() const
    {
        auto& style = ImGui::GetStyle();
        return style.FramePadding.y * 2.f + ImGui::GetFontSize();
    }

    void setPreferredWindowSize(ImVec2 windowSize)
    {
        preferredContentSize = windowSize;
        preferredContentSize.y += extraWindowHeight();
    }

    ImVec2 preferredWindowSize() const
    {
        auto finalSize = preferredContentSize;
        finalSize.y += extraWindowHeight();
        return finalSize;
    }
    
    std::string helpString = "No help specified";
        
    struct
    {
        bool hasData = false;
        ImVec2 pos = ImVec2(0,0);
        ImVec2 size = ImVec2(0,0);
        ImGuiCond_ imGuiCond = ImGuiCond_Always;
    } layoutUpdateOnNextFrame;
        
    std::map<std::string, std::function<void(void)>> preRenderCallbacks;
    
private:
    // Keeping everything to have good performance in Debug too.
    std::string _name;
    ImGuiID _id = 0; // == ImHashStr(name)
    bool _isVisible = true;
    bool _isDocked = false;
};

struct WindowCategory
{
    std::string name;
    std::vector<WindowData*> windows;
};

class WindowManager
{
public:
    static constexpr int windowListWidth = 200;
    
public:
    const std::vector<std::unique_ptr<WindowData>>& windowsData() const { return _windowsData; };
    
    void AddMenuBarCallback(const std::string& name, const std::function<void(void)>& callback)
    {
        _menuBarCallbacks[name] = callback;
    }
    
    WindowData& AddWindow (const char* windowName, std::unique_ptr<Window> windowPtr)
    {
        Window* window = windowPtr.get();
        _windows.emplace_back(std::move(windowPtr));
        auto& data = FindOrCreateDataForWindow(windowName);
        data.window = window;
        window->imGuiData = &data;
        
        auto& vp = *ImGui::GetMainViewport();
        
        const auto preferredWindowSize = data.preferredWindowSize();
        data.layoutUpdateOnNextFrame.size = preferredWindowSize;
        
        const float availableWidth = std::max(0.f, (vp.Size.x - windowListWidth - preferredWindowSize.x));
        const float availableHeight = std::max(0.f, (vp.Size.y - preferredWindowSize.y));
        
        data.layoutUpdateOnNextFrame.pos.x = vp.Pos.x + windowListWidth + (float(rand()) / float(RAND_MAX)) * availableWidth;
        data.layoutUpdateOnNextFrame.pos.y = vp.Pos.y + (float(rand()) / float(RAND_MAX)) * availableHeight;
        data.layoutUpdateOnNextFrame.imGuiCond = ImGuiCond_FirstUseEver;
        data.layoutUpdateOnNextFrame.hasData = true;
        return data;
    }
    
    // Could take a single set with a Json to know which properties to update.
    WindowData& SetWindowCategory (const char* windowName, const char* newCategory)
    {
        auto& data = FindOrCreateDataForWindow(windowName);
        if (data.category == newCategory)
            return data;
        
        auto& oldCat = findOrCreateCategory(data.category.c_str());
        
        auto oldCatIt = std::find_if(oldCat.windows.begin(), oldCat.windows.end(), [&](WindowData* p) {
            return &data == p;
        });
        oldCat.windows.erase(oldCatIt);
        
        data.category = newCategory;
        auto& newCat = findOrCreateCategory(newCategory);
        newCat.windows.push_back (&data);
        return data;
    }
    
    WindowData& SetWindowPreferredSize (const char* windowName, const ImVec2& preferredSize)
    {
        auto& data = FindOrCreateDataForWindow(windowName);
        data.preferredContentSize = preferredSize;
        return data;
    }
    
    WindowData& SetWindowHelpString (const char* windowName, const std::string& helpString)
    {
        auto& data = FindOrCreateDataForWindow(windowName);
        data.helpString = helpString;
        return data;
    }
    
    WindowData& FindOrCreateDataForWindow (const char* windowName)
    {
        ImGuiID windowID = ImHashStr(windowName);
        WindowData* data = findDataForWindow(windowID);
        if (data != nullptr)
            return *data;
        return createDataForWindow(windowName, WindowData::defaultCategoryName());
    }
    
    void TileAndScaleVisibleWindows()
    {
        auto& vp = *ImGui::GetMainViewport();
        
        auto isWindowHeightSmaller = [](const Window* w1, const Window* w2) {
            if (w1->imGuiData->preferredContentSize.y < w2->imGuiData->preferredContentSize.y)
                return true;
            if (w1->imGuiData->preferredContentSize.y > w2->imGuiData->preferredContentSize.y)
                return false;
            if (w1->imGuiData->preferredContentSize.x < w2->imGuiData->preferredContentSize.x)
                return true;
            if (w1->imGuiData->preferredContentSize.x > w2->imGuiData->preferredContentSize.x)
                return false;
            return w1->imGuiData->name() < w2->imGuiData->name();
        };
        
        std::set<Window*, decltype(isWindowHeightSmaller)> windowsSortedBySize(isWindowHeightSmaller);
        for (const auto& win : _windows)
            windowsSortedBySize.insert (win.get());

        const float startX = windowListWidth + vp.Pos.x;
        const float endX = vp.Pos.x + vp.Size.x;
        const float startY = vp.Pos.y;
        const float endY = vp.Pos.y + vp.Size.y;
        
        float scaleFactor = 1.0f;
        bool didFit = false;
        while (!didFit)
        {
            float currentX = startX;
            float currentY = startY;
            float maxHeightInCurrentRow = 0;
            
            // Start optimistic.
            didFit = true;
            
            for (auto& win : windowsSortedBySize)
            {
                auto* winData = win->imGuiData;
                if (!winData->isVisible())
                    continue;

                // Don't reorganize docked windows.
                if (winData->isDocked())
                    continue;
                
                auto preferredSize = winData->preferredWindowSize();
                ImVec2 scaledWinSize (preferredSize.x*scaleFactor,
                                      preferredSize.y*scaleFactor);
                
                // Start new row?
                if (currentX > startX && currentX + scaledWinSize.x > endX)
                {
                    currentX = startX;
                    currentY += maxHeightInCurrentRow;
                    maxHeightInCurrentRow = 0;
                }
                
                // No more row? Start again from the top, but with an offset.
                if (currentY + scaledWinSize.y > endY)
                {
                    didFit = false;
                    // Retry with a smaller scale.
                    scaleFactor *= 0.95;
                    break;
                }
                
                winData->layoutUpdateOnNextFrame.size = scaledWinSize;
                winData->layoutUpdateOnNextFrame.pos = ImVec2(currentX, currentY);
                winData->layoutUpdateOnNextFrame.imGuiCond = ImGuiCond_Always;
                winData->layoutUpdateOnNextFrame.hasData = true;
                ImGui::SetWindowFocus(winData->name().c_str());
                
                currentX += scaledWinSize.x;
                maxHeightInCurrentRow = std::max(maxHeightInCurrentRow, scaledWinSize.y);
            }
        }
    }
    
    void MaybeRenderSaveCurrentLayout(const char* popupName)
    {
        if (ImGui::BeginPopupModal(popupName, NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputText("Name of the layout", _pathBuffer, 256);
            ImGui::Text("(Will be written to %s)", fs::current_path().c_str());
            
            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                std::string fullName = _pathBuffer + std::string(".ini");
                
                if (!std::ofstream(fullName))
                {
                    throw std::runtime_error(("Could not write to " + fs::current_path().string() + '/' + fullName).c_str());
                }
                else
                {
                    ImGui::SaveIniSettingsToDisk(fullName.c_str());
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
    }
    
    void Render()
    {
        auto& vp = *ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp.Pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(windowListWidth, vp.Size.y), ImGuiCond_Always);
        if (ImGui::Begin("Window List",
                         nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar))
        {
            bool openSavePopup = false;
            
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("CVLog"))
                {
                    if (ImGui::BeginMenu("Windows"))
                    {
                        if (ImGui::MenuItem("Show All"))
                        {
                            for (const auto& winData : _windowsData)
                                winData->isVisibleRef() = true;
                        }

                        if (ImGui::MenuItem("Hide All"))
                        {
                            for (const auto& winData : _windowsData)
                                winData->isVisibleRef() = false;
                        }
                        
                        if (ImGui::MenuItem("Tile Windows"))
                        {
                            TileAndScaleVisibleWindows();
                        }
                        
                        ImGui::EndMenu();
                    }
                    
                    if (ImGui::MenuItem("Save Layout As..."))
                    {
                        openSavePopup = true;
                    }
                    
                    if (ImGui::BeginMenu("Load Preset"))
                    {
                        for(const auto& p: fs::directory_iterator(fs::current_path()))
                        {
                            if (p.path().extension() != ".ini")
                                continue;
                            
                            if (ImGui::MenuItem(p.path().filename().string().c_str()))
                            {
                                ImGui::LoadIniSettingsFromDisk(p.path().string().c_str());
                            }
                        }
                                                
                        ImGui::EndMenu();
                    }

                    if (ImGui::MenuItem("Clear All"))
                    {
                        ImGui::CVLog::ClearAll();
                    }
                    
                    ImGui::EndMenu();
                }
                
                for (const auto& it : _menuBarCallbacks)
                    it.second();
                                    
                // We need OpenPopup and BeginPopup to be at the same level.
                if (openSavePopup)
                    ImGui::OpenPopup("Save windows layout as...");
                MaybeRenderSaveCurrentLayout("Save windows layout as...");
            }
            ImGui::EndMenuBar();
                        
            for (auto& cat : _windowsPerCategory)
            {
                // 3-state checkbox for all the windows in the category.
                bool showCat = false;
                {
                    const int checkboxWidth = ImGui::GetFrameHeight() - ImGui::GetStyle().FramePadding.x;
                    
                    showCat = ImGui::CollapsingHeader(cat.name.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_DefaultOpen);
                    
                    ImGui::SameLine(GetContentRegionMax().x - checkboxWidth);
                    
                    int numVisible = 0;
                    for (auto& winData : cat.windows)
                        numVisible += winData->isVisible();
                    
                    bool mixedState = (numVisible > 0 && numVisible != cat.windows.size());
                    
                    if (mixedState)
                    {
                        ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);
                    }
                    
                    bool selected = (numVisible == cat.windows.size());
                    if (ImGui::Checkbox(("##" + cat.name).c_str(), &selected))
                    {
                        for (auto& winData : cat.windows)
                            winData->isVisibleRef() = selected;
                    }
                    
                    if (mixedState)
                        ImGui::PopItemFlag();
                }
                                
                if (!showCat)
                    continue;
                
                for (auto& winData : cat.windows)
                {
                    const bool disabled = (winData->window == nullptr);
                    if (disabled)
                    {
                        // Let us still enable the window in case the update code is conditioned by isVisible.
                        // ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                    }
                    
                    if (ImGui::Checkbox(winData->name().c_str(), &winData->isVisibleRef()))
                    {
                        // Make sure we save the new visible state.
                        ImGui::MarkIniSettingsDirty();
                    }
                    
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                        ImGui::TextUnformatted(winData->name().c_str());
                        ImGui::TextUnformatted(winData->helpString.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                    }
                    
                    if (disabled)
                    {
                        // ImGui::PopItemFlag();
                        ImGui::PopStyleVar();
                    }
                }
            }
        }
        ImGui::End();
        
        for (auto& winData : _windowsData)
        {
            if (winData->window && winData->isVisible())
            {
                if (winData->layoutUpdateOnNextFrame.hasData)
                {
                    ImGui::SetNextWindowPos(winData->layoutUpdateOnNextFrame.pos, winData->layoutUpdateOnNextFrame.imGuiCond);
                    ImGui::SetNextWindowSize(winData->layoutUpdateOnNextFrame.size, winData->layoutUpdateOnNextFrame.imGuiCond);
                    ImGui::SetNextWindowCollapsed(false, winData->layoutUpdateOnNextFrame.imGuiCond);
                    winData->layoutUpdateOnNextFrame = {}; // reset it.
                    ImGui::MarkIniSettingsDirty();
                }
                    
                if (winData->window->Begin(&winData->isVisibleRef()))
                {
                    winData->setPreferredWindowSize(ImGui::GetWindowSize());
                    winData->isDockedRef() = ImGui::IsWindowDocked();

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                        ImGui::TextUnformatted(winData->name().c_str());
                        ImGui::TextUnformatted(winData->helpString.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                    }
                }
                ImGui::End();
                
                if (!winData->preRenderCallbacks.empty())
                {
                    if (winData->window->Begin(nullptr))
                    {
                        for (const auto& it : winData->preRenderCallbacks)
                            it.second();
                    }
                    ImGui::End();
                }
                
                winData->window->Render();
            }
        }
    }
    
    WindowData* ConcurrentFindWindowById(ImGuiID id)
    {
        void* window = concurrent.windowsByID.GetVoidPtr(id);
        return reinterpret_cast<WindowData*>(window);
    }

    WindowData* ConcurrentFindWindow (const char* name)
    {
        ImGuiID id = ImHashStr(name);
        WindowData* windowData = ConcurrentFindWindowById(id);
        
        if (windowData == nullptr)
            return nullptr;
        
        // Non-unique hash? Should be extremely rare! Rename your window if that somehow happens.
        IM_ASSERT (windowData->name() == name);
        return windowData;
    }
    
private:
    void helpMarker(const char* desc)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
    
    WindowData& createDataForWindow (const char* windowName, const char* categoryName)
    {
        _windowsData.emplace_back(std::make_unique<WindowData>(windowName));
        auto* winData = _windowsData.back().get();
        winData->category = categoryName;
        
        auto& cat = findOrCreateCategory(categoryName);
        cat.windows.emplace_back(winData);
        
        {
            std::lock_guard<std::mutex> _(concurrent.lock);
            concurrent.windowsByID.SetVoidPtr(winData->id(), winData);
        }

        return *winData;
    }
    
    WindowCategory& findOrCreateCategory(const char* categoryName)
    {
        for (auto& cat : _windowsPerCategory)
            if (cat.name == categoryName)
                return cat;
        _windowsPerCategory.push_back(WindowCategory());
        _windowsPerCategory.back().name = categoryName;
        return _windowsPerCategory.back();
    }
    
    WindowData* findDataForWindow (const ImGuiID& windowID)
    {
        for (auto& data : _windowsData)
            if (data->id() == windowID)
                return data.get();
        return nullptr;
    }
    
private:
    // Might get accessed as read-only by other threads.
    // If you want to modify it, you need to grab a lock.
    struct
    {
        std::mutex lock;
        ImGuiStorage windowsByID;
    } concurrent;
    
private:
    std::vector<std::unique_ptr<Window>> _windows;
    std::vector<std::unique_ptr<WindowData>> _windowsData;
    std::vector<WindowCategory> _windowsPerCategory;
    std::unordered_map<std::string, std::function<void(void)>> _menuBarCallbacks;
    char _pathBuffer[256];
};

struct Context
{
    struct
    {
        std::mutex lock;
        std::vector<std::function<void(void)>> tasksForNextFrame;
        std::map<std::string, std::function<void(void)>> tasksToRepeatForEachFrame;
    } concurrentTasks;
        
    // Cache to avoid reallocating data on every frame.
    struct {
        std::vector<std::function<void(void)>> tasksToRun;
    } cache;
    
    WindowManager windowManager;
};

} // CVLog
} // ImGui

namespace ImGui
{
namespace CVLog
{

Context* g_Context = new Context();

const char* Window::name() const
{
    return imGuiData->name().c_str();
}

bool Window::isVisible() const
{
    // Thread-safety is not a real issue here since it's just a boolean,
    // no not taking any locking for performance reasons.
    return imGuiData->isVisible();
}

void SetPerFrameCallback(const char* callbackName,
                         const std::function<void(void)>& callback)
{
    std::lock_guard<std::mutex> _ (g_Context->concurrentTasks.lock);
    if (callback)
    {
        g_Context->concurrentTasks.tasksToRepeatForEachFrame[callbackName] = callback;
    }
    else
    {
        g_Context->concurrentTasks.tasksToRepeatForEachFrame.erase(callbackName);
    }
}

void SetWindowProperties(const char* windowName,
                         const char* categoryName, /* = nullptr for no change */
                         const char* helpString, /* = nullptr for no change */
                         int preferredWidth, /* -1 for no change */
                         int preferredHeight /* -1 for no change */)
{
    std::string windowNameCopy = windowName ? windowName : "";
    std::string categoryNameCopy = categoryName ? categoryName : "";
    std::string helpStringCopy = helpString ? helpString : "";
    
    RunOnceInImGuiThread([=]() {
        auto& winData = g_Context->windowManager.FindOrCreateDataForWindow(windowNameCopy.c_str());

        if (!categoryNameCopy.empty())
            g_Context->windowManager.SetWindowCategory(windowNameCopy.c_str(), categoryNameCopy.c_str());

        if (!helpStringCopy.empty())
            winData.helpString = helpStringCopy;
        
        if (preferredWidth > 0)
            winData.preferredContentSize.x = preferredWidth;
        
        if (preferredHeight > 0)
            winData.preferredContentSize.y = preferredHeight;
    });
}

void SetWindowPreRenderCallback(const char* windowName,
                                const char* callbackName,
                                const std::function<void(void)>& callback)
{
    std::string windowNameCopy = windowName;
    std::string callbackNameCopy = callbackName;
    RunOnceInImGuiThread([windowNameCopy,callbackNameCopy,callback]() {
        auto& winData = g_Context->windowManager.FindOrCreateDataForWindow(windowNameCopy.c_str());
        if (callback)
        {
            winData.preRenderCallbacks[callbackNameCopy] = callback;
        }
        else
        {
            winData.preRenderCallbacks.erase(callbackNameCopy);
        }
    });
}

void AddMenuBarCallback(const char* name, const std::function<void(void)>& callback)
{
    std::string nameCopy = name;
    RunOnceInImGuiThread([callback, nameCopy]() {
        g_Context->windowManager.AddMenuBarCallback(nameCopy, callback);
    });
}

void RunOnceInImGuiThread(const std::function<void(void)>& f)
{
    std::lock_guard<std::mutex> _ (g_Context->concurrentTasks.lock);
    g_Context->concurrentTasks.tasksForNextFrame.emplace_back(f);
}

Window* FindOrCreateWindow(const char* name, const std::function<Window*(void)>& createWindowFunc)
{
    WindowData* windowData = g_Context->windowManager.ConcurrentFindWindow(name);
    if (windowData && windowData->window)
        return windowData->window;
    
    auto* concreteWindow = createWindowFunc();
    g_Context->windowManager.AddWindow(name, std::unique_ptr<Window>(concreteWindow));
    return concreteWindow;
}

bool WindowIsVisible(const char* windowName, bool** persistentAddressOfFlag)
{
    auto* windowData = g_Context->windowManager.ConcurrentFindWindow(windowName);
    if (persistentAddressOfFlag)
        *persistentAddressOfFlag = windowData ? &(windowData->isVisibleRef()) : nullptr;
    return windowData && windowData->isVisible();
}

Window* FindWindow(const char* windowName)
{
    auto* windowData = g_Context->windowManager.ConcurrentFindWindow(windowName);
    if (windowData)
        return windowData->window;
    else
        return nullptr;
}

static void CVLogSettingsHandler_ClearAll(ImGuiContext* ctx, ImGuiSettingsHandler*)
{
    ImGuiContext& g = *ctx;
    for (int i = 0; i != g.Windows.Size; i++)
        g.Windows[i]->SettingsOffset = -1;
    g.SettingsWindows.clear();
}

static void* CVLogSettingsHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name)
{
    WindowData& settings = g_Context->windowManager.FindOrCreateDataForWindow(name);
    return (void*)&settings;
}

static void CVLogSettingsHandler_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
{
    WindowData* settings = (WindowData*)entry;
    int i;
    if (sscanf(line, "Visible=%d", &i) == 1)
    {
        settings->isVisibleRef() = i;
    }
}

// Apply to existing windows (if any)
static void CVLogSettingsHandler_ApplyAll(ImGuiContext* ctx, ImGuiSettingsHandler*)
{
}

static void CVLogSettingsHandler_WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    const auto& windowsData = g_Context->windowManager.windowsData();
    
    // Write to text buffer
    buf->reserve(buf->size() + (int)windowsData.size() * 6); // ballpark reserve
    for (const auto& winData : windowsData)
    {
        buf->appendf("[%s][%s]\n", handler->TypeName, winData->name().c_str());
        buf->appendf("Visible=%d\n", winData->isVisible());
        buf->append("\n");
    }
}

// Gui-thread only
void Init()
{
    // Add .ini handle for ImGuiWindow type
    {
        ImGuiSettingsHandler ini_handler;
        ini_handler.TypeName = "CvLogData";
        ini_handler.TypeHash = ImHashStr("CvLogData");
        ini_handler.ClearAllFn = CVLogSettingsHandler_ClearAll;
        ini_handler.ReadOpenFn = CVLogSettingsHandler_ReadOpen;
        ini_handler.ReadLineFn = CVLogSettingsHandler_ReadLine;
        ini_handler.ApplyAllFn = CVLogSettingsHandler_ApplyAll;
        ini_handler.WriteAllFn = CVLogSettingsHandler_WriteAll;
        ImGui::GetCurrentContext()->SettingsHandlers.push_back(ini_handler);
    }
}

void ClearWindow(const char* name)
{
    std::string nameCopy (name);
    RunOnceInImGuiThread([nameCopy](){
        auto* window = FindWindow(nameCopy.c_str());
        if (window)
            window->Clear();
    });
}

void ClearAll()
{
    RunOnceInImGuiThread([](){
        for (const auto& winData : g_Context->windowManager.windowsData())
        {
            if (winData->window)
                winData->window->Clear();
        }
    });
}

// Gui thread only.
void Render()
{
    {
        std::lock_guard<std::mutex> _ (g_Context->concurrentTasks.lock);
        g_Context->cache.tasksToRun.insert (g_Context->cache.tasksToRun.begin(),
                                            g_Context->concurrentTasks.tasksForNextFrame.begin(),
                                            g_Context->concurrentTasks.tasksForNextFrame.end());
        g_Context->concurrentTasks.tasksForNextFrame.clear();
        
        for (const auto& it : g_Context->concurrentTasks.tasksToRepeatForEachFrame)
            g_Context->cache.tasksToRun.push_back(it.second);
    }
    
    for (auto& task : g_Context->cache.tasksToRun)
        task();
    
    g_Context->cache.tasksToRun.clear();
    
    g_Context->windowManager.Render();
}

} // CVLog
} // ImGui
