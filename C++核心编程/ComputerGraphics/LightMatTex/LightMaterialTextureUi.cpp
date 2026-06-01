#define _CRT_SECURE_NO_WARNINGS

#include "LightMaterialTexture.h"
#include "LightMaterialTextureMath.h"

#include <cstdio>
#include <string>
#include <vector>
#include <tuple>

AppState g_app;

namespace {

std::vector<std::pair<std::wstring, std::wstring>> GetTextureFiles(const std::wstring& subDir);

HBRUSH g_uiBackgroundBrush = nullptr;
HBRUSH g_dividerBrush = nullptr;
std::vector<std::pair<std::wstring, std::wstring>> g_diffuseFiles;
std::vector<std::pair<std::wstring, std::wstring>> g_normalFiles;

void EnsureUiBrushes() {
    if (!g_uiBackgroundBrush) {
        g_uiBackgroundBrush = CreateSolidBrush(RGB(18, 28, 40));
    }
    if (!g_dividerBrush) {
        g_dividerBrush = CreateSolidBrush(RGB(50, 74, 95));
    }
}

HWND CreateTrackBar(HWND parent, int id) {
    HWND hwnd = CreateWindowExW(
        0, TRACKBAR_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_TOOLTIPS,
        0, 0, 100, 24,
        parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr), nullptr);
    SendMessageW(hwnd, TBM_SETPAGESIZE, 0, 5);
    return hwnd;
}

HWND CreateLabel(HWND parent, const wchar_t* text, int id = 0, DWORD style = WS_CHILD | WS_VISIBLE) {
    return CreateWindowExW(
        0, L"STATIC", text, style,
        0, 0, 100, 20,
        parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr), nullptr);
}

HWND CreateButton(HWND parent, const wchar_t* text, int id, DWORD style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON) {
    return CreateWindowExW(
        0, L"BUTTON", text, style,
        0, 0, 100, 24,
        parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr), nullptr);
}

HWND CreateGroupBox(HWND parent, const wchar_t* text) {
    return CreateWindowExW(
        0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 0, 100, 100,
        parent, nullptr,
        GetModuleHandleW(nullptr), nullptr);
}

void SetSliderRange(HWND slider, int minValue, int maxValue, int currentValue) {
    SendMessageW(slider, TBM_SETRANGEMIN, TRUE, minValue);
    SendMessageW(slider, TBM_SETRANGEMAX, TRUE, maxValue);
    SendMessageW(slider, TBM_SETPOS, TRUE, currentValue);
}

int GetSliderPos(HWND slider) {
    return static_cast<int>(SendMessageW(slider, TBM_GETPOS, 0, 0));
}

void SetLabelText(HWND hwnd, const std::wstring& text) {
    SetWindowTextW(hwnd, text.c_str());
}

void OnCommand(HWND hwnd, int id) {
    switch (id) {
    case IDC_OBJECT_CUBE:
        g_app.activeObject = ObjectType::Cube;
        break;
    case IDC_OBJECT_SPHERE:
        g_app.activeObject = ObjectType::Sphere;
        break;
    case IDC_OBJECT_TORUS:
        g_app.activeObject = ObjectType::Torus;
        break;
    case IDC_LIGHT1_ENABLED:
    case IDC_LIGHT2_ENABLED:
    case IDC_DIFFUSE_MAP:
    case IDC_NORMAL_MAP:
        UpdateCheckboxState();
        InvalidateRect(g_app.controls.scene, nullptr, FALSE);
        UpdateWindow(g_app.controls.scene);
        break;
    case IDC_LIGHT1_COLOR:
        ChooseAndSetColor(hwnd, g_app.light1.color);
        break;
    case IDC_LIGHT2_COLOR:
        ChooseAndSetColor(hwnd, g_app.light2.color);
        break;
    case IDC_MATERIAL_COLOR:
        ChooseAndSetColor(hwnd, g_app.material.color);
        break;
    case IDC_RESET_BUTTON: {
        LoadDefaults();
        SyncControlsFromState();
        break;
    }
    default:
        break;
    }

    InvalidateRect(g_app.controls.scene, nullptr, FALSE);
    UpdateWindow(g_app.controls.scene);
}

