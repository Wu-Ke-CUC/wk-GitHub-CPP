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
    IDC_OBJECT_CUBE,
    IDC_OBJECT_SPHERE,
    IDC_OBJECT_TORUS,
    IDC_LIGHT1_INTENSITY,
    IDC_LIGHT1_INTENSITY_VALUE,
    IDC_LIGHT1_COLOR,
    IDC_LIGHT1_ENABLED,
    IDC_LIGHT1_POS_X,
    IDC_LIGHT1_POS_X_VALUE,
    IDC_LIGHT1_POS_Y,
    IDC_LIGHT1_POS_Y_VALUE,
    IDC_LIGHT1_POS_Z,
    IDC_LIGHT1_POS_Z_VALUE,
    IDC_LIGHT2_INTENSITY,
    IDC_LIGHT2_INTENSITY_VALUE,
    IDC_LIGHT2_COLOR,
    IDC_LIGHT2_ENABLED,
    IDC_LIGHT2_POS_X,
    IDC_LIGHT2_POS_X_VALUE,
    IDC_LIGHT2_POS_Y,
    IDC_LIGHT2_POS_Y_VALUE,
    IDC_LIGHT2_POS_Z,
    IDC_LIGHT2_POS_Z_VALUE,
    IDC_AMBIENT_INTENSITY,
    IDC_AMBIENT_INTENSITY_VALUE,
    IDC_AMBIENT_COEFF,
    IDC_AMBIENT_COEFF_VALUE,
    IDC_DIFFUSE_COEFF,
    IDC_DIFFUSE_COEFF_VALUE,
    IDC_SPECULAR_COEFF,
    IDC_SPECULAR_COEFF_VALUE,
    IDC_SHININESS,
    IDC_SHININESS_VALUE,
    IDC_MATERIAL_COLOR,
    IDC_AMBIENT_COLOR,
    IDC_SPECULAR_COLOR,
    IDC_EMISSIVE_COEFF,
    IDC_EMISSIVE_COEFF_VALUE,
    IDC_EMISSIVE_COLOR,
    IDC_OPACITY,
    IDC_OPACITY_VALUE,
    IDC_NORMAL_STRENGTH,
    IDC_NORMAL_STRENGTH_VALUE,
    IDC_DIFFUSE_MAP,
    IDC_DIFFUSE_MAP_COMBO,
    IDC_NORMAL_MAP,
    IDC_NORMAL_MAP_COMBO,
    IDC_TEXTURE_SCALE,
    IDC_TEXTURE_SCALE_VALUE,
    IDC_TEXTURE_BLEND,
    IDC_TEXTURE_BLEND_VALUE,
    IDC_RESET_BUTTON
};

enum class ObjectType {
    Cube,
    Sphere,
    Torus
};

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct LightState {
    bool enabled = true;
    float intensity = 1.0f;
    COLORREF color = RGB(255, 255, 255);
    Vec3 position {};
};

struct MaterialState {
    COLORREF color = RGB(79, 172, 254);
    COLORREF ambientColor = RGB(48, 90, 140);
    COLORREF specularColor = RGB(255, 255, 255);
    COLORREF emissiveColor = RGB(24, 42, 60);
    float ambient = 0.2f;
    float diffuse = 0.8f;
    float specular = 0.5f;
    float shininess = 30.0f;
    float emissive = 0.0f;
    float opacity = 1.0f;
    float normalStrength = 0.25f;
    bool diffuseMapEnabled = true;
    bool normalMapEnabled = false;
    std::wstring diffuseMapFile = L"";
    std::wstring normalMapFile = L"";
    float textureScale = 1.0f;
    float textureBlend = 1.0f;
};

struct Controls {
    HWND scene = nullptr;
    HWND lightingGroup = nullptr;
    HWND materialGroup = nullptr;
    HWND textureGroup = nullptr;
    HWND objectLabel = nullptr;
    HWND objectCube = nullptr;
    HWND objectSphere = nullptr;
    HWND objectTorus = nullptr;
    HWND light1IntensityLabel = nullptr;
    HWND light1Intensity = nullptr;
    HWND light1IntensityValue = nullptr;
    HWND light1Color = nullptr;
    HWND light1Enabled = nullptr;
    HWND light1PosXLabel = nullptr;
    HWND light1PosX = nullptr;
    HWND light1PosXValue = nullptr;
    HWND light1PosYLabel = nullptr;
    HWND light1PosY = nullptr;
    HWND light1PosYValue = nullptr;
    HWND light1PosZLabel = nullptr;
    HWND light1PosZ = nullptr;
    HWND light1PosZValue = nullptr;
    HWND light2IntensityLabel = nullptr;
    HWND light2Intensity = nullptr;
    HWND light2IntensityValue = nullptr;
    HWND light2Color = nullptr;
    HWND light2Enabled = nullptr;
    HWND light2PosXLabel = nullptr;
    HWND light2PosX = nullptr;
    HWND light2PosXValue = nullptr;
    HWND light2PosYLabel = nullptr;
    HWND light2PosY = nullptr;
    HWND light2PosYValue = nullptr;
    HWND light2PosZLabel = nullptr;
    HWND light2PosZ = nullptr;
    HWND light2PosZValue = nullptr;
    HWND ambientIntensityLabel = nullptr;
    HWND ambientIntensity = nullptr;
    HWND ambientIntensityValue = nullptr;
    HWND ambientCoeffLabel = nullptr;
    HWND ambientCoeff = nullptr;
    HWND ambientCoeffValue = nullptr;
    HWND diffuseCoeffLabel = nullptr;
    HWND diffuseCoeff = nullptr;
    HWND diffuseCoeffValue = nullptr;
    HWND specularCoeffLabel = nullptr;
    HWND specularCoeff = nullptr;
    HWND specularCoeffValue = nullptr;
    HWND shininessLabel = nullptr;
    HWND shininess = nullptr;
    HWND shininessValue = nullptr;
    HWND materialColor = nullptr;
    HWND ambientColor = nullptr;
    HWND specularColor = nullptr;
    HWND emissiveCoeffLabel = nullptr;
    HWND emissiveCoeff = nullptr;
    HWND emissiveCoeffValue = nullptr;
    HWND emissiveColor = nullptr;
    HWND opacityLabel = nullptr;
    HWND opacity = nullptr;
    HWND opacityValue = nullptr;
    HWND normalStrengthLabel = nullptr;
    HWND normalStrength = nullptr;
    HWND normalStrengthValue = nullptr;
    HWND diffuseMap = nullptr;
    HWND diffuseMapCombo = nullptr;
    HWND normalMap = nullptr;
    HWND normalMapCombo = nullptr;
    HWND textureScaleLabel = nullptr;
    HWND textureScale = nullptr;
    HWND textureScaleValue = nullptr;
    HWND textureBlendLabel = nullptr;
    HWND textureBlend = nullptr;
    HWND textureBlendValue = nullptr;
    HWND resetButton = nullptr;
};

struct RenderHit {
    bool hit = false;
    float distance = 0.0f;
    Vec3 position {};
    Vec3 localPosition {};
    Vec3 normal {};
    int materialId = 0;
};

struct AppState {
    HWND mainWindow = nullptr;
    ObjectType activeObject = ObjectType::Cube;
    LightState light1;
    LightState light2;
    float ambientLightIntensity = 0.2f;
    MaterialState material;
    Controls controls;
    float autoRotation = 0.0f;
    float userYaw = 0.4f;
    float userPitch = -0.2f;
    float cameraDistance = 4.7f;
    bool dragging = false;
    POINT lastMouse {};
    COLORREF customColors[16] {};
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
