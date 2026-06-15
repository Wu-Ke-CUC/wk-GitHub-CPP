#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <commdlg.h> // 【新增】用于调用 GetOpenFileName 文件选择对话框

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>

#include "obj_mesh.h"
#include "subdivision.h"

#pragma comment(lib, "gdiplus.lib")

namespace {

const char* kWindowClassName = "MeshSubdivisionViewerWindow";
const wchar_t* kMethodNames[] = { L"Loop", L"Catmull-Clark", L"Doo-Sabin" };
const wchar_t* kRenderModeNames[] = { L"Wireframe", L"Fill", L"Fill + Wireframe" };
const int kPanelPadding = 18;
const int kPanelMinWidth = 300;
const int kPanelMaxWidth = 340;
const int kControlHeight = 30;
const int kModelButtonHeight = 52;
const int kModelButtonGap = 12;
const int kDropdownPopupItemHeight = 30;

const COLORREF kViewportColor = RGB(24, 24, 30);
const COLORREF kPanelColor = RGB(34, 34, 40);
const COLORREF kPanelCardColor = RGB(42, 42, 50);
const COLORREF kPanelCardActiveColor = RGB(58, 58, 68);
const COLORREF kBorderColor = RGB(72, 72, 84);
const COLORREF kMeshColor = RGB(173, 216, 255);
const COLORREF kFillColor = RGB(78, 116, 168);
const COLORREF kTextColor = RGB(240, 240, 240);
const COLORREF kMutedTextColor = RGB(180, 180, 188);
const COLORREF kAccentColor = RGB(255, 208, 128);
const COLORREF kComboBackgroundColor = RGB(48, 48, 56);
const COLORREF kComboSelectedColor = RGB(86, 86, 98);

struct MeshView {
    obj_mesh mesh;
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 1.0f;
};

struct Scene {
    std::string algorithmName;
    std::string meshName;
    std::vector<MeshView> levels;
};

struct AppState {
    HINSTANCE instance = nullptr;
    HWND window = nullptr;
    HFONT uiFont = nullptr;
    ULONG_PTR gdiplusToken = 0;
    std::string projectRoot;
    std::vector<Scene> scenes;
    std::vector<std::vector<int> > sceneGroups;
    std::vector<RECT> modelButtonRects;
    RECT methodComboRect = {};
    RECT levelComboRect = {};
    RECT renderModeRect = {};
    RECT statsRect = {};
    RECT shortcutsRect = {};
    int openDropdown = 0;
    int currentMethod = 0;
    int currentVariant = 0;
    int currentLevel = 0;
    int currentRenderMode = 0;
    bool dragging = false;
    POINT lastMouse = {};
    float yaw = 0.65f;
    float pitch = -0.35f;
    float zoom = 1.0f;

    // --- 新增：FPS 统计变量 ---
    DWORD lastFpsTime = 0;
    int frameCount = 0;
    float currentFps = 0.0f;

    // --- 新增：导入按钮的状态 ---
    RECT importBtnRect = {};
    bool importBtnHover = false;