std::vector<std::pair<std::wstring, std::wstring>> GetTextureFiles(const std::wstring& subDir) {
    std::vector<std::pair<std::wstring, std::wstring>> files;
    WIN32_FIND_DATAW fd;
    const wchar_t* patterns[] = { L"\\*.jpg", L"\\*.png", L"\\*.bmp" };
    for (const wchar_t* pattern : patterns) {
        std::wstring searchPath = subDir + pattern;
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::wstring fileName = fd.cFileName;
                    size_t lastDot = fileName.find_last_of(L".");
                    std::wstring nameNoExt = (lastDot != std::wstring::npos) ? fileName.substr(0, lastDot) : fileName;
                    std::wstring fullPath = subDir + L"\\" + fileName;
                    files.push_back({ nameNoExt, fullPath });
                }
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }
    }
    return files;
}

} // namespace

std::wstring FormatFloat(float value, int decimals) {
    wchar_t buffer[64] {};
    std::swprintf(buffer, 64, (decimals == 0) ? L"%.0f" : (decimals == 2 ? L"%.2f" : L"%.1f"), value);
    return buffer;
}

void LoadDefaults() {
    g_app.activeObject = ObjectType::Cube;

    g_app.light1.enabled = true;
    g_app.light1.intensity = 1.0f;
    g_app.light1.color = RGB(255, 255, 255);
    g_app.light1.position = { 2.6f, 2.8f, 3.2f };

    g_app.light2.enabled = true;
    g_app.light2.intensity = 0.65f;
    g_app.light2.color = RGB(255, 170, 92);
    g_app.light2.position = { -2.4f, 1.7f, 2.8f };

    g_app.ambientLightIntensity = 0.18f;
    g_app.material.color = RGB(79, 172, 254);
    g_app.material.ambientColor = RGB(48, 90, 140);
    g_app.material.specularColor = RGB(255, 255, 255);
    g_app.material.emissiveColor = RGB(24, 42, 60);
    g_app.material.ambient = 0.18f;
    g_app.material.diffuse = 0.92f;
    g_app.material.specular = 0.65f;
    g_app.material.shininess = 42.0f;
    g_app.material.emissive = 0.0f;
    g_app.material.opacity = 1.0f;
    g_app.material.normalStrength = 0.25f;
    g_app.material.diffuseMapEnabled = true;
    g_app.material.normalMapEnabled = false;

    g_diffuseFiles = GetTextureFiles(L"texture\\diffuse");
    g_normalFiles = GetTextureFiles(L"texture\\normal");
    
    g_app.material.diffuseMapFile = g_diffuseFiles.empty() ? L"" : g_diffuseFiles[0].second;
    g_app.material.normalMapFile = g_normalFiles.empty() ? L"" : g_normalFiles[0].second;

    auto fillCombo = [&](HWND combo, const std::vector<std::pair<std::wstring, std::wstring>>& files) {
        if (!combo) return;
        SendMessageW(combo, CB_RESETCONTENT, 0, 0);
        for (const auto& f : files) {
            SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(f.first.c_str()));
        }
    };
    fillCombo(g_app.controls.diffuseMapCombo, g_diffuseFiles);
    fillCombo(g_app.controls.normalMapCombo, g_normalFiles);

    g_app.material.textureScale = 1.0f;
    g_app.material.textureBlend = 0.45f;

    g_app.userYaw = 0.4f;
    g_app.userPitch = -0.2f;
    g_app.cameraDistance = 4.7f;
}

