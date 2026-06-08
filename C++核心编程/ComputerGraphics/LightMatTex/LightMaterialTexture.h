#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string>

inline constexpr float kPi = 3.1415926535f;
inline constexpr int kTimerId = 1;
inline constexpr int kSceneMinWidth = 420;
inline constexpr wchar_t kMainClassName[] = L"LightMaterialTextureMainWindow";
inline constexpr wchar_t kSceneClassName[] = L"LightMaterialTextureSceneView";

enum ControlId {
    IDC_SCENE = 100,
    IDC_TARGET_SELECT,
    IDC_TARGET_LABEL,
    IDC_LIGHT1_INTENSITY,
    IDC_LIGHT1_INTENSITY_VALUE,
    IDC_LIGHT1_COLOR,
    IDC_LIGHT1_ENABLED,
    IDC_LIGHT1_POS_X,
    IDC_LIGHT1_POS_Y,
    IDC_LIGHT1_POS_Z,
    IDC_LIGHT1_POS_LABEL,
    IDC_LIGHT2_INTENSITY,
    IDC_LIGHT2_INTENSITY_VALUE,
    IDC_LIGHT2_COLOR,
    IDC_LIGHT2_ENABLED,
    IDC_LIGHT2_POS_X,
    IDC_LIGHT2_POS_Y,
    IDC_LIGHT2_POS_Z,
    IDC_LIGHT2_POS_LABEL,
    IDC_MainLight,
    IDC_AMBIENT_INTENSITY,
    IDC_AMBIENT_INTENSITY_VALUE,
    IDC_DIFFUSE_COEFF,
    IDC_DIFFUSE_COEFF_VALUE,
    IDC_SPECULAR_COEFF,
    IDC_SPECULAR_COEFF_VALUE,
    IDC_SHININESS,
    IDC_SHININESS_VALUE,
    IDC_MATERIAL_COLOR,
    IDC_OPACITY,
    IDC_OPACITY_VALUE,
    IDC_DIFFUSE_MAP,
    IDC_DIFFUSE_MAP_COMBO,
    IDC_NORMAL_MAP,
    IDC_NORMAL_MAP_COMBO,
    IDC_TEXTURE_SCALE,
    IDC_TEXTURE_SCALE_VALUE,
    IDC_RESET_BUTTON
};

// 增加编辑目标类型(3个模型+5面墙)
enum class TargetType {
    ModelCube = 0,
    ModelSphere,
    ModelTorus,
    WallLeft,
    WallRight,
    WallBack,
    WallTop,
    WallBottom,
    Count
};

struct Vec2 { float x = 0.0f; float y = 0.0f; };
struct Vec3 { float x = 0.0f; float y = 0.0f; float z = 0.0f; };

struct LightState {
    bool enabled = true;
    float intensity = 1.0f;
    COLORREF color = RGB(255, 255, 255);
    Vec3 position{};
};

struct MaterialState {
    COLORREF color = RGB(200, 200, 200);
    COLORREF ambientColor = RGB(100, 100, 100);
    COLORREF specularColor = RGB(255, 255, 255);
    COLORREF emissiveColor = RGB(0, 0, 0);
    float ambient = 0.2f;
    float diffuse = 0.8f;
    float specular = 0.5f;
    float shininess = 30.0f;
    float emissive = 0.0f;
    float opacity = 1.0f;
    float normalStrength = 0.5f;
    bool diffuseMapEnabled = false;
    bool normalMapEnabled = false;
    std::wstring diffuseMapFile = L"";
    std::wstring normalMapFile = L"";
    float textureScale = 1.0f;
};

struct WallState {
    COLORREF color = RGB(220, 220, 220);
    bool textureEnabled = false;
    std::wstring textureFile = L"";
};

struct Controls {
    HWND scene = nullptr;
    HWND targetLabel = nullptr;
    HWND targetSelect = nullptr;
    HWND lightingGroup = nullptr;
    HWND materialGroup = nullptr;
    HWND textureGroup = nullptr;
    HWND light1IntensityLabel = nullptr, light1Intensity = nullptr, light1IntensityValue = nullptr;
    HWND light1PosLabel = nullptr, light1PosX = nullptr, light1PosY = nullptr, light1PosZ = nullptr;
    HWND light1Color = nullptr, light1Enabled = nullptr;
    HWND light2IntensityLabel = nullptr, light2Intensity = nullptr, light2IntensityValue = nullptr;
    HWND light2PosLabel = nullptr, light2PosX = nullptr, light2PosY = nullptr, light2PosZ = nullptr;
    HWND light2Color = nullptr, light2Enabled = nullptr;
    HWND MainLight = nullptr;
    HWND ambientIntensityLabel = nullptr, ambientIntensity = nullptr, ambientIntensityValue = nullptr;
    HWND diffuseCoeffLabel = nullptr, diffuseCoeff = nullptr, diffuseCoeffValue = nullptr;
    HWND specularCoeffLabel = nullptr, specularCoeff = nullptr, specularCoeffValue = nullptr;
    HWND shininessLabel = nullptr, shininess = nullptr, shininessValue = nullptr;
    HWND materialColor = nullptr;
    HWND opacityLabel = nullptr, opacity = nullptr, opacityValue = nullptr;
    HWND diffuseMap = nullptr, diffuseMapCombo = nullptr;
    HWND normalMap = nullptr, normalMapCombo = nullptr;
    HWND textureScaleLabel = nullptr, textureScale = nullptr, textureScaleValue = nullptr;
    HWND resetButton = nullptr;
};

struct AppState {
    HWND mainWindow = nullptr;
    TargetType activeTarget = TargetType::ModelCube;
    LightState light1;
    LightState light2;
    int mainLightIndex = 0;  // 0 = Light 1, 1 = Light 2
    float ambientLightIntensity = 0.2f;

    MaterialState objectMaterials[3];
    WallState walls[5];

    Controls controls;
    float autoRotation = 0.0f;
    float userYaw = 0.0f;
    float userPitch = 0.1f;
    float cameraDistance = 9.0f;
    bool dragging = false;
    POINT lastMouse{};
    COLORREF customColors[16]{};
    HFONT sectionFont = nullptr;
};

extern AppState g_app;

std::wstring FormatFloat(float value, int decimals = 1);
void LoadDefaults();
void SyncControlsFromState();
void UpdateValuesFromTrackbars();
void UpdateCheckboxState();
void ChooseAndSetColor(HWND owner, COLORREF& targetColor);
void CreateUi(HWND hwnd);
void LayoutControls(HWND hwnd);
void PaintScene(HWND hwnd, HDC hdc);
void DestroySceneRenderer();

LRESULT CALLBACK SceneWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
bool RegisterWindowClasses(HINSTANCE instance);