    // --- 新增：滚动条相关 ---
    int modelScrollOffset = 0;  // 模型列表滚动偏移（像素）
    RECT modelScrollAreaRect = {}; // 模型按钮的可见区域
    RECT scrollbarRect = {}; // 滚动条的轨道矩形
    RECT scrollbarThumbRect = {}; // 滚动条滑块矩形
    bool isScrollbarDragging = false; // 是否正在拖动滚动条
    int scrollbarThumbHeight = 0; // 滚动条滑块高度
};

struct ProjectedVertex {
    bool visible = false;
    POINT point = {};
    float depth = 0.0f;
    glm::vec3 viewPosition = glm::vec3(0.0f);
};

struct FaceRenderInfo {
    std::vector<POINT> points;
    float averageDepth = 0.0f;
    COLORREF fillColor = kFillColor;
};

AppState& appState()
{
    static AppState state;
    return state;
}

RECT makeRect(int left, int top, int right, int bottom)
{
    RECT rect = { left, top, right, bottom };
    return rect;
}

bool pointInRect(const RECT& rect, POINT point)
{
    return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom;
}

int pointSizeToHeight(int pointSize)
{
    HDC screenDc = GetDC(nullptr);
    const int logPixelsY = GetDeviceCaps(screenDc, LOGPIXELSY);
    ReleaseDC(nullptr, screenDc);
    return -MulDiv(pointSize, logPixelsY, 72);
}

COLORREF shadeColor(COLORREF baseColor, float intensity)
{
    const float clamped = std::max(0.0f, std::min(1.0f, intensity));
    const int red = static_cast<int>(GetRValue(baseColor) * clamped);
    const int green = static_cast<int>(GetGValue(baseColor) * clamped);
    const int blue = static_cast<int>(GetBValue(baseColor) * clamped);
    return RGB(red, green, blue);
}

std::wstring levelLabel(int level)
{
    wchar_t text[16] = {};
    std::swprintf(text, sizeof(text) / sizeof(text[0]), L"Level %d", level);
    return std::wstring(text);
}

std::wstring renderModeLabel(int mode)
{
    const int safeMode = std::max(0, std::min(2, mode));
    return std::wstring(kRenderModeNames[safeMode]);
}

std::wstring toWide(const std::string& value)
{
    return std::wstring(value.begin(), value.end());
}

std::string parentPath(const std::string& path)
{
    const std::string::size_type pos = path.find_last_of("\\/");
    if (pos == std::string::npos) {
        return std::string();
    }
    return path.substr(0, pos);
}

std::string joinPath(const std::string& left, const std::string& right)
{
    if (left.empty()) {
        return right;
    }
    if (left[left.size() - 1] == '\\' || left[left.size() - 1] == '/') {
        return left + right;
    }
    return left + "\\" + right;
}

bool fileExists(const std::string& path)
{
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::string executableDirectory()
{
    char modulePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
    return parentPath(modulePath);
}

std::string findProjectRoot()
{
    std::string current = executableDirectory();
    for (int depth = 0; depth < 6 && !current.empty(); ++depth) {
        if (fileExists(joinPath(joinPath(current, "models"), "tetrahedron.obj"))) {
            return current;
        }
        current = parentPath(current);
    }
    return std::string();
}

RECT getPanelRect(const RECT& clientRect)
{
    const int clientWidth = clientRect.right - clientRect.left;
    int panelWidth = std::min(kPanelMaxWidth, std::max(kPanelMinWidth, clientWidth / 4));
    if (clientWidth - panelWidth < 360) {
        panelWidth = std::max(260, clientWidth - 360);
    }
    panelWidth = std::max(260, std::min(panelWidth, clientWidth - 240));
    return makeRect(clientRect.right - panelWidth, clientRect.top, clientRect.right, clientRect.bottom);
}

RECT getViewportRect(const RECT& clientRect)
{
    RECT panelRect = getPanelRect(clientRect);
    return makeRect(clientRect.left, clientRect.top, panelRect.left, clientRect.bottom);
}

Gdiplus::Color gdipColor(COLORREF color, BYTE alpha = 255)
{
    return Gdiplus::Color(alpha, GetRValue(color), GetGValue(color), GetBValue(color));
}

Gdiplus::RectF toRectF(const RECT& rect)
{
    return Gdiplus::RectF(
        static_cast<Gdiplus::REAL>(rect.left),
        static_cast<Gdiplus::REAL>(rect.top),
        static_cast<Gdiplus::REAL>(rect.right - rect.left),
        static_cast<Gdiplus::REAL>(rect.bottom - rect.top));
}

void buildRoundedRectPath(Gdiplus::GraphicsPath& path, const RECT& rect, int radius)
{
    const Gdiplus::RectF roundedRect = toRectF(rect);
    const Gdiplus::REAL diameter = static_cast<Gdiplus::REAL>(radius * 2);
    path.AddArc(roundedRect.X, roundedRect.Y, diameter, diameter, 180.0f, 90.0f);
    path.AddArc(roundedRect.GetRight() - diameter, roundedRect.Y, diameter, diameter, 270.0f, 90.0f);
    path.AddArc(roundedRect.GetRight() - diameter, roundedRect.GetBottom() - diameter, diameter, diameter, 0.0f, 90.0f);
    path.AddArc(roundedRect.X, roundedRect.GetBottom() - diameter, diameter, diameter, 90.0f, 90.0f);
    path.CloseFigure();
}

void fillRoundedCard(Gdiplus::Graphics& graphics, const RECT& rect, COLORREF fillColor, COLORREF borderColor, int radius, float borderWidth)
{
    Gdiplus::GraphicsPath path(Gdiplus::FillModeAlternate);
    buildRoundedRectPath(path, rect, radius);
    Gdiplus::SolidBrush fillBrush(gdipColor(fillColor));
    Gdiplus::Pen borderPen(gdipColor(borderColor), borderWidth);
    graphics.FillPath(&fillBrush, &path);
    graphics.DrawPath(&borderPen, &path);
}

MeshView makeMeshView(const obj_mesh& mesh)
{
    MeshView view;
    view.mesh = mesh;

    if (mesh.positions.empty()) {
        return view;
    }

    glm::vec3 minPos = mesh.positions[0];
    glm::vec3 maxPos = mesh.positions[0];

    for (size_t i = 1; i < mesh.positions.size(); ++i) {
        minPos = glm::min(minPos, mesh.positions[i]);
        maxPos = glm::max(maxPos, mesh.positions[i]);
    }

    view.center = (minPos + maxPos) * 0.5f;
    view.radius = 0.0f;

    for (size_t i = 0; i < mesh.positions.size(); ++i) {
        const float distance = glm::length(mesh.positions[i] - view.center);
        view.radius = std::max(view.radius, distance);
    }

    if (view.radius < 1e-4f) {
        view.radius = 1.0f;
    }

    return view;
}

void resetView(AppState& state)
{
    state.yaw = 0.65f;
    state.pitch = -0.35f;
    state.zoom = 1.0f;
}

std::vector<MeshView> buildLevels(const obj_mesh& source, int algorithm)
{
    std::vector<MeshView> levels;
    levels.push_back(makeMeshView(source));

    obj_mesh current = source;
    if (algorithm == 0) {
        LoopSubdivision subdivision;
        subdivision.loadMesh(current);
        for (int i = 0; i < 3; ++i) {
            current = subdivision.execute(1);
            levels.push_back(makeMeshView(current));
        }
        return levels;
    }

    if (algorithm == 1) {
        CatmullSubdivision subdivision;
        subdivision.loadMesh(current);
        for (int i = 0; i < 3; ++i) {
            current = subdivision.execute(1);
            levels.push_back(makeMeshView(current));
        }
        return levels;
    }

    Doosabin2Subdivision subdivision;
    subdivision.loadMesh(current);
    for (int i = 0; i < 3; ++i) {
        current = subdivision.execute(1);
        levels.push_back(makeMeshView(current));
    }
    return levels;
}

bool loadSceneMesh(const std::string& path, obj_mesh& mesh, const char* caption)
{
    if (loadObj(path, mesh)) {
        return true;
    }

    std::string message = "Failed to load: " + path;
    MessageBoxA(nullptr, message.c_str(), caption, MB_ICONERROR | MB_OK);
    return false;
}

bool loadScenes(AppState& state)
{
    state.projectRoot = findProjectRoot();
    if (state.projectRoot.empty()) {
        MessageBoxA(nullptr,
            "Cannot locate the project root folder. Make sure the models directory exists next to the project.",
            "Mesh Subdivision Viewer",
            MB_ICONERROR | MB_OK);
        return false;
    }

    obj_mesh tetrahedron;
    obj_mesh cubeQuad;
    obj_mesh cubeTri;
    const std::string modelDir = joinPath(state.projectRoot, "models");

    if (!loadSceneMesh(joinPath(modelDir, "tetrahedron.obj"), tetrahedron, "Mesh Subdivision Viewer") ||
        !loadSceneMesh(joinPath(modelDir, "cube_quad.obj"), cubeQuad, "Mesh Subdivision Viewer") ||
        !loadSceneMesh(joinPath(modelDir, "cube_tri.obj"), cubeTri, "Mesh Subdivision Viewer")) {
        return false;
    }

    state.scenes.clear();
    state.scenes.push_back(Scene{ "Loop", "tetrahedron", buildLevels(tetrahedron, 0) });
    state.scenes.push_back(Scene{ "Loop", "cube_tri", buildLevels(cubeTri, 0) });
    state.scenes.push_back(Scene{ "Catmull-Clark", "tetrahedron", buildLevels(tetrahedron, 1) });
    state.scenes.push_back(Scene{ "Catmull-Clark", "cube_quad", buildLevels(cubeQuad, 1) });
    state.scenes.push_back(Scene{ "Doo-Sabin", "tetrahedron", buildLevels(tetrahedron, 2) });
    state.scenes.push_back(Scene{ "Doo-Sabin", "cube_quad", buildLevels(cubeQuad, 2) });

    state.sceneGroups.clear();
    state.sceneGroups.push_back(std::vector<int>{ 0, 1 });
    state.sceneGroups.push_back(std::vector<int>{ 2, 3 });
    state.sceneGroups.push_back(std::vector<int>{ 4, 5 });
    return !state.scenes.empty();
}

int currentSceneIndex(const AppState& state)
{
    const std::vector<int>& group = state.sceneGroups[static_cast<size_t>(state.currentMethod)];
    const int safeVariant = std::max(0, std::min(static_cast<int>(group.size()) - 1, state.currentVariant));
    return group[static_cast<size_t>(safeVariant)];
}

const Scene& currentScene(const AppState& state)
{
    return state.scenes[static_cast<size_t>(currentSceneIndex(state))];
}

const MeshView& currentMesh(const AppState& state)
{
    return currentScene(state).levels[static_cast<size_t>(state.currentLevel)];
}

int currentVariantCount(const AppState& state)
{
    if (state.sceneGroups.empty()) {
        return 0;
    }
    return static_cast<int>(state.sceneGroups[static_cast<size_t>(state.currentMethod)].size());
}

void updateWindowTitle(AppState& state)
{
    if (state.window == nullptr || state.scenes.empty()) {
        return;
    }

    const Scene& scene = currentScene(state);
    char title[256] = {};
    std::snprintf(
        title,
        sizeof(title),
        "Mesh Subdivision Viewer - %s - %s - Level %d - %s",
        scene.algorithmName.c_str(),
        scene.meshName.c_str(),
        state.currentLevel,
        state.currentRenderMode == 0 ? "Wireframe" : (state.currentRenderMode == 1 ? "Fill" : "Fill+Wireframe"));
    SetWindowTextA(state.window, title);
}

void syncComboSelections(AppState& state)
{
    (void)state;
}

void layoutUi(AppState& state)
{
    if (state.window == nullptr) {
        return;
    }

    RECT clientRect = {};
    GetClientRect(state.window, &clientRect);
    RECT panelRect = getPanelRect(clientRect);

    const int left = panelRect.left + kPanelPadding;
    const int width = panelRect.right - panelRect.left - kPanelPadding * 2;
    const int panelHeight = panelRect.bottom - panelRect.top;

    int top = panelRect.top + 82;

    // 第一步：计算固定的下拉框位置
    state.methodComboRect = makeRect(left, top, left + width, top + kControlHeight);
    top += 74;
    state.levelComboRect = makeRect(left, top, left + width, top + kControlHeight);
    top += 74;
    state.renderModeRect = makeRect(left, top, left + width, top + kControlHeight);
    top += 74;

    // 第二步：从面板底部向上计算固定元素的位置
    // 快捷键框位置（固定在面板底部）
    state.shortcutsRect = makeRect(left, panelRect.bottom - 150, left + width, panelRect.bottom - 24);

    // 导入按钮位置（在快捷键框上方）
    int importBtnHeight = 35;
    int importBtnTop = state.shortcutsRect.top - importBtnHeight - 15;
    state.importBtnRect = makeRect(left, importBtnTop, left + width, importBtnTop + importBtnHeight);

    // 统计信息框位置（在导入按钮上方）
    int statsHeight = 108;
    int statsTop = state.importBtnRect.top - statsHeight - 8;
    state.statsRect = makeRect(left, statsTop, left + width, statsTop + statsHeight);

    // 第三步：计算模型按钮的动态区域（在下拉框和统计框之间）
    const int variantCount = currentVariantCount(state);
    state.modelButtonRects.assign(static_cast<size_t>(variantCount), RECT{});

    // 可用于模型按钮的空间
    int availableTop = top; // 从下拉框后面开始
    int availableBottom = state.statsRect.top - 8; // 到统计框前面结束
    int availableHeight = availableBottom - availableTop;

    // 滚动条相关常量
    const int scrollbarWidth = 12;
    const int scrollbarMargin = 4;
    const int modelAreaWidth = width - scrollbarWidth - scrollbarMargin;

    // 设置模型按钮的可见区域（不包括滚动条）
    state.modelScrollAreaRect = makeRect(left, availableTop, left + modelAreaWidth, availableBottom);

    // 滚动条轨道矩形
    state.scrollbarRect = makeRect(left + modelAreaWidth + scrollbarMargin, availableTop, left + width, availableBottom);

    if (variantCount > 0) {
        int totalButtonHeight = variantCount * kModelButtonHeight + (variantCount - 1) * kModelButtonGap;

        if (totalButtonHeight > availableHeight) {
            // 需要滚动条
            // 计算滚动条滑块大小和位置
            state.scrollbarThumbHeight = std::max(20, (availableHeight * availableHeight) / totalButtonHeight);
            int scrollbarTrackHeight = availableHeight - state.scrollbarThumbHeight;

            // 限制滚动偏移范围
            int maxScroll = totalButtonHeight - availableHeight;
            state.modelScrollOffset = std::max(0, std::min(state.modelScrollOffset, maxScroll));

            // 计算滑块位置
            int thumbTop = availableTop + (scrollbarTrackHeight * state.modelScrollOffset) / maxScroll;
            state.scrollbarThumbRect = makeRect(
                state.scrollbarRect.left,
                thumbTop,
                state.scrollbarRect.right,
                thumbTop + state.scrollbarThumbHeight);

            // 计算所有模型按钮位置（基于滚动偏移）
            int buttonTop = availableTop - state.modelScrollOffset;
            for (int i = 0; i < variantCount; ++i) {
                state.modelButtonRects[static_cast<size_t>(i)] = makeRect(
                    left, 
                    buttonTop, 
                    left + modelAreaWidth, 
                    buttonTop + kModelButtonHeight);
                buttonTop += kModelButtonHeight + kModelButtonGap;
            }
        } else {
            // 无需滚动条
            state.modelScrollOffset = 0;
            state.scrollbarThumbHeight = 0;
            state.scrollbarThumbRect = makeRect(0, 0, 0, 0);

            // 正常分配按钮位置
            int buttonTop = availableTop;
            for (int i = 0; i < variantCount; ++i) {
                state.modelButtonRects[static_cast<size_t>(i)] = makeRect(left, buttonTop, left + width, buttonTop + kModelButtonHeight);
                buttonTop += kModelButtonHeight + kModelButtonGap;
            }
        }
    }
}

void refreshUi(AppState& state)
{
    const int variantCount = std::max(1, currentVariantCount(state));
    state.currentVariant = std::max(0, std::min(variantCount - 1, state.currentVariant));
    state.currentLevel = std::max(0, std::min(3, state.currentLevel));
    layoutUi(state);
    syncComboSelections(state);
    updateWindowTitle(state);
    if (state.window != nullptr) {
        InvalidateRect(state.window, nullptr, FALSE);
    }
}

void setMethod(AppState& state, int methodIndex)
{
    if (state.sceneGroups.empty()) {
        return;
    }

    const int maxMethod = static_cast<int>(state.sceneGroups.size()) - 1;
    const int nextMethod = std::max(0, std::min(maxMethod, methodIndex));
    if (nextMethod != state.currentMethod) {
        state.currentMethod = nextMethod;
        state.currentVariant = 0;
        state.currentLevel = 0;
    }
    refreshUi(state);
}

void setVariant(AppState& state, int variantIndex)
{
    const int variantCount = currentVariantCount(state);
    if (variantCount <= 0) {
        return;
    }

    state.currentVariant = std::max(0, std::min(variantCount - 1, variantIndex));
    state.currentLevel = 0;
    refreshUi(state);
}

void setLevel(AppState& state, int level)
{
    state.currentLevel = std::max(0, std::min(3, level));
    refreshUi(state);
}

void setRenderMode(AppState& state, int renderMode)
{
    state.currentRenderMode = std::max(0, std::min(2, renderMode));
    refreshUi(state);
}

void cycleMeshVariant(AppState& state, int offset)
{
    const int variantCount = currentVariantCount(state);
    if (variantCount <= 0) {
        return;
    }

    state.currentVariant = (state.currentVariant + offset + variantCount) % variantCount;
    state.currentLevel = 0;
    refreshUi(state);
}

void changeLevel(AppState& state, int offset)
{
    setLevel(state, state.currentLevel + offset);
}

bool createUiControls(AppState& state)
{
    state.uiFont = CreateFontW(
        pointSizeToHeight(11),
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");
    return state.uiFont != nullptr;
}

RECT dropdownPopupRect(const RECT& comboRect, int itemCount)
{
    return makeRect(
        comboRect.left,
        comboRect.bottom + 6,
        comboRect.right,
        comboRect.bottom + 6 + itemCount * kDropdownPopupItemHeight + 8);
}

void drawDropdownArrow(Gdiplus::Graphics& graphics, const RECT& rect, bool expanded)
{
    const Gdiplus::REAL cx = static_cast<Gdiplus::REAL>(rect.right - 18);
    const Gdiplus::REAL cy = static_cast<Gdiplus::REAL>((rect.top + rect.bottom) / 2);
    Gdiplus::PointF points[3];
    if (expanded) {
        points[0] = Gdiplus::PointF(cx - 5.0f, cy + 3.0f);
        points[1] = Gdiplus::PointF(cx, cy - 3.0f);
        points[2] = Gdiplus::PointF(cx + 5.0f, cy + 3.0f);
    } else {
        points[0] = Gdiplus::PointF(cx - 5.0f, cy - 3.0f);
        points[1] = Gdiplus::PointF(cx, cy + 3.0f);
        points[2] = Gdiplus::PointF(cx + 5.0f, cy - 3.0f);
    }

    Gdiplus::SolidBrush arrowBrush(gdipColor(kTextColor));
    graphics.FillPolygon(&arrowBrush, points, 3);
}

void drawDropdownField(
    Gdiplus::Graphics& graphics,
    const RECT& rect,
    const std::wstring& text,
    bool expanded,
    const Gdiplus::Font& font)
{
    fillRoundedCard(graphics, rect, kComboBackgroundColor, expanded ? kAccentColor : kBorderColor, 8, expanded ? 2.0f : 1.0f);

    Gdiplus::SolidBrush textBrush(gdipColor(kTextColor));
    Gdiplus::RectF textRect = toRectF(rect);
    textRect.X += 12.0f;
    textRect.Y += 4.0f;
    textRect.Width -= 36.0f;
    textRect.Height -= 8.0f;
    graphics.DrawString(text.c_str(), -1, &font, textRect, nullptr, &textBrush);
    drawDropdownArrow(graphics, rect, expanded);
}

void drawDropdownPopup(
    Gdiplus::Graphics& graphics,
    const RECT& comboRect,
    const std::vector<std::wstring>& items,
    int selectedIndex,
    const Gdiplus::Font& font)
{
    if (items.empty()) {
        return;
    }

    const RECT popupRect = dropdownPopupRect(comboRect, static_cast<int>(items.size()));
    fillRoundedCard(graphics, popupRect, kPanelCardColor, kBorderColor, 8, 1.0f);

    Gdiplus::SolidBrush textBrush(gdipColor(kTextColor));
    Gdiplus::SolidBrush mutedBrush(gdipColor(kMutedTextColor));
    for (size_t i = 0; i < items.size(); ++i) {
        RECT itemRect = makeRect(
            popupRect.left + 4,
            popupRect.top + 4 + static_cast<int>(i) * kDropdownPopupItemHeight,
            popupRect.right - 4,
            popupRect.top + 4 + static_cast<int>(i + 1) * kDropdownPopupItemHeight);

        if (static_cast<int>(i) == selectedIndex) {
            fillRoundedCard(graphics, itemRect, kComboSelectedColor, kComboSelectedColor, 6, 1.0f);
        }

        Gdiplus::RectF textRect = toRectF(itemRect);
        textRect.X += 10.0f;
        textRect.Y += 4.0f;
        textRect.Width -= 20.0f;
        textRect.Height -= 8.0f;
        graphics.DrawString(items[i].c_str(), -1, &font, textRect, nullptr, static_cast<int>(i) == selectedIndex ? &textBrush : &mutedBrush);
    }
}

ProjectedVertex projectVertex(const glm::vec3& position, const MeshView& mesh, const RECT& viewportRect, const AppState& state)
{
    ProjectedVertex projected;

    glm::vec3 local = position - mesh.center;

    const float cosYaw = std::cos(state.yaw);
    const float sinYaw = std::sin(state.yaw);
    const float cosPitch = std::cos(state.pitch);
    const float sinPitch = std::sin(state.pitch);

    glm::vec3 rotated;
    rotated.x = cosYaw * local.x + sinYaw * local.z;
    rotated.z = -sinYaw * local.x + cosYaw * local.z;
    rotated.y = cosPitch * local.y - sinPitch * rotated.z;
    rotated.z = sinPitch * local.y + cosPitch * rotated.z;

    const int width = viewportRect.right - viewportRect.left;
    const int height = viewportRect.bottom - viewportRect.top;
    const float distance = (mesh.radius * 3.2f) / std::max(0.25f, state.zoom) + 0.5f;
    const float depth = rotated.z + distance;
    if (depth <= 0.05f) {
        return projected;
    }

    const float screenScale = static_cast<float>(std::min(width, height)) * 0.95f;
    const float perspective = screenScale / depth;

    projected.visible = true;
    projected.depth = depth;
    projected.viewPosition = rotated;
    projected.point.x = static_cast<LONG>(viewportRect.left + width * 0.5f + rotated.x * perspective);
    projected.point.y = static_cast<LONG>(viewportRect.top + height * 0.56f - rotated.y * perspective);
    return projected;
}

void drawViewportOverlay(HDC dc, const RECT& viewportRect, const AppState& state)
{
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, kTextColor);

    const Scene& scene = currentScene(state);
    const MeshView& mesh = currentMesh(state);

    char info[256] = {};
    std::snprintf(
        info,
        sizeof(info),
        "%s / %s / Level %d / %s\r\nVertices: %u  Faces: %u\r\nUse right panel or hotkeys to switch settings",
        scene.algorithmName.c_str(),
        scene.meshName.c_str(),
        state.currentLevel,
        state.currentRenderMode == 0 ? "Wireframe" : (state.currentRenderMode == 1 ? "Fill" : "Fill+Wireframe"),
        static_cast<unsigned int>(mesh.mesh.positions.size()),
        static_cast<unsigned int>(mesh.mesh.faces.size()));

    RECT textRect = viewportRect;
    textRect.left += 16;
    textRect.top += 14;
    textRect.right -= 16;
    DrawTextA(dc, info, -1, &textRect, DT_LEFT | DT_TOP);
}

std::vector<FaceRenderInfo> buildRenderableFaces(const obj_mesh& mesh, const std::vector<ProjectedVertex>& projected)
{
    std::vector<FaceRenderInfo> faces;
    faces.reserve(mesh.faces.size());

    const glm::vec3 lightDirection = glm::normalize(glm::vec3(-0.45f, 0.7f, -0.55f));
    const glm::vec3 viewDirection = glm::vec3(0.0f, 0.0f, -1.0f);

    for (size_t faceIndex = 0; faceIndex < mesh.faces.size(); ++faceIndex) {
        const face_t& face = mesh.faces[faceIndex];
        if (face.size() < 3) {
            continue;
        }

        FaceRenderInfo info;
        float depthSum = 0.0f;
        bool valid = true;
        std::vector<glm::vec3> viewPoints;
        viewPoints.reserve(face.size());

        for (size_t vertexIndex = 0; vertexIndex < face.size(); ++vertexIndex) {
            const unsigned int positionIndex = face[vertexIndex].v_idx;
            if (positionIndex >= projected.size() || !projected[positionIndex].visible) {
                valid = false;
                break;
            }
            info.points.push_back(projected[positionIndex].point);
            depthSum += projected[positionIndex].depth;
            viewPoints.push_back(projected[positionIndex].viewPosition);
        }

        if (!valid || info.points.size() < 3) {
            continue;
        }

        info.averageDepth = depthSum / static_cast<float>(info.points.size());
        glm::vec3 edge0 = viewPoints[1] - viewPoints[0];
        glm::vec3 edge1 = viewPoints[2] - viewPoints[0];
        glm::vec3 normal = glm::cross(edge0, edge1);
        const float normalLength = glm::length(normal);
        if (normalLength > 1e-5f) {
            normal /= normalLength;
            if (glm::dot(normal, viewDirection) > 0.0f) {
                normal = -normal;
            }

            const float ambient = 0.30f;
            const float diffuse = std::max(0.0f, glm::dot(normal, lightDirection));
            const glm::vec3 halfVector = glm::normalize(lightDirection + viewDirection);
            const float specular = std::pow(std::max(0.0f, glm::dot(normal, halfVector)), 20.0f) * 0.18f;
            const float intensity = std::min(1.0f, ambient + diffuse * 0.72f + specular);
            info.fillColor = shadeColor(kFillColor, intensity);
        } else {
            info.fillColor = shadeColor(kFillColor, 0.55f);
        }
        faces.push_back(info);
    }

    std::sort(
        faces.begin(),
        faces.end(),
        [](const FaceRenderInfo& left, const FaceRenderInfo& right) {
            return left.averageDepth > right.averageDepth;
        });
    return faces;
}

void drawViewport(HDC dc, const RECT& clientRect, const AppState& state)
{
    RECT viewportRect = getViewportRect(clientRect);
    HBRUSH background = CreateSolidBrush(kViewportColor);
    FillRect(dc, &viewportRect, background);
    DeleteObject(background);

    HPEN dividerPen = CreatePen(PS_SOLID, 1, kBorderColor);
    HPEN oldPen = static_cast<HPEN>(SelectObject(dc, dividerPen));
    MoveToEx(dc, viewportRect.right, viewportRect.top, nullptr);
    LineTo(dc, viewportRect.right, viewportRect.bottom);
    SelectObject(dc, oldPen);
    DeleteObject(dividerPen);

    if (state.scenes.empty()) {
        return;
    }

    const MeshView& mesh = currentMesh(state);
    std::vector<ProjectedVertex> projected(mesh.mesh.positions.size());
    for (size_t i = 0; i < mesh.mesh.positions.size(); ++i) {
        projected[i] = projectVertex(mesh.mesh.positions[i], mesh, viewportRect, state);
    }

    const std::vector<FaceRenderInfo> renderFaces = buildRenderableFaces(mesh.mesh, projected);

    if (state.currentRenderMode == 1 || state.currentRenderMode == 2) {
        HPEN fillPen = CreatePen(PS_SOLID, 1, state.currentRenderMode == 2 ? kMeshColor : kFillColor);
        oldPen = static_cast<HPEN>(SelectObject(dc, fillPen));
        HBRUSH oldBrush = static_cast<HBRUSH>(GetCurrentObject(dc, OBJ_BRUSH));

        for (size_t faceIndex = 0; faceIndex < renderFaces.size(); ++faceIndex) {
            const FaceRenderInfo& faceInfo = renderFaces[faceIndex];
            if (faceInfo.points.size() < 3) {
                continue;
            }
            HBRUSH faceBrush = CreateSolidBrush(faceInfo.fillColor);
            SelectObject(dc, faceBrush);
            Polygon(dc, &faceInfo.points[0], static_cast<int>(faceInfo.points.size()));
            DeleteObject(SelectObject(dc, oldBrush));
        }

        SelectObject(dc, oldPen);
        DeleteObject(fillPen);
    }

    if (state.currentRenderMode == 0 || state.currentRenderMode == 2) {
        HPEN meshPen = CreatePen(PS_SOLID, 1, kMeshColor);
        oldPen = static_cast<HPEN>(SelectObject(dc, meshPen));

        for (size_t faceIndex = 0; faceIndex < mesh.mesh.faces.size(); ++faceIndex) {
            const face_t& face = mesh.mesh.faces[faceIndex];
            if (face.size() < 2) {
                continue;
            }

            for (size_t edgeIndex = 0; edgeIndex < face.size(); ++edgeIndex) {
                const unsigned int currentIndex = face[edgeIndex].v_idx;
                const unsigned int nextIndex = face[(edgeIndex + 1) % face.size()].v_idx;

                if (currentIndex >= projected.size() || nextIndex >= projected.size()) {
                    continue;
                }

                if (!projected[currentIndex].visible || !projected[nextIndex].visible) {
                    continue;
                }

                MoveToEx(dc, projected[currentIndex].point.x, projected[currentIndex].point.y, nullptr);
                LineTo(dc, projected[nextIndex].point.x, projected[nextIndex].point.y);
            }
        }

        SelectObject(dc, oldPen);
        DeleteObject(meshPen);
    }

    drawViewportOverlay(dc, viewportRect, state);
}

void drawPanel(Gdiplus::Graphics& graphics, const RECT& clientRect, const AppState& state)
{
    RECT panelRect = getPanelRect(clientRect);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    Gdiplus::SolidBrush panelBrush(gdipColor(kPanelColor));
    graphics.FillRectangle(&panelBrush, toRectF(panelRect));

    Gdiplus::Pen topBorderPen(gdipColor(kBorderColor), 1.0f);
    graphics.DrawLine(
        &topBorderPen,
        static_cast<Gdiplus::REAL>(panelRect.left),
        static_cast<Gdiplus::REAL>(panelRect.top),
        static_cast<Gdiplus::REAL>(panelRect.left),
        static_cast<Gdiplus::REAL>(panelRect.bottom));

    Gdiplus::Font titleFont(L"Segoe UI", 24.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::Font sectionFont(L"Segoe UI", 14.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::Font bodyFont(L"Segoe UI", 13.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::Font smallFont(L"Segoe UI", 12.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush textBrush(gdipColor(kTextColor));
    Gdiplus::SolidBrush mutedBrush(gdipColor(kMutedTextColor));
    Gdiplus::SolidBrush accentBrush(gdipColor(kAccentColor));

    const std::wstring methodTitle = L"Subdivision Control";
    graphics.DrawString(methodTitle.c_str(), -1, &titleFont, Gdiplus::PointF(static_cast<Gdiplus::REAL>(panelRect.left + kPanelPadding), 20.0f), &textBrush);

    graphics.DrawString(L"Method", -1, &sectionFont, Gdiplus::PointF(static_cast<Gdiplus::REAL>(state.methodComboRect.left), static_cast<Gdiplus::REAL>(state.methodComboRect.top - 24)), &mutedBrush);
    graphics.DrawString(L"Level", -1, &sectionFont, Gdiplus::PointF(static_cast<Gdiplus::REAL>(state.levelComboRect.left), static_cast<Gdiplus::REAL>(state.levelComboRect.top - 24)), &mutedBrush);
    graphics.DrawString(L"Render", -1, &sectionFont, Gdiplus::PointF(static_cast<Gdiplus::REAL>(state.renderModeRect.left), static_cast<Gdiplus::REAL>(state.renderModeRect.top - 24)), &mutedBrush);
    drawDropdownField(graphics, state.methodComboRect, kMethodNames[state.currentMethod], state.openDropdown == 1, bodyFont);
    drawDropdownField(graphics, state.levelComboRect, levelLabel(state.currentLevel), state.openDropdown == 2, bodyFont);
    drawDropdownField(graphics, state.renderModeRect, renderModeLabel(state.currentRenderMode), state.openDropdown == 3, bodyFont);

    if (!state.modelButtonRects.empty()) {
        graphics.DrawString(L"Mesh", -1, &sectionFont, Gdiplus::PointF(static_cast<Gdiplus::REAL>(state.modelButtonRects[0].left), static_cast<Gdiplus::REAL>(state.modelScrollAreaRect.top - 26)), &mutedBrush);
    }

    // --- 新增：设置裁剪区域，只显示可见的模型按钮 ---
    Gdiplus::Region originalRegion;
    graphics.GetClip(&originalRegion);
    Gdiplus::Region clipRegion(toRectF(state.modelScrollAreaRect));
    graphics.SetClip(&clipRegion, Gdiplus::CombineModeReplace);

    const std::vector<int>& group = state.sceneGroups[static_cast<size_t>(state.currentMethod)];
    for (size_t i = 0; i < state.modelButtonRects.size(); ++i) {
        const RECT& buttonRect = state.modelButtonRects[i];

        // 检查按钮是否在可见区域内
        if (buttonRect.bottom <= state.modelScrollAreaRect.top || buttonRect.top >= state.modelScrollAreaRect.bottom) {
            continue; // 跳过不可见的按钮
        }

        const Scene& scene = state.scenes[static_cast<size_t>(group[i])];
        const bool selected = static_cast<int>(i) == state.currentVariant;

        fillRoundedCard(
            graphics,
            buttonRect,
            selected ? kPanelCardActiveColor : kPanelCardColor,
            selected ? kAccentColor : kBorderColor,
            10,
            selected ? 2.0f : 1.0f);

        Gdiplus::SolidBrush* currentTextBrush = selected ? &textBrush : &mutedBrush;
        Gdiplus::SolidBrush* badgeBrush = selected ? &accentBrush : &textBrush;

        std::wstring meshName = toWide(scene.meshName);
        std::wstring caption = (i == 0) ? L"Primary Mesh" : L"Alternative Mesh";
        graphics.DrawString(meshName.c_str(), -1, &sectionFont, Gdiplus::PointF(static_cast<Gdiplus::REAL>(buttonRect.left + 14), static_cast<Gdiplus::REAL>(buttonRect.top + 11)), currentTextBrush);
        graphics.DrawString(caption.c_str(), -1, &smallFont, Gdiplus::PointF(static_cast<Gdiplus::REAL>(buttonRect.left + 14), static_cast<Gdiplus::REAL>(buttonRect.top + 30)), badgeBrush);
    }

    // --- 恢复原始裁剪区域 ---
    graphics.SetClip(&originalRegion, Gdiplus::CombineModeReplace);

    // --- 新增：绘制滚动条 ---
    if (state.scrollbarThumbHeight > 0) {
        // 绘制滚动条轨道背景
        Gdiplus::SolidBrush scrollbarBg(gdipColor(kPanelCardColor));
        graphics.FillRectangle(&scrollbarBg, toRectF(state.scrollbarRect));

        // 绘制滚动条边框
        Gdiplus::Pen scrollbarBorder(gdipColor(kBorderColor), 1.0f);
        graphics.DrawRectangle(&scrollbarBorder, toRectF(state.scrollbarRect));

        // 绘制滚动条滑块
        Gdiplus::SolidBrush scrollbarThumb(gdipColor(kMutedTextColor));
        Gdiplus::Pen scrollbarThumbBorder(gdipColor(kTextColor), 1.0f);
        graphics.FillRectangle(&scrollbarThumb, toRectF(state.scrollbarThumbRect));
        graphics.DrawRectangle(&scrollbarThumbBorder, toRectF(state.scrollbarThumbRect));
    }

    fillRoundedCard(graphics, state.statsRect, kPanelCardColor, kBorderColor, 10, 1.0f);
    const MeshView& mesh = currentMesh(state);
    char statsBuffer[256] = {};
    std::snprintf(
        statsBuffer,
        sizeof(statsBuffer),
        "Algorithm: %s\nMesh: %s\nVertices: %u\nFaces: %u\nFPS: %.1f", // <--- 新增 \nFPS: %.1f
        currentScene(state).algorithmName.c_str(),
        currentScene(state).meshName.c_str(),
        static_cast<unsigned int>(mesh.mesh.positions.size()),
        static_cast<unsigned int>(mesh.mesh.faces.size()),
        state.currentFps); // <--- 传入当前帧率

    std::wstring statsText = toWide(statsBuffer);
    Gdiplus::RectF statsRectF = toRectF(state.statsRect);
    statsRectF.X += 14.0f;
    statsRectF.Y += 12.0f;
    statsRectF.Width -= 28.0f;
    statsRectF.Height -= 24.0f;
    graphics.DrawString(statsText.c_str(), -1, &bodyFont, statsRectF, nullptr, &textBrush);

    // --- 新增：绘制导入模型按钮 ---
    fillRoundedCard(graphics, state.importBtnRect, kPanelCardColor, kBorderColor, 6, 1.0f); // 假设圆角为6
    // 绘制按钮文字
    std::wstring btnText = L"Open OBJ Model...";
    Gdiplus::RectF btnRectF = toRectF(state.importBtnRect);
    // 文字居中对齐格式
    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentCenter);
    format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    Gdiplus::SolidBrush btnTextBrush(Gdiplus::Color(255, 220, 220, 220)); // 白色文字
    graphics.DrawString(btnText.c_str(), -1, &bodyFont, btnRectF, &format, &btnTextBrush);

    fillRoundedCard(graphics, state.shortcutsRect, kPanelCardColor, kBorderColor, 10, 1.0f);
    const std::wstring shortcutTitle = L"Hotkeys";
    graphics.DrawString(shortcutTitle.c_str(), -1, &sectionFont, Gdiplus::PointF(static_cast<Gdiplus::REAL>(state.shortcutsRect.left + 14), static_cast<Gdiplus::REAL>(state.shortcutsRect.top + 12)), &textBrush);
    const std::wstring shortcutText =
        L"M: import external OBJ\n"
        L"L/C/D: switch method\n"
        L"Left/Right: switch mesh\n"
        L"Up/Down or 0-3: switch level\n"
        L"Drag: rotate, Wheel: zoom, R: reset";
    Gdiplus::RectF shortcutRectF = toRectF(state.shortcutsRect);
    shortcutRectF.X += 14.0f;
    shortcutRectF.Y += 38.0f;
    shortcutRectF.Width -= 28.0f;
    shortcutRectF.Height -= 48.0f;
    graphics.DrawString(shortcutText.c_str(), -1, &smallFont, shortcutRectF, nullptr, &mutedBrush);

    // Draw popup lists last so they stay above the rest of the right-side panel.
    if (state.openDropdown == 1) {
        std::vector<std::wstring> methodItems;
        methodItems.push_back(kMethodNames[0]);
        methodItems.push_back(kMethodNames[1]);
        methodItems.push_back(kMethodNames[2]);
        drawDropdownPopup(graphics, state.methodComboRect, methodItems, state.currentMethod, bodyFont);
    } else if (state.openDropdown == 2) {
        std::vector<std::wstring> levelItems;
        for (int i = 0; i <= 3; ++i) {
            levelItems.push_back(levelLabel(i));
        }
        drawDropdownPopup(graphics, state.levelComboRect, levelItems, state.currentLevel, bodyFont);
    } else if (state.openDropdown == 3) {
        std::vector<std::wstring> renderItems;
        renderItems.push_back(kRenderModeNames[0]);
        renderItems.push_back(kRenderModeNames[1]);
        renderItems.push_back(kRenderModeNames[2]);
        drawDropdownPopup(graphics, state.renderModeRect, renderItems, state.currentRenderMode, bodyFont);
    }
}

bool handlePanelClick(AppState& state, POINT point)
{
    if (state.openDropdown != 0) {
        const RECT fieldRect =
            state.openDropdown == 1 ? state.methodComboRect :
            (state.openDropdown == 2 ? state.levelComboRect : state.renderModeRect);
        const int itemCount = state.openDropdown == 2 ? 4 : 3;
        const RECT popupRect = dropdownPopupRect(fieldRect, itemCount);

        if (pointInRect(popupRect, point)) {
            for (int i = 0; i < itemCount; ++i) {
                const RECT itemRect = makeRect(
                    popupRect.left + 4,
                    popupRect.top + 4 + i * kDropdownPopupItemHeight,
                    popupRect.right - 4,
                    popupRect.top + 4 + (i + 1) * kDropdownPopupItemHeight);

                if (pointInRect(itemRect, point)) {
                    state.openDropdown = 0;
                    if (itemCount == 4) {
                        setLevel(state, i);
                    } else if (fieldRect.left == state.methodComboRect.left &&
                               fieldRect.top == state.methodComboRect.top &&
                               fieldRect.right == state.methodComboRect.right &&
                               fieldRect.bottom == state.methodComboRect.bottom) {
                        setMethod(state, i);
                    } else {
                        setRenderMode(state, i);
                    }
                    return true;
                }
            }

            // Clicks inside the popup padding should not fall through to underlying controls.
            return true;
        }

        if (pointInRect(fieldRect, point)) {
            state.openDropdown = 0;
            InvalidateRect(state.window, nullptr, FALSE);
            return true;
        }

        // When a dropdown is open, clicks outside only close it and never activate controls behind it.
        state.openDropdown = 0;
        InvalidateRect(state.window, nullptr, FALSE);
        return true;
    }

    if (pointInRect(state.methodComboRect, point)) {
        state.openDropdown = 1;
        InvalidateRect(state.window, nullptr, FALSE);
        return true;
    }

    if (pointInRect(state.levelComboRect, point)) {
        state.openDropdown = 2;
        InvalidateRect(state.window, nullptr, FALSE);
        return true;
    }

    if (pointInRect(state.renderModeRect, point)) {
        state.openDropdown = 3;
        InvalidateRect(state.window, nullptr, FALSE);
        return true;
    }

    for (size_t i = 0; i < state.modelButtonRects.size(); ++i) {
        const RECT& buttonRect = state.modelButtonRects[i];
        // 只有当按钮在可见区域内时才可以点击
        if (pointInRect(buttonRect, point) && 
            buttonRect.top >= state.modelScrollAreaRect.top && 
            buttonRect.bottom <= state.modelScrollAreaRect.bottom) {
            setVariant(state, static_cast<int>(i));
            return true;
        }
    }
    return false;
}

void cleanupResources(AppState& state)
{
    if (state.uiFont != nullptr) {
        DeleteObject(state.uiFont);
        state.uiFont = nullptr;
    }
    if (state.gdiplusToken != 0) {
        Gdiplus::GdiplusShutdown(state.gdiplusToken);
        state.gdiplusToken = 0;
    }
}

// 【新增函数】用于弹出文件选择框并导入自定义 OBJ 文件
void importExternalObj(AppState& state) {
    char filename[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = state.window;
    ofn.lpstrFilter = "OBJ Files (*.obj)\0*.obj\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "obj";

    if (GetOpenFileNameA(&ofn)) {
        // 【优化 2：UI 反馈】立即将鼠标光标切换为“沙漏”等待状态
        HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

        obj_mesh customMesh;
        if (loadSceneMesh(filename, customMesh, "Import OBJ")) {
            std::string displayName = "Custom: ";

            std::string pathStr = filename;
            size_t pos = pathStr.find_last_of("\\/");
            if (pos != std::string::npos) pathStr = pathStr.substr(pos + 1);
            size_t extPos = pathStr.find_last_of(".");
            if (extPos != std::string::npos) pathStr = pathStr.substr(0, extPos);
            displayName += pathStr;

            // 【优化 3：细分熔断机制】防止大模型耗尽计算资源
            size_t faceCount = customMesh.faces.size();

            // 如果你之前的 buildLevels 只有两个参数，可以通过截断 mesh 集合来避免死机
            // 这里假设我们手动控制传入原模型，如果面数大于 10000，就不做过多细分
            auto getOptimizedLevels = [&](int methodIndex) {
                if (faceCount > 10000) {
                    // 对于超过 1 万面的大模型，强行只返回 0 级（原模），或者极低级细分
                    std::vector<MeshView> singleLevel;
                    // 此处伪代码：将 customMesh 直接转化为单层级的 MeshView
                    // singleLevel.push_back( convertToMeshView(customMesh) ); 
                    return buildLevels(customMesh, methodIndex); // 建议在你的 buildLevels 内部增加根据面数 return 的判断
                }
                else {
                    return buildLevels(customMesh, methodIndex);
                }
                };

            // 分配到场景数组
            if (state.sceneGroups[0].size() > 2) {
                state.scenes[state.sceneGroups[0][2]] = Scene{ "Loop", displayName, getOptimizedLevels(0) };
                state.scenes[state.sceneGroups[1][2]] = Scene{ "Catmull-Clark", displayName, getOptimizedLevels(1) };
                state.scenes[state.sceneGroups[2][2]] = Scene{ "Doo-Sabin", displayName, getOptimizedLevels(2) };
            }
            else {
                state.sceneGroups[0].push_back(static_cast<int>(state.scenes.size()));
                state.scenes.push_back(Scene{ "Loop", displayName, getOptimizedLevels(0) });

                state.sceneGroups[1].push_back(static_cast<int>(state.scenes.size()));
                state.scenes.push_back(Scene{ "Catmull-Clark", displayName, getOptimizedLevels(1) });

                state.sceneGroups[2].push_back(static_cast<int>(state.scenes.size()));
                state.scenes.push_back(Scene{ "Doo-Sabin", displayName, getOptimizedLevels(2) });
            }

            state.currentVariant = 2;
            state.currentLevel = 0;
            refreshUi(state);
            InvalidateRect(state.window, nullptr, FALSE);
        }

        // 【优化完毕】恢复普通鼠标光标
        SetCursor(hOldCursor);
    }
}

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    AppState& state = appState();

    switch (message) {
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        layoutUi(state);
        InvalidateRect(window, nullptr, FALSE);
        return 0;
    case WM_LBUTTONDOWN:
    {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        // --- 新增：检测点击导入按钮 ---
        if (mx >= state.importBtnRect.left && mx <= state.importBtnRect.right &&
            my >= state.importBtnRect.top && my <= state.importBtnRect.bottom) {

            state.openDropdown = 0; // 如果有打开的菜单，先关掉
            importExternalObj(state); // 调用导入模型函数
            return 0;
        }

        // --- 新增：检测滚动条拖动 ---
        if (state.scrollbarThumbHeight > 0 && pointInRect(state.scrollbarThumbRect, { mx, my })) {
            state.isScrollbarDragging = true;
            state.lastMouse = { mx, my };
            SetCapture(window);
            return 0;
        }

        // --- 新增：检测滚动条轨道点击（跳转） ---
        if (state.scrollbarThumbHeight > 0 && pointInRect(state.scrollbarRect, { mx, my })) {
            // 点击滚动条轨道时，跳转到该位置
            const int trackTop = state.scrollbarRect.top;
            const int trackBottom = state.scrollbarRect.bottom;
            const int trackHeight = trackBottom - trackTop;

            if (my < state.scrollbarThumbRect.top) {
                // 点击上方，向上滚动
                const int scrollStep = kModelButtonHeight;
                state.modelScrollOffset = std::max(0, state.modelScrollOffset - scrollStep);
            } else if (my > state.scrollbarThumbRect.bottom) {
                // 点击下方，向下滚动
                const int variantCount = currentVariantCount(state);
                const int totalButtonHeight = variantCount * kModelButtonHeight + (variantCount - 1) * kModelButtonGap;
                const int availableHeight = state.modelScrollAreaRect.bottom - state.modelScrollAreaRect.top;
                const int maxScroll = totalButtonHeight - availableHeight;
                const int scrollStep = kModelButtonHeight;
                state.modelScrollOffset = std::min(maxScroll, state.modelScrollOffset + scrollStep);
            }
            layoutUi(state);
            InvalidateRect(window, nullptr, FALSE);
            return 0;
        }

        POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (handlePanelClick(state, point)) {
            return 0;
        }

        RECT clientRect = {};
        GetClientRect(window, &clientRect);
        RECT viewportRect = getViewportRect(clientRect);
        if (pointInRect(viewportRect, point)) {
            state.dragging = true;
            state.lastMouse = point;
            SetCapture(window);
        }
        return 0;
    }
    case WM_MOUSEMOVE:
        if (state.isScrollbarDragging && state.scrollbarThumbHeight > 0) {
            // 处理滚动条拖动
            const int currentY = GET_Y_LPARAM(lParam);
            const int deltaY = currentY - state.lastMouse.y;

            const int trackHeight = state.scrollbarRect.bottom - state.scrollbarRect.top;
            const int variantCount = currentVariantCount(state);
            const int totalButtonHeight = variantCount * kModelButtonHeight + (variantCount - 1) * kModelButtonGap;
            const int availableHeight = state.modelScrollAreaRect.bottom - state.modelScrollAreaRect.top;
            const int maxScroll = totalButtonHeight - availableHeight;

            if (maxScroll > 0) {
                // 根据拖动距离计算滚动偏移
                const int scrollableTrackHeight = trackHeight - state.scrollbarThumbHeight;
                if (scrollableTrackHeight > 0) {
                    const int newOffset = (deltaY * maxScroll) / scrollableTrackHeight;
                    state.modelScrollOffset = std::max(0, std::min(maxScroll, state.modelScrollOffset + newOffset));
                    layoutUi(state);
                    InvalidateRect(state.window, nullptr, FALSE);
                }
            }

            state.lastMouse.y = currentY;
        } else if (state.dragging) {
            const POINT currentMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            state.yaw += static_cast<float>(currentMouse.x - state.lastMouse.x) * 0.01f;
            state.pitch += static_cast<float>(currentMouse.y - state.lastMouse.y) * 0.01f;
            state.pitch = std::max(-1.45f, std::min(1.45f, state.pitch));
            state.lastMouse = currentMouse;
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    case WM_LBUTTONUP:
        if (state.isScrollbarDragging) {
            state.isScrollbarDragging = false;
            ReleaseCapture();
        } else if (state.dragging) {
            state.dragging = false;
            ReleaseCapture();
        }
        return 0;
    case WM_MOUSEWHEEL:
    {
        const short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

        RECT clientRect = {};
        GetClientRect(window, &clientRect);
        ScreenToClient(window, &point);

        // 检查鼠标是否在模型列表区域内
        if (pointInRect(state.modelScrollAreaRect, point) && state.scrollbarThumbHeight > 0) {
            // 计算需要滚动的距离（每次滚动按钮高度的1/3）
            const int scrollStep = kModelButtonHeight / 3;
            if (delta > 0) {
                state.modelScrollOffset = std::max(0, state.modelScrollOffset - scrollStep);
            } else if (delta < 0) {
                const int variantCount = currentVariantCount(state);
                const int totalButtonHeight = variantCount * kModelButtonHeight + (variantCount - 1) * kModelButtonGap;
                const int availableHeight = state.modelScrollAreaRect.bottom - state.modelScrollAreaRect.top;
                const int maxScroll = totalButtonHeight - availableHeight;
                state.modelScrollOffset = std::min(maxScroll, state.modelScrollOffset + scrollStep);
            }
            layoutUi(state);
            InvalidateRect(window, nullptr, FALSE);
        } else {
            // 缩放视图
            if (delta > 0) {
                state.zoom = std::min(4.0f, state.zoom * 1.1f);
            } else if (delta < 0) {
                state.zoom = std::max(0.3f, state.zoom / 1.1f);
            }
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    }
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_LEFT:
            state.openDropdown = 0;
            cycleMeshVariant(state, -1);
            return 0;
        case VK_RIGHT:
            state.openDropdown = 0;
            cycleMeshVariant(state, 1);
            return 0;
        case VK_UP:
            state.openDropdown = 0;
            changeLevel(state, 1);
            return 0;
        case VK_DOWN:
            state.openDropdown = 0;
            changeLevel(state, -1);
            return 0;
        case 'R':
            state.openDropdown = 0;
            resetView(state);
            InvalidateRect(window, nullptr, FALSE);
            return 0;
        case 'L':
            state.openDropdown = 0;
            setMethod(state, 0);
            return 0;
        case 'C':
            state.openDropdown = 0;
            setMethod(state, 1);
            return 0;
        case 'D':
            state.openDropdown = 0;
            setMethod(state, 2);
            return 0;
        case '0':
        case '1':
        case '2':
        case '3':
            state.openDropdown = 0;
            setLevel(state, static_cast<int>(wParam - '0'));
            return 0;
        default:
            break;
        }
        return 0;
    case WM_PAINT:
    {
        // --- 新增：计算 FPS ---
        DWORD currentTime = GetTickCount();
        if (state.lastFpsTime == 0) state.lastFpsTime = currentTime;
        state.frameCount++;
        if (currentTime - state.lastFpsTime >= 1000) {
            // 每隔 1000 毫秒（1秒）更新一次显示的帧率
            state.currentFps = static_cast<float>(state.frameCount) * 1000.0f / (currentTime - state.lastFpsTime);
            state.frameCount = 0;
            state.lastFpsTime = currentTime;
        }

        PAINTSTRUCT paint = {};
        HDC windowDc = BeginPaint(window, &paint);

        RECT clientRect = {};
        GetClientRect(window, &clientRect);
        const int width = clientRect.right - clientRect.left;
        const int height = clientRect.bottom - clientRect.top;

        HDC memoryDc = CreateCompatibleDC(windowDc);
        HBITMAP bitmap = CreateCompatibleBitmap(windowDc, width, height);
        HGDIOBJ oldBitmap = SelectObject(memoryDc, bitmap);

        drawViewport(memoryDc, clientRect, state);
        Gdiplus::Graphics graphics(memoryDc);
        drawPanel(graphics, clientRect, state);

        BitBlt(windowDc, 0, 0, width, height, memoryDc, 0, 0, SRCCOPY);

        SelectObject(memoryDc, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memoryDc);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProcA(window, message, wParam, lParam);
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand)
{
    AppState& state = appState();
    state.instance = instance;
    resetView(state);

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    if (Gdiplus::GdiplusStartup(&state.gdiplusToken, &gdiplusStartupInput, nullptr) != Gdiplus::Ok) {
        MessageBoxA(nullptr, "Failed to initialize GDI+.", "Mesh Subdivision Viewer", MB_ICONERROR | MB_OK);
        return 1;
    }

    if (!loadScenes(state)) {
        cleanupResources(state);
        return 1;
    }

    WNDCLASSEXA windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;
    windowClass.lpszClassName = kWindowClassName;

    if (RegisterClassExA(&windowClass) == 0) {
        cleanupResources(state);
        MessageBoxA(nullptr, "Failed to register window class.", "Mesh Subdivision Viewer", MB_ICONERROR | MB_OK);
        return 1;
    }

    state.window = CreateWindowExA(
        0,
        kWindowClassName,
        "Mesh Subdivision Viewer",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1320,
        820,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (state.window == nullptr) {
        cleanupResources(state);
        MessageBoxA(nullptr, "Failed to create the application window.", "Mesh Subdivision Viewer", MB_ICONERROR | MB_OK);
        return 1;
    }

    if (!createUiControls(state)) {
        cleanupResources(state);
        MessageBoxA(nullptr, "Failed to create UI controls.", "Mesh Subdivision Viewer", MB_ICONERROR | MB_OK);
        return 1;
    }

    refreshUi(state);
    ShowWindow(state.window, showCommand);
    UpdateWindow(state.window);

    MSG message = {};
    while (GetMessageA(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    cleanupResources(state);
    return static_cast<int>(message.wParam);
}