void SyncControlsFromState() {
    CheckRadioButton(g_app.mainWindow, IDC_OBJECT_CUBE, IDC_OBJECT_TORUS,
        g_app.activeObject == ObjectType::Cube ? IDC_OBJECT_CUBE :
        g_app.activeObject == ObjectType::Sphere ? IDC_OBJECT_SPHERE : IDC_OBJECT_TORUS);

    SetSliderRange(g_app.controls.light1Intensity, 0, 200, static_cast<int>(g_app.light1.intensity * 100.0f));
    SetSliderRange(g_app.controls.light2Intensity, 0, 200, static_cast<int>(g_app.light2.intensity * 100.0f));
    SetSliderRange(g_app.controls.ambientIntensity, 0, 100, static_cast<int>(g_app.ambientLightIntensity * 100.0f));
    SetSliderRange(g_app.controls.diffuseCoeff, 0, 100, static_cast<int>(g_app.material.diffuse * 100.0f));
    SetSliderRange(g_app.controls.specularCoeff, 0, 100, static_cast<int>(g_app.material.specular * 100.0f));
    SetSliderRange(g_app.controls.shininess, 1, 200, static_cast<int>(g_app.material.shininess));
    SetSliderRange(g_app.controls.opacity, 5, 100, static_cast<int>(g_app.material.opacity * 100.0f));
    SetSliderRange(g_app.controls.textureScale, 1, 50, static_cast<int>(g_app.material.textureScale * 10.0f));

    Button_SetCheck(g_app.controls.light1Enabled, g_app.light1.enabled ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(g_app.controls.light2Enabled, g_app.light2.enabled ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(g_app.controls.diffuseMap, g_app.material.diffuseMapEnabled ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(g_app.controls.normalMap, g_app.material.normalMapEnabled ? BST_CHECKED : BST_UNCHECKED);

    SetLabelText(g_app.controls.light1IntensityValue, FormatFloat(g_app.light1.intensity));
    SetLabelText(g_app.controls.light2IntensityValue, FormatFloat(g_app.light2.intensity));
    SetLabelText(g_app.controls.ambientIntensityValue, FormatFloat(g_app.ambientLightIntensity));
    SetLabelText(g_app.controls.diffuseCoeffValue, FormatFloat(g_app.material.diffuse));
    SetLabelText(g_app.controls.specularCoeffValue, FormatFloat(g_app.material.specular));
    SetLabelText(g_app.controls.shininessValue, FormatFloat(g_app.material.shininess, 0));
    SetLabelText(g_app.controls.opacityValue, FormatFloat(g_app.material.opacity));
    auto syncCombo = [](HWND combo, const std::wstring& selectedPath, const std::vector<std::pair<std::wstring, std::wstring>>& files) {
        for (size_t i = 0; i < files.size(); ++i) {
            if (files[i].second == selectedPath || (files[i].second.empty() && selectedPath.empty())) {
                SendMessageW(combo, CB_SETCURSEL, i, 0);
                return;
            }
        }
        SendMessageW(combo, CB_SETCURSEL, 0, 0);
    };
    syncCombo(g_app.controls.diffuseMapCombo, g_app.material.diffuseMapFile, g_diffuseFiles);
    syncCombo(g_app.controls.normalMapCombo, g_app.material.normalMapFile, g_normalFiles);

    SetLabelText(g_app.controls.textureScaleValue, FormatFloat(g_app.material.textureScale));

    InvalidateRect(g_app.controls.scene, nullptr, FALSE);
    UpdateWindow(g_app.controls.scene);
}

void UpdateValuesFromTrackbars() {
    g_app.light1.intensity = GetSliderPos(g_app.controls.light1Intensity) / 100.0f;
    g_app.light2.intensity = GetSliderPos(g_app.controls.light2Intensity) / 100.0f;
    g_app.ambientLightIntensity = GetSliderPos(g_app.controls.ambientIntensity) / 100.0f;
    g_app.material.diffuse = GetSliderPos(g_app.controls.diffuseCoeff) / 100.0f;
    g_app.material.specular = GetSliderPos(g_app.controls.specularCoeff) / 100.0f;
    g_app.material.shininess = static_cast<float>(GetSliderPos(g_app.controls.shininess));
    g_app.material.opacity = GetSliderPos(g_app.controls.opacity) / 100.0f;
    g_app.material.textureScale = GetSliderPos(g_app.controls.textureScale) / 10.0f;

    SetLabelText(g_app.controls.light1IntensityValue, FormatFloat(g_app.light1.intensity));
    SetLabelText(g_app.controls.light2IntensityValue, FormatFloat(g_app.light2.intensity));
    SetLabelText(g_app.controls.ambientIntensityValue, FormatFloat(g_app.ambientLightIntensity));
    SetLabelText(g_app.controls.diffuseCoeffValue, FormatFloat(g_app.material.diffuse));
    SetLabelText(g_app.controls.specularCoeffValue, FormatFloat(g_app.material.specular));
    SetLabelText(g_app.controls.shininessValue, FormatFloat(g_app.material.shininess, 0));
    SetLabelText(g_app.controls.opacityValue, FormatFloat(g_app.material.opacity));
    SetLabelText(g_app.controls.textureScaleValue, FormatFloat(g_app.material.textureScale));
}

void UpdateCheckboxState() {
    g_app.light1.enabled = Button_GetCheck(g_app.controls.light1Enabled) == BST_CHECKED;
    g_app.light2.enabled = Button_GetCheck(g_app.controls.light2Enabled) == BST_CHECKED;
    g_app.material.diffuseMapEnabled = Button_GetCheck(g_app.controls.diffuseMap) == BST_CHECKED;
    g_app.material.normalMapEnabled = Button_GetCheck(g_app.controls.normalMap) == BST_CHECKED;
}

void ChooseAndSetColor(HWND owner, COLORREF& targetColor) {
    CHOOSECOLORW cc {};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = owner;
    cc.rgbResult = targetColor;
    cc.lpCustColors = g_app.customColors;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColorW(&cc)) {
        targetColor = cc.rgbResult;
        InvalidateRect(g_app.controls.scene, nullptr, FALSE);
        UpdateWindow(g_app.controls.scene);
    }
}

void LayoutControls(HWND hwnd) {
    RECT rc {};
    GetClientRect(hwnd, &rc);

    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    int controlWidth = 330;
    int gap = 14;
    int sceneWidth = std::max(kSceneMinWidth, width - controlWidth - gap * 3);
    sceneWidth = std::min(sceneWidth, width - gap * 2);
    int sceneHeight = height - gap * 2;
    int controlsX = gap + sceneWidth + gap;
    int innerX = controlsX + 10;
    int innerWidth = controlWidth - 20;

    MoveWindow(g_app.controls.scene, gap, gap, sceneWidth, sceneHeight, TRUE);

    auto moveLine = [&](HWND label, HWND value, HWND slider, int y) {
        MoveWindow(label, innerX, y, innerWidth - 54, 20, TRUE);
        MoveWindow(value, innerX + innerWidth - 48, y, 48, 20, TRUE);
        MoveWindow(slider, innerX, y + 18, innerWidth, 26, TRUE);
    };

    auto moveButtonPair = [&](HWND left, HWND right, int y) {
        MoveWindow(left, innerX, y, 146, 24, TRUE);
        MoveWindow(right, innerX + innerWidth - 146, y, 146, 24, TRUE);
    };

    int y = gap;
    MoveWindow(g_app.controls.objectLabel, controlsX, y, 100, 20, TRUE);

    MoveWindow(g_app.controls.objectCube, controlsX, y + 24, 96, 24, TRUE);
    MoveWindow(g_app.controls.objectSphere, controlsX + 108, y + 24, 96, 24, TRUE);
    MoveWindow(g_app.controls.objectTorus, controlsX + 216, y + 24, 96, 24, TRUE);
    y += 60;

    const int lightingHeight = 242;
    const int materialHeight = 274;
    const int textureHeight = 196;

    MoveWindow(g_app.controls.lightingGroup, controlsX, y, controlWidth, lightingHeight, TRUE);
    int groupY = y + 24;
    moveLine(g_app.controls.light1IntensityLabel, g_app.controls.light1IntensityValue, g_app.controls.light1Intensity, groupY);
    groupY += 48;
    moveButtonPair(g_app.controls.light1Color, g_app.controls.light1Enabled, groupY);
    groupY += 36;
    moveLine(g_app.controls.light2IntensityLabel, g_app.controls.light2IntensityValue, g_app.controls.light2Intensity, groupY);
    groupY += 48;
    moveButtonPair(g_app.controls.light2Color, g_app.controls.light2Enabled, groupY);
    groupY += 36;
    moveLine(g_app.controls.ambientIntensityLabel, g_app.controls.ambientIntensityValue, g_app.controls.ambientIntensity, groupY);
    y += lightingHeight + 42;

    MoveWindow(g_app.controls.materialGroup, controlsX, y, controlWidth, materialHeight, TRUE);
    groupY = y + 24;
    moveLine(g_app.controls.diffuseCoeffLabel, g_app.controls.diffuseCoeffValue, g_app.controls.diffuseCoeff, groupY);
    groupY += 48;
    moveLine(g_app.controls.specularCoeffLabel, g_app.controls.specularCoeffValue, g_app.controls.specularCoeff, groupY);
    groupY += 48;
    moveLine(g_app.controls.shininessLabel, g_app.controls.shininessValue, g_app.controls.shininess, groupY);
    groupY += 48;
    moveLine(g_app.controls.opacityLabel, g_app.controls.opacityValue, g_app.controls.opacity, groupY);
    groupY += 48;
    MoveWindow(g_app.controls.materialColor, innerX, groupY, innerWidth, 28, TRUE);
    y += materialHeight + 42;

    MoveWindow(g_app.controls.textureGroup, controlsX, y, controlWidth, textureHeight, TRUE);
    groupY = y + 24;
    MoveWindow(g_app.controls.diffuseMap, innerX, groupY, 140, 22, TRUE);
    MoveWindow(g_app.controls.diffuseMapCombo, innerX + 140, groupY, innerWidth - 140, 200, TRUE);
    groupY += 34;
    MoveWindow(g_app.controls.normalMap, innerX, groupY, 140, 22, TRUE);
    MoveWindow(g_app.controls.normalMapCombo, innerX + 140, groupY, innerWidth - 140, 200, TRUE);
    groupY += 38;
    moveLine(g_app.controls.textureScaleLabel, g_app.controls.textureScaleValue, g_app.controls.textureScale, groupY);
    groupY += 50;
    MoveWindow(g_app.controls.resetButton, innerX, groupY, innerWidth, 28, TRUE);
}

void CreateUi(HWND hwnd) {
    g_app.controls.scene = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        kSceneClassName,
        L"",
        WS_CHILD | WS_VISIBLE,
        0, 0, 100, 100,
        hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_SCENE)),
        GetModuleHandleW(nullptr), nullptr);

    g_app.controls.lightingGroup = CreateGroupBox(hwnd, L"Lighting");
    g_app.controls.materialGroup = CreateGroupBox(hwnd, L"Material");
    g_app.controls.textureGroup = CreateGroupBox(hwnd, L"Texture");
    g_app.controls.objectLabel = CreateLabel(hwnd, L"Object");
    g_app.controls.objectCube = CreateButton(hwnd, L"Cube", IDC_OBJECT_CUBE, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP);
    g_app.controls.objectSphere = CreateButton(hwnd, L"Sphere", IDC_OBJECT_SPHERE, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON);
    g_app.controls.objectTorus = CreateButton(hwnd, L"Torus", IDC_OBJECT_TORUS, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON);

    g_app.controls.light1IntensityLabel = CreateLabel(hwnd, L"Light 1 Intensity");
    g_app.controls.light1IntensityValue = CreateLabel(hwnd, L"1.0", IDC_LIGHT1_INTENSITY_VALUE);
    g_app.controls.light1Intensity = CreateTrackBar(hwnd, IDC_LIGHT1_INTENSITY);
    g_app.controls.light1Color = CreateButton(hwnd, L"Light 1 Color", IDC_LIGHT1_COLOR);
    g_app.controls.light1Enabled = CreateButton(hwnd, L"Enable Light 1", IDC_LIGHT1_ENABLED, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);
    g_app.controls.light2IntensityLabel = CreateLabel(hwnd, L"Light 2 Intensity");
    g_app.controls.light2IntensityValue = CreateLabel(hwnd, L"0.8", IDC_LIGHT2_INTENSITY_VALUE);
    g_app.controls.light2Intensity = CreateTrackBar(hwnd, IDC_LIGHT2_INTENSITY);
    g_app.controls.light2Color = CreateButton(hwnd, L"Light 2 Color", IDC_LIGHT2_COLOR);
    g_app.controls.light2Enabled = CreateButton(hwnd, L"Enable Light 2", IDC_LIGHT2_ENABLED, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);

    g_app.controls.ambientIntensityLabel = CreateLabel(hwnd, L"Ambient Intensity");
    g_app.controls.ambientIntensityValue = CreateLabel(hwnd, L"0.2", IDC_AMBIENT_INTENSITY_VALUE);
    g_app.controls.ambientIntensity = CreateTrackBar(hwnd, IDC_AMBIENT_INTENSITY);

    g_app.controls.diffuseCoeffLabel = CreateLabel(hwnd, L"Diffuse Coefficient");
    g_app.controls.diffuseCoeffValue = CreateLabel(hwnd, L"0.8", IDC_DIFFUSE_COEFF_VALUE);
    g_app.controls.diffuseCoeff = CreateTrackBar(hwnd, IDC_DIFFUSE_COEFF);

    g_app.controls.specularCoeffLabel = CreateLabel(hwnd, L"Specular Coefficient");
    g_app.controls.specularCoeffValue = CreateLabel(hwnd, L"0.5", IDC_SPECULAR_COEFF_VALUE);
    g_app.controls.specularCoeff = CreateTrackBar(hwnd, IDC_SPECULAR_COEFF);

    g_app.controls.shininessLabel = CreateLabel(hwnd, L"Shininess");
    g_app.controls.shininessValue = CreateLabel(hwnd, L"30", IDC_SHININESS_VALUE);
    g_app.controls.shininess = CreateTrackBar(hwnd, IDC_SHININESS);
    g_app.controls.materialColor = CreateButton(hwnd, L"Material Color", IDC_MATERIAL_COLOR);
    g_app.controls.opacityLabel = CreateLabel(hwnd, L"Opacity");
    g_app.controls.opacityValue = CreateLabel(hwnd, L"1.0", IDC_OPACITY_VALUE);
    g_app.controls.opacity = CreateTrackBar(hwnd, IDC_OPACITY);

    g_app.controls.diffuseMap = CreateButton(hwnd, L"Enable Diffuse Map", IDC_DIFFUSE_MAP, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);
    g_app.controls.diffuseMapCombo = CreateWindowExW(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP, 0, 0, 100, 200, hwnd, (HMENU)IDC_DIFFUSE_MAP_COMBO, GetModuleHandleW(nullptr), nullptr);
    g_app.controls.normalMap = CreateButton(hwnd, L"Enable Normal Map", IDC_NORMAL_MAP, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);
    g_app.controls.normalMapCombo = CreateWindowExW(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP, 0, 0, 100, 200, hwnd, (HMENU)IDC_NORMAL_MAP_COMBO, GetModuleHandleW(nullptr), nullptr);

    g_app.controls.textureScaleLabel = CreateLabel(hwnd, L"Texture Scale");
    g_app.controls.textureScaleValue = CreateLabel(hwnd, L"1.0", IDC_TEXTURE_SCALE_VALUE);
    g_app.controls.textureScale = CreateTrackBar(hwnd, IDC_TEXTURE_SCALE);
    g_app.controls.resetButton = CreateButton(hwnd, L"Reset to Defaults", IDC_RESET_BUTTON);

    NONCLIENTMETRICSW metrics { sizeof(metrics) };
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)) {
        metrics.lfMessageFont.lfWeight = FW_SEMIBOLD;
        g_app.sectionFont = CreateFontIndirectW(&metrics.lfMessageFont);
    }

    if (g_app.sectionFont) {
        SendMessageW(g_app.controls.objectLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.sectionFont), TRUE);
        SendMessageW(g_app.controls.lightingGroup, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.sectionFont), TRUE);
        SendMessageW(g_app.controls.materialGroup, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.sectionFont), TRUE);
        SendMessageW(g_app.controls.textureGroup, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.sectionFont), TRUE);
        SendMessageW(g_app.controls.diffuseMapCombo, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.sectionFont), TRUE);
        SendMessageW(g_app.controls.normalMapCombo, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.sectionFont), TRUE);
    }

    SyncControlsFromState();
}

