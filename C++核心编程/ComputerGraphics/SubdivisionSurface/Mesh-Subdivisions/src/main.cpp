#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>

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
    int top = panelRect.top + 82;

    state.methodComboRect = makeRect(left, top, left + width, top + kControlHeight);
    top += 74;
    state.levelComboRect = makeRect(left, top, left + width, top + kControlHeight);
    top += 74;
    state.renderModeRect = makeRect(left, top, left + width, top + kControlHeight);
    top += 74;

    const int variantCount = currentVariantCount(state);
    state.modelButtonRects.assign(static_cast<size_t>(variantCount), RECT{});
    for (int i = 0; i < variantCount; ++i) {
        state.modelButtonRects[static_cast<size_t>(i)] = makeRect(left, top, left + width, top + kModelButtonHeight);
        top += kModelButtonHeight + kModelButtonGap;
    }

    state.statsRect = makeRect(left, top + 8, left + width, top + 120);
    state.shortcutsRect = makeRect(left, panelRect.bottom - 150, left + width, panelRect.bottom - 24);

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
        graphics.DrawString(L"Mesh", -1, &sectionFont, Gdiplus::PointF(static_cast<Gdiplus::REAL>(state.modelButtonRects[0].left), static_cast<Gdiplus::REAL>(state.modelButtonRects[0].top - 26)), &mutedBrush);
    }

    const std::vector<int>& group = state.sceneGroups[static_cast<size_t>(state.currentMethod)];
    for (size_t i = 0; i < state.modelButtonRects.size(); ++i) {
        const RECT& buttonRect = state.modelButtonRects[i];
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

    fillRoundedCard(graphics, state.statsRect, kPanelCardColor, kBorderColor, 10, 1.0f);
    const MeshView& mesh = currentMesh(state);
    char statsBuffer[256] = {};
    std::snprintf(
        statsBuffer,
        sizeof(statsBuffer),
        "Algorithm: %s\nMesh: %s\nVertices: %u\nFaces: %u",
        currentScene(state).algorithmName.c_str(),
        currentScene(state).meshName.c_str(),
        static_cast<unsigned int>(mesh.mesh.positions.size()),
        static_cast<unsigned int>(mesh.mesh.faces.size()));
    std::wstring statsText = toWide(statsBuffer);
    Gdiplus::RectF statsRectF = toRectF(state.statsRect);
    statsRectF.X += 14.0f;
    statsRectF.Y += 12.0f;
    statsRectF.Width -= 28.0f;
    statsRectF.Height -= 24.0f;
    graphics.DrawString(statsText.c_str(), -1, &bodyFont, statsRectF, nullptr, &textBrush);

    fillRoundedCard(graphics, state.shortcutsRect, kPanelCardColor, kBorderColor, 10, 1.0f);
    const std::wstring shortcutTitle = L"Hotkeys";
    graphics.DrawString(shortcutTitle.c_str(), -1, &sectionFont, Gdiplus::PointF(static_cast<Gdiplus::REAL>(state.shortcutsRect.left + 14), static_cast<Gdiplus::REAL>(state.shortcutsRect.top + 12)), &textBrush);
    const std::wstring shortcutText =
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
        if (pointInRect(state.modelButtonRects[i], point)) {
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
        if (state.dragging) {
            const POINT currentMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            state.yaw += static_cast<float>(currentMouse.x - state.lastMouse.x) * 0.01f;
            state.pitch += static_cast<float>(currentMouse.y - state.lastMouse.y) * 0.01f;
            state.pitch = std::max(-1.45f, std::min(1.45f, state.pitch));
            state.lastMouse = currentMouse;
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    case WM_LBUTTONUP:
        if (state.dragging) {
            state.dragging = false;
            ReleaseCapture();
        }
        return 0;
    case WM_MOUSEWHEEL:
    {
        const short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0) {
            state.zoom = std::min(4.0f, state.zoom * 1.1f);
        } else if (delta < 0) {
            state.zoom = std::max(0.3f, state.zoom / 1.1f);
        }
        InvalidateRect(window, nullptr, FALSE);
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