LRESULT CALLBACK SceneWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_ERASEBKGND:
        return 1;

    case WM_SIZE:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDOWN:
        g_app.dragging = true;
        g_app.lastMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        SetCapture(hwnd);
        return 0;

    case WM_MOUSEMOVE:
        if (g_app.dragging) {
            POINT pt { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            g_app.userYaw += (pt.x - g_app.lastMouse.x) * 0.01f;
            g_app.userPitch += (pt.y - g_app.lastMouse.y) * 0.01f;
            g_app.userPitch = Clamp(g_app.userPitch, -1.2f, 1.2f);
            g_app.lastMouse = pt;
            InvalidateRect(hwnd, nullptr, FALSE);
            UpdateWindow(hwnd);
        }
        return 0;

    case WM_LBUTTONUP:
        g_app.dragging = false;
        ReleaseCapture();
        return 0;

    case WM_MOUSEWHEEL:
        g_app.cameraDistance -= GET_WHEEL_DELTA_WPARAM(wParam) / 1200.0f;
        g_app.cameraDistance = Clamp(g_app.cameraDistance, 3.0f, 7.5f);
        InvalidateRect(hwnd, nullptr, FALSE);
        UpdateWindow(hwnd);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps {};
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintScene(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        DestroySceneRenderer();
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        g_app.mainWindow = hwnd;
        EnsureUiBrushes();
        CreateUi(hwnd);
        g_diffuseFiles = GetTextureFiles(L"texture\\diffuse");
        g_normalFiles = GetTextureFiles(L"texture\\normal");
        LoadDefaults();
        LayoutControls(hwnd);
        SetTimer(hwnd, kTimerId, 30, nullptr);
        return 0;

    case WM_SIZE:
        LayoutControls(hwnd);
        return 0;

    case WM_TIMER:
        if (wParam == kTimerId) {
            g_app.autoRotation += 0.02f;
            if (g_app.autoRotation > 2.0f * kPi) {
                g_app.autoRotation -= 2.0f * kPi;
            }
            InvalidateRect(g_app.controls.scene, nullptr, FALSE);
        }
        return 0;

    case WM_HSCROLL:
        UpdateValuesFromTrackbars();
        InvalidateRect(g_app.controls.scene, nullptr, FALSE);
        UpdateWindow(g_app.controls.scene);
        return 0;

    case WM_COMMAND: {
        if (HIWORD(wParam) == CBN_SELCHANGE) {
            int id = LOWORD(wParam);
            if (id == IDC_DIFFUSE_MAP_COMBO || id == IDC_NORMAL_MAP_COMBO) {
                HWND combo = reinterpret_cast<HWND>(lParam);
                int idx = SendMessageW(combo, CB_GETCURSEL, 0, 0);
                if (idx != CB_ERR) {
                    const auto& files = (id == IDC_DIFFUSE_MAP_COMBO) ? g_diffuseFiles : g_normalFiles;
                    if (idx >= 0 && idx < static_cast<int>(files.size())) {
                        if (id == IDC_DIFFUSE_MAP_COMBO) g_app.material.diffuseMapFile = files[idx].second;
                        else if (id == IDC_NORMAL_MAP_COMBO) g_app.material.normalMapFile = files[idx].second;
                        InvalidateRect(g_app.controls.scene, nullptr, FALSE);
                        UpdateWindow(g_app.controls.scene);
                    }
                }
            }
            return 0;
        }
        OnCommand(hwnd, LOWORD(wParam));
        return 0;
    }

    case WM_ERASEBKGND: {
        EnsureUiBrushes();
        HDC hdc = reinterpret_cast<HDC>(wParam);
        RECT rc {};
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, g_uiBackgroundBrush);

        RECT divider = rc;
        int dividerLeft = (std::max)(kSceneMinWidth, static_cast<int>(rc.right - 344));
        divider.left = dividerLeft;
        divider.right = divider.left + 1;
        FillRect(hdc, &divider, g_dividerBrush);
        return 1;
    }

    case WM_CTLCOLORSTATIC: {
        EnsureUiBrushes();
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, RGB(230, 240, 248));
        SetBkColor(hdc, RGB(18, 28, 40));
        SetBkMode(hdc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(g_uiBackgroundBrush);
    }

    case WM_CTLCOLORBTN: {
        EnsureUiBrushes();
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkColor(hdc, RGB(18, 28, 40));
        SetBkMode(hdc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(g_uiBackgroundBrush);
    }

    case WM_PAINT: {
        PAINTSTRUCT ps {};
        HDC hdc = BeginPaint(hwnd, &ps);
        SendMessageW(hwnd, WM_ERASEBKGND, reinterpret_cast<WPARAM>(hdc), 0);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, kTimerId);
        if (g_uiBackgroundBrush) {
            DeleteObject(g_uiBackgroundBrush);
            g_uiBackgroundBrush = nullptr;
        }
        if (g_dividerBrush) {
            DeleteObject(g_dividerBrush);
            g_dividerBrush = nullptr;
        }
        if (g_app.sectionFont) {
            DeleteObject(g_app.sectionFont);
            g_app.sectionFont = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool RegisterWindowClasses(HINSTANCE instance) {
    WNDCLASSW mainClass {};
    mainClass.style = CS_HREDRAW | CS_VREDRAW;
    mainClass.lpfnWndProc = MainWindowProc;
    mainClass.hInstance = instance;
    mainClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    mainClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    mainClass.lpszClassName = kMainClassName;

    WNDCLASSW sceneClass {};
    sceneClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    sceneClass.lpfnWndProc = SceneWindowProc;
    sceneClass.hInstance = instance;
    sceneClass.hCursor = LoadCursorW(nullptr, IDC_HAND);
    sceneClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    sceneClass.lpszClassName = kSceneClassName;

    return RegisterClassW(&mainClass) != 0 && RegisterClassW(&sceneClass) != 0;
}
