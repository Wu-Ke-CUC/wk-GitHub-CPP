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
        if (!g_uiBackgroundBrush) g_uiBackgroundBrush = CreateSolidBrush(RGB(18, 28, 40));
        if (!g_dividerBrush) g_dividerBrush = CreateSolidBrush(RGB(50, 74, 95));
    }

    HWND CreateTrackBar(HWND parent, int id) {
        HWND hwnd = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_TOOLTIPS, 0, 0, 100, 24, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
        SendMessageW(hwnd, TBM_SETPAGESIZE, 0, 5);
        return hwnd;
    }

    HWND CreateLabel(HWND parent, const wchar_t* text, int id = 0, DWORD style = WS_CHILD | WS_VISIBLE) {
        return CreateWindowExW(0, L"STATIC", text, style, 0, 0, 100, 20, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
    }

    HWND CreateButton(HWND parent, const wchar_t* text, int id, DWORD style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON) {
        return CreateWindowExW(0, L"BUTTON", text, style, 0, 0, 100, 24, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
    }

    HWND CreateGroupBox(HWND parent, const wchar_t* text) {
        return CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 0, 100, 100, parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    }

    void SetSliderRange(HWND slider, int minValue, int maxValue, int currentValue) {
        SendMessageW(slider, TBM_SETRANGEMIN, TRUE, minValue);
        SendMessageW(slider, TBM_SETRANGEMAX, TRUE, maxValue);
        SendMessageW(slider, TBM_SETPOS, TRUE, currentValue);
    }

    int GetSliderPos(HWND slider) { return static_cast<int>(SendMessageW(slider, TBM_GETPOS, 0, 0)); }
    void SetLabelText(HWND hwnd, const std::wstring& text) { SetWindowTextW(hwnd, text.c_str()); }

    void OnCommand(HWND hwnd, int id) {
        switch (id) {
        case IDC_LIGHT1_ENABLED:
        case IDC_LIGHT2_ENABLED:
        case IDC_DIFFUSE_MAP:
        case IDC_NORMAL_MAP:
            UpdateCheckboxState();
            break;
        case IDC_LIGHT1_COLOR: ChooseAndSetColor(hwnd, g_app.light1.color); break;
        case IDC_LIGHT2_COLOR: ChooseAndSetColor(hwnd, g_app.light2.color); break;
        case IDC_MATERIAL_COLOR:
            if (g_app.activeTarget <= TargetType::ModelTorus) {
                ChooseAndSetColor(hwnd, g_app.objectMaterials[static_cast<int>(g_app.activeTarget)].color);
            }
            else {
                ChooseAndSetColor(hwnd, g_app.walls[static_cast<int>(g_app.activeTarget) - 3].color);
            }
            break;
        case IDC_RESET_BUTTON:
            LoadDefaults();
            SyncControlsFromState();
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
                        files.push_back({ (lastDot != std::wstring::npos) ? fileName.substr(0, lastDot) : fileName, subDir + L"\\" + fileName });
                    }
                } while (FindNextFileW(hFind, &fd));
                FindClose(hFind);
            }
        }
        return files;
    }

} // namespace

std::wstring FormatFloat(float value, int decimals) {
    wchar_t buffer[64]{};
    std::swprintf(buffer, 64, (decimals == 0) ? L"%.0f" : (decimals == 2 ? L"%.2f" : L"%.1f"), value);
    return buffer;
}

void LoadDefaults() {
    g_app.activeTarget = TargetType::ModelCube;

    g_app.light1.enabled = true;
    g_app.light1.intensity = 1.0f;
    g_app.light1.color = RGB(255, 255, 255);
    g_app.light1.position = { 0.0f, 4.5f, 0.0f }; // 天花板主光源

    g_app.light2.enabled = true;
    g_app.light2.intensity = 0.5f;
    g_app.light2.color = RGB(255, 180, 100);
    g_app.light2.position = { -4.0f, 0.0f, 4.0f }; // 侧边辅助光源

    g_app.ambientLightIntensity = 0.2f;

    g_diffuseFiles = GetTextureFiles(L"texture\\diffuse");
    g_normalFiles = GetTextureFiles(L"texture\\normal");
    std::wstring defDiffuse = g_diffuseFiles.empty() ? L"" : g_diffuseFiles[0].second;
    std::wstring defNormal = g_normalFiles.empty() ? L"" : g_normalFiles[0].second;

    // 0: Cube (粗糙漫反射 - 石膏/纸张)
    g_app.objectMaterials[0].color = RGB(220, 220, 220);
    g_app.objectMaterials[0].ambient = 0.2f;
    g_app.objectMaterials[0].diffuse = 0.8f;
    g_app.objectMaterials[0].specular = 0.05f;
    g_app.objectMaterials[0].shininess = 5.0f;
    g_app.objectMaterials[0].diffuseMapEnabled = true;
    g_app.objectMaterials[0].normalMapEnabled = true;
    g_app.objectMaterials[0].diffuseMapFile = defDiffuse;
    g_app.objectMaterials[0].normalMapFile = defNormal;
    g_app.objectMaterials[0].textureScale = 1.0f;

    // 1: Sphere (光滑非金属 - 塑料)
    g_app.objectMaterials[1].color = RGB(50, 150, 255);
    g_app.objectMaterials[1].specularColor = RGB(255, 255, 255);
    g_app.objectMaterials[1].ambient = 0.2f;
    g_app.objectMaterials[1].diffuse = 0.7f;
    g_app.objectMaterials[1].specular = 0.6f;
    g_app.objectMaterials[1].shininess = 70.0f;
    g_app.objectMaterials[1].diffuseMapEnabled = false;
    g_app.objectMaterials[1].normalMapEnabled = false;

    // 2: Torus (次表面材质 - 翡翠/玉石)
    g_app.objectMaterials[2].color = RGB(110, 190, 140);         // 基础漫反射颜色：浅翠绿色
    g_app.objectMaterials[2].ambientColor = RGB(40, 100, 70);    // 环境光颜色：较深的绿色，模拟内部折射的底色
    g_app.objectMaterials[2].specularColor = RGB(220, 255, 230); // 高光颜色：带一点微绿的亮白色，体现抛光质感
    g_app.objectMaterials[2].emissiveColor = RGB(20, 50, 30);    // 自发光颜色：用于伪造次表面通透感
    g_app.objectMaterials[2].ambient = 0.8f;                     // 环境光系数：调高以模拟光线在内部的漫反射
    g_app.objectMaterials[2].diffuse = 0.7f;                     // 漫反射系数
    g_app.objectMaterials[2].specular = 0.9f;                    // 高光系数：玉石表面通常抛光很好，反光强烈
    g_app.objectMaterials[2].shininess = 80.0f;                  // 光泽度：产生锐利且集中的高光斑
    g_app.objectMaterials[2].emissive = 0.4f;                    // 开启一定程度的自发光来提供内部的“莹润感”
    g_app.objectMaterials[2].opacity = 0.85f;                    // 透明度：<0.99 触发渲染器中的 GL_BLEND，透出背景
    g_app.objectMaterials[2].diffuseMapEnabled = false;
    g_app.objectMaterials[2].normalMapEnabled = false;

    // Cornell Box 墙面颜色初始化
    g_app.walls[0].color = RGB(200, 50, 50);   // 左红
    g_app.walls[1].color = RGB(50, 200, 50);   // 右绿
    g_app.walls[2].color = RGB(220, 220, 220); // 后白
    g_app.walls[3].color = RGB(220, 220, 220); // 上白
    g_app.walls[4].color = RGB(220, 220, 220); // 下白

    for (int i = 0; i < 5; ++i) {
        g_app.walls[i].textureEnabled = false;
        g_app.walls[i].textureFile = defDiffuse;
    }

    auto fillCombo = [&](HWND combo, const std::vector<std::pair<std::wstring, std::wstring>>& files) {
        if (!combo) return;
        SendMessageW(combo, CB_RESETCONTENT, 0, 0);
        for (const auto& f : files) SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(f.first.c_str()));
        };
    fillCombo(g_app.controls.diffuseMapCombo, g_diffuseFiles);
    fillCombo(g_app.controls.normalMapCombo, g_normalFiles);

    g_app.userYaw = -0.3f;
    g_app.userPitch = 0.1f;
    g_app.cameraDistance = 9.0f;
}

void SyncControlsFromState() {
    SendMessageW(g_app.controls.targetSelect, CB_SETCURSEL, static_cast<int>(g_app.activeTarget), 0);

    SetSliderRange(g_app.controls.light1Intensity, 0, 200, static_cast<int>(g_app.light1.intensity * 100.0f));
    SetSliderRange(g_app.controls.light1PosX, -100, 100, static_cast<int>(g_app.light1.position.x * 10.0f));
    SetSliderRange(g_app.controls.light1PosY, -100, 100, static_cast<int>(g_app.light1.position.y * 10.0f));
    SetSliderRange(g_app.controls.light1PosZ, -100, 100, static_cast<int>(g_app.light1.position.z * 10.0f));

    SetSliderRange(g_app.controls.light2Intensity, 0, 200, static_cast<int>(g_app.light2.intensity * 100.0f));
    SetSliderRange(g_app.controls.light2PosX, -100, 100, static_cast<int>(g_app.light2.position.x * 10.0f));
    SetSliderRange(g_app.controls.light2PosY, -100, 100, static_cast<int>(g_app.light2.position.y * 10.0f));
    SetSliderRange(g_app.controls.light2PosZ, -100, 100, static_cast<int>(g_app.light2.position.z * 10.0f));

    SetSliderRange(g_app.controls.ambientIntensity, 0, 100, static_cast<int>(g_app.ambientLightIntensity * 100.0f));

    Button_SetCheck(g_app.controls.light1Enabled, g_app.light1.enabled ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(g_app.controls.light2Enabled, g_app.light2.enabled ? BST_CHECKED : BST_UNCHECKED);
    SetLabelText(g_app.controls.light1IntensityValue, FormatFloat(g_app.light1.intensity));
    SetLabelText(g_app.controls.light2IntensityValue, FormatFloat(g_app.light2.intensity));
    SetLabelText(g_app.controls.ambientIntensityValue, FormatFloat(g_app.ambientLightIntensity));

    auto syncCombo = [](HWND combo, const std::wstring& selectedPath, const std::vector<std::pair<std::wstring, std::wstring>>& files) {
        for (size_t i = 0; i < files.size(); ++i) {
            if (files[i].second == selectedPath || (files[i].second.empty() && selectedPath.empty())) {
                SendMessageW(combo, CB_SETCURSEL, i, 0); return;
            }
        }
        SendMessageW(combo, CB_SETCURSEL, 0, 0);
        };

    bool isModel = g_app.activeTarget <= TargetType::ModelTorus;
    EnableWindow(g_app.controls.diffuseCoeff, isModel);
    EnableWindow(g_app.controls.specularCoeff, isModel);
    EnableWindow(g_app.controls.shininess, isModel);
    EnableWindow(g_app.controls.opacity, isModel);
    EnableWindow(g_app.controls.normalMap, isModel);
    EnableWindow(g_app.controls.normalMapCombo, isModel);
    EnableWindow(g_app.controls.textureScale, isModel);

    if (isModel) {
        const auto& mat = g_app.objectMaterials[static_cast<int>(g_app.activeTarget)];
        SetSliderRange(g_app.controls.diffuseCoeff, 0, 100, static_cast<int>(mat.diffuse * 100.0f));
        SetSliderRange(g_app.controls.specularCoeff, 0, 100, static_cast<int>(mat.specular * 100.0f));
        SetSliderRange(g_app.controls.shininess, 1, 200, static_cast<int>(mat.shininess));
        SetSliderRange(g_app.controls.opacity, 5, 100, static_cast<int>(mat.opacity * 100.0f));
        SetSliderRange(g_app.controls.textureScale, 1, 50, static_cast<int>(mat.textureScale * 10.0f));

        Button_SetCheck(g_app.controls.diffuseMap, mat.diffuseMapEnabled ? BST_CHECKED : BST_UNCHECKED);
        Button_SetCheck(g_app.controls.normalMap, mat.normalMapEnabled ? BST_CHECKED : BST_UNCHECKED);
        syncCombo(g_app.controls.diffuseMapCombo, mat.diffuseMapFile, g_diffuseFiles);
        syncCombo(g_app.controls.normalMapCombo, mat.normalMapFile, g_normalFiles);

        SetLabelText(g_app.controls.diffuseCoeffValue, FormatFloat(mat.diffuse));
        SetLabelText(g_app.controls.specularCoeffValue, FormatFloat(mat.specular));
        SetLabelText(g_app.controls.shininessValue, FormatFloat(mat.shininess, 0));
        SetLabelText(g_app.controls.opacityValue, FormatFloat(mat.opacity));
        SetLabelText(g_app.controls.textureScaleValue, FormatFloat(mat.textureScale));
    }
    else {
        const auto& wall = g_app.walls[static_cast<int>(g_app.activeTarget) - 3];
        Button_SetCheck(g_app.controls.diffuseMap, wall.textureEnabled ? BST_CHECKED : BST_UNCHECKED);
        syncCombo(g_app.controls.diffuseMapCombo, wall.textureFile, g_diffuseFiles);

        SetLabelText(g_app.controls.diffuseCoeffValue, L"-");
        SetLabelText(g_app.controls.specularCoeffValue, L"-");
        SetLabelText(g_app.controls.shininessValue, L"-");
        SetLabelText(g_app.controls.opacityValue, L"-");
        SetLabelText(g_app.controls.textureScaleValue, L"-");
    }

    InvalidateRect(g_app.controls.scene, nullptr, FALSE);
    UpdateWindow(g_app.controls.scene);
}

void UpdateValuesFromTrackbars() {
    g_app.light1.intensity = GetSliderPos(g_app.controls.light1Intensity) / 100.0f;
    g_app.light1.position.x = GetSliderPos(g_app.controls.light1PosX) / 10.0f;
    g_app.light1.position.y = GetSliderPos(g_app.controls.light1PosY) / 10.0f;
    g_app.light1.position.z = GetSliderPos(g_app.controls.light1PosZ) / 10.0f;

    g_app.light2.intensity = GetSliderPos(g_app.controls.light2Intensity) / 100.0f;
    g_app.light2.position.x = GetSliderPos(g_app.controls.light2PosX) / 10.0f;
    g_app.light2.position.y = GetSliderPos(g_app.controls.light2PosY) / 10.0f;
    g_app.light2.position.z = GetSliderPos(g_app.controls.light2PosZ) / 10.0f;

    g_app.ambientLightIntensity = GetSliderPos(g_app.controls.ambientIntensity) / 100.0f;

    if (g_app.activeTarget <= TargetType::ModelTorus) {
        int idx = static_cast<int>(g_app.activeTarget);
        g_app.objectMaterials[idx].diffuse = GetSliderPos(g_app.controls.diffuseCoeff) / 100.0f;
        g_app.objectMaterials[idx].specular = GetSliderPos(g_app.controls.specularCoeff) / 100.0f;
        g_app.objectMaterials[idx].shininess = static_cast<float>(GetSliderPos(g_app.controls.shininess));
        g_app.objectMaterials[idx].opacity = GetSliderPos(g_app.controls.opacity) / 100.0f;
        g_app.objectMaterials[idx].textureScale = GetSliderPos(g_app.controls.textureScale) / 10.0f;
    }

    SetLabelText(g_app.controls.light1IntensityValue, FormatFloat(g_app.light1.intensity));
    SetLabelText(g_app.controls.light2IntensityValue, FormatFloat(g_app.light2.intensity));
    SetLabelText(g_app.controls.ambientIntensityValue, FormatFloat(g_app.ambientLightIntensity));

    if (g_app.activeTarget <= TargetType::ModelTorus) {
        int idx = static_cast<int>(g_app.activeTarget);
        SetLabelText(g_app.controls.diffuseCoeffValue, FormatFloat(g_app.objectMaterials[idx].diffuse));
        SetLabelText(g_app.controls.specularCoeffValue, FormatFloat(g_app.objectMaterials[idx].specular));
        SetLabelText(g_app.controls.shininessValue, FormatFloat(g_app.objectMaterials[idx].shininess, 0));
        SetLabelText(g_app.controls.opacityValue, FormatFloat(g_app.objectMaterials[idx].opacity));
        SetLabelText(g_app.controls.textureScaleValue, FormatFloat(g_app.objectMaterials[idx].textureScale));
    }
}

void UpdateCheckboxState() {
    g_app.light1.enabled = Button_GetCheck(g_app.controls.light1Enabled) == BST_CHECKED;
    g_app.light2.enabled = Button_GetCheck(g_app.controls.light2Enabled) == BST_CHECKED;

    bool diffEnabled = Button_GetCheck(g_app.controls.diffuseMap) == BST_CHECKED;
    bool normEnabled = Button_GetCheck(g_app.controls.normalMap) == BST_CHECKED;
    if (g_app.activeTarget <= TargetType::ModelTorus) {
        g_app.objectMaterials[static_cast<int>(g_app.activeTarget)].diffuseMapEnabled = diffEnabled;
        g_app.objectMaterials[static_cast<int>(g_app.activeTarget)].normalMapEnabled = normEnabled;
    }
    else {
        g_app.walls[static_cast<int>(g_app.activeTarget) - 3].textureEnabled = diffEnabled;
    }
}

void ChooseAndSetColor(HWND owner, COLORREF& targetColor) {
    CHOOSECOLORW cc{};
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
    RECT rc{};
    GetClientRect(hwnd, &rc);

    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    int controlWidth = 360;
    int gap = 12;
    int sceneWidth = std::max(kSceneMinWidth, width - controlWidth - gap * 3);
    sceneWidth = std::min(sceneWidth, width - gap * 2);
    int sceneHeight = height - gap * 2;
    int controlsX = gap + sceneWidth + gap;
    int innerX = controlsX + 12;
    int innerWidth = controlWidth - 24;

    MoveWindow(g_app.controls.scene, gap, gap, sceneWidth, sceneHeight, TRUE);

    auto moveLine = [&](HWND label, HWND value, HWND slider, int y) {
        MoveWindow(label, innerX, y, innerWidth - 54, 20, TRUE);
        MoveWindow(value, innerX + innerWidth - 48, y, 48, 20, TRUE);
        MoveWindow(slider, innerX, y + 18, innerWidth, 26, TRUE);
        };
    auto movePos = [&](HWND label, HWND px, HWND py, HWND pz, int y) {
        MoveWindow(label, innerX, y, innerWidth, 16, TRUE);
        MoveWindow(px, innerX, y + 16, innerWidth / 3, 24, TRUE);
        MoveWindow(py, innerX + innerWidth / 3, y + 16, innerWidth / 3, 24, TRUE);
        MoveWindow(pz, innerX + (innerWidth / 3) * 2, y + 16, innerWidth / 3, 24, TRUE);
        };

    int y = gap;
    MoveWindow(g_app.controls.targetLabel, controlsX, y, 80, 20, TRUE);
    MoveWindow(g_app.controls.targetSelect, controlsX + 90, y, controlWidth - 90, 200, TRUE);
    y += 34;

    const int lightingHeight = 310;
    MoveWindow(g_app.controls.lightingGroup, controlsX, y, controlWidth, lightingHeight, TRUE);
    int groupY = y + 20;
    moveLine(g_app.controls.light1IntensityLabel, g_app.controls.light1IntensityValue, g_app.controls.light1Intensity, groupY);
    groupY += 44;
    movePos(g_app.controls.light1PosLabel, g_app.controls.light1PosX, g_app.controls.light1PosY, g_app.controls.light1PosZ, groupY);
    groupY += 40;
    MoveWindow(g_app.controls.light1Color, innerX, groupY, 140, 24, TRUE);
    MoveWindow(g_app.controls.light1Enabled, innerX + innerWidth - 140, groupY, 140, 24, TRUE);
    groupY += 34;

    moveLine(g_app.controls.light2IntensityLabel, g_app.controls.light2IntensityValue, g_app.controls.light2Intensity, groupY);
    groupY += 44;
    movePos(g_app.controls.light2PosLabel, g_app.controls.light2PosX, g_app.controls.light2PosY, g_app.controls.light2PosZ, groupY);
    groupY += 40;
    MoveWindow(g_app.controls.light2Color, innerX, groupY, 140, 24, TRUE);
    MoveWindow(g_app.controls.light2Enabled, innerX + innerWidth - 140, groupY, 140, 24, TRUE);
    groupY += 34;
    y += lightingHeight + 10;

    const int materialHeight = 270;
    MoveWindow(g_app.controls.materialGroup, controlsX, y, controlWidth, materialHeight, TRUE);
    groupY = y + 24;
    MoveWindow(g_app.controls.materialColor, innerX, groupY, innerWidth, 26, TRUE);
    groupY += 36;
    moveLine(g_app.controls.diffuseCoeffLabel, g_app.controls.diffuseCoeffValue, g_app.controls.diffuseCoeff, groupY);
    groupY += 46;
    moveLine(g_app.controls.specularCoeffLabel, g_app.controls.specularCoeffValue, g_app.controls.specularCoeff, groupY);
    groupY += 46;
    moveLine(g_app.controls.shininessLabel, g_app.controls.shininessValue, g_app.controls.shininess, groupY);
    groupY += 46;
    moveLine(g_app.controls.opacityLabel, g_app.controls.opacityValue, g_app.controls.opacity, groupY);
    y += materialHeight + 10;

    const int textureHeight = 176;
    MoveWindow(g_app.controls.textureGroup, controlsX, y, controlWidth, textureHeight, TRUE);
    groupY = y + 24;
    MoveWindow(g_app.controls.diffuseMap, innerX, groupY, 140, 22, TRUE);
    MoveWindow(g_app.controls.diffuseMapCombo, innerX + 140, groupY, innerWidth - 140, 200, TRUE);
    groupY += 34;
    MoveWindow(g_app.controls.normalMap, innerX, groupY, 140, 22, TRUE);
    MoveWindow(g_app.controls.normalMapCombo, innerX + 140, groupY, innerWidth - 140, 200, TRUE);
    groupY += 34;
    moveLine(g_app.controls.textureScaleLabel, g_app.controls.textureScaleValue, g_app.controls.textureScale, groupY);
    groupY += 50;
    MoveWindow(g_app.controls.resetButton, innerX, groupY, innerWidth, 26, TRUE);
}

void CreateUi(HWND hwnd) {
    g_app.controls.scene = CreateWindowExW(WS_EX_CLIENTEDGE, kSceneClassName, L"", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_SCENE)), GetModuleHandleW(nullptr), nullptr);
    g_app.controls.lightingGroup = CreateGroupBox(hwnd, L"Lighting (Global)");
    g_app.controls.materialGroup = CreateGroupBox(hwnd, L"Material (Selected Target)");
    g_app.controls.textureGroup = CreateGroupBox(hwnd, L"Texture (Selected Target)");

    g_app.controls.targetLabel = CreateLabel(hwnd, L"Edit Target:");
    g_app.controls.targetSelect = CreateWindowExW(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP, 0, 0, 100, 200, hwnd, (HMENU)IDC_TARGET_SELECT, GetModuleHandleW(nullptr), nullptr);
    const wchar_t* targets[] = { L"Model: Cube", L"Model: Sphere", L"Model: Torus", L"Wall: Left", L"Wall: Right", L"Wall: Back", L"Wall: Top", L"Wall: Bottom" };
    for (const auto& t : targets) SendMessageW(g_app.controls.targetSelect, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(t));

    g_app.controls.light1IntensityLabel = CreateLabel(hwnd, L"Light 1 Intensity");
    g_app.controls.light1IntensityValue = CreateLabel(hwnd, L"1.0", IDC_LIGHT1_INTENSITY_VALUE);
    g_app.controls.light1Intensity = CreateTrackBar(hwnd, IDC_LIGHT1_INTENSITY);
    g_app.controls.light1PosLabel = CreateLabel(hwnd, L"Light 1 Position (X, Y, Z)");
    g_app.controls.light1PosX = CreateTrackBar(hwnd, IDC_LIGHT1_POS_X);
    g_app.controls.light1PosY = CreateTrackBar(hwnd, IDC_LIGHT1_POS_Y);
    g_app.controls.light1PosZ = CreateTrackBar(hwnd, IDC_LIGHT1_POS_Z);
    g_app.controls.light1Color = CreateButton(hwnd, L"Light 1 Color", IDC_LIGHT1_COLOR);
    g_app.controls.light1Enabled = CreateButton(hwnd, L"Enable Light 1", IDC_LIGHT1_ENABLED, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);

    g_app.controls.light2IntensityLabel = CreateLabel(hwnd, L"Light 2 Intensity");
    g_app.controls.light2IntensityValue = CreateLabel(hwnd, L"0.5", IDC_LIGHT2_INTENSITY_VALUE);
    g_app.controls.light2Intensity = CreateTrackBar(hwnd, IDC_LIGHT2_INTENSITY);
    g_app.controls.light2PosLabel = CreateLabel(hwnd, L"Light 2 Position (X, Y, Z)");
    g_app.controls.light2PosX = CreateTrackBar(hwnd, IDC_LIGHT2_POS_X);
    g_app.controls.light2PosY = CreateTrackBar(hwnd, IDC_LIGHT2_POS_Y);
    g_app.controls.light2PosZ = CreateTrackBar(hwnd, IDC_LIGHT2_POS_Z);
    g_app.controls.light2Color = CreateButton(hwnd, L"Light 2 Color", IDC_LIGHT2_COLOR);
    g_app.controls.light2Enabled = CreateButton(hwnd, L"Enable Light 2", IDC_LIGHT2_ENABLED, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);

    g_app.controls.ambientIntensityLabel = CreateLabel(hwnd, L"Ambient Intensity");
    g_app.controls.ambientIntensityValue = CreateLabel(hwnd, L"0.2", IDC_AMBIENT_INTENSITY_VALUE);
    g_app.controls.ambientIntensity = CreateTrackBar(hwnd, IDC_AMBIENT_INTENSITY);

    g_app.controls.diffuseCoeffLabel = CreateLabel(hwnd, L"Diffuse Coeff");
    g_app.controls.diffuseCoeffValue = CreateLabel(hwnd, L"0.8", IDC_DIFFUSE_COEFF_VALUE);
    g_app.controls.diffuseCoeff = CreateTrackBar(hwnd, IDC_DIFFUSE_COEFF);
    g_app.controls.specularCoeffLabel = CreateLabel(hwnd, L"Specular Coeff");
    g_app.controls.specularCoeffValue = CreateLabel(hwnd, L"0.5", IDC_SPECULAR_COEFF_VALUE);
    g_app.controls.specularCoeff = CreateTrackBar(hwnd, IDC_SPECULAR_COEFF);
    g_app.controls.shininessLabel = CreateLabel(hwnd, L"Shininess");
    g_app.controls.shininessValue = CreateLabel(hwnd, L"30", IDC_SHININESS_VALUE);
    g_app.controls.shininess = CreateTrackBar(hwnd, IDC_SHININESS);
    g_app.controls.opacityLabel = CreateLabel(hwnd, L"Opacity");
    g_app.controls.opacityValue = CreateLabel(hwnd, L"1.0", IDC_OPACITY_VALUE);
    g_app.controls.opacity = CreateTrackBar(hwnd, IDC_OPACITY);
    g_app.controls.materialColor = CreateButton(hwnd, L"Target Base Color", IDC_MATERIAL_COLOR);

    g_app.controls.diffuseMap = CreateButton(hwnd, L"Enable Diffuse Map", IDC_DIFFUSE_MAP, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);
    g_app.controls.diffuseMapCombo = CreateWindowExW(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP, 0, 0, 100, 200, hwnd, (HMENU)IDC_DIFFUSE_MAP_COMBO, GetModuleHandleW(nullptr), nullptr);
    g_app.controls.normalMap = CreateButton(hwnd, L"Enable Normal Map", IDC_NORMAL_MAP, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);
    g_app.controls.normalMapCombo = CreateWindowExW(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP, 0, 0, 100, 200, hwnd, (HMENU)IDC_NORMAL_MAP_COMBO, GetModuleHandleW(nullptr), nullptr);

    g_app.controls.textureScaleLabel = CreateLabel(hwnd, L"Texture Scale");
    g_app.controls.textureScaleValue = CreateLabel(hwnd, L"1.0", IDC_TEXTURE_SCALE_VALUE);
    g_app.controls.textureScale = CreateTrackBar(hwnd, IDC_TEXTURE_SCALE);
    g_app.controls.resetButton = CreateButton(hwnd, L"Reset Settings", IDC_RESET_BUTTON);

    NONCLIENTMETRICSW metrics{ sizeof(metrics) };
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)) {
        metrics.lfMessageFont.lfWeight = FW_SEMIBOLD;
        g_app.sectionFont = CreateFontIndirectW(&metrics.lfMessageFont);
    }
    if (g_app.sectionFont) {
        SendMessageW(g_app.controls.targetLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.sectionFont), TRUE);
        SendMessageW(g_app.controls.lightingGroup, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.sectionFont), TRUE);
        SendMessageW(g_app.controls.materialGroup, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.sectionFont), TRUE);
        SendMessageW(g_app.controls.textureGroup, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.sectionFont), TRUE);
        SendMessageW(g_app.controls.targetSelect, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.sectionFont), TRUE);
        SendMessageW(g_app.controls.diffuseMapCombo, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.sectionFont), TRUE);
        SendMessageW(g_app.controls.normalMapCombo, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.sectionFont), TRUE);
    }

    SyncControlsFromState();
}

LRESULT CALLBACK SceneWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_ERASEBKGND: return 1;
    case WM_SIZE: InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_LBUTTONDOWN: g_app.dragging = true; g_app.lastMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }; SetCapture(hwnd); return 0;
    case WM_MOUSEMOVE:
        if (g_app.dragging) {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            g_app.userYaw += (pt.x - g_app.lastMouse.x) * 0.01f;
            g_app.userPitch += (pt.y - g_app.lastMouse.y) * 0.01f;
            g_app.userPitch = Clamp(g_app.userPitch, -1.2f, 1.2f);
            g_app.lastMouse = pt;
            InvalidateRect(hwnd, nullptr, FALSE);
            UpdateWindow(hwnd);
        }
        return 0;
    case WM_LBUTTONUP: g_app.dragging = false; ReleaseCapture(); return 0;
    case WM_MOUSEWHEEL:
        g_app.cameraDistance -= GET_WHEEL_DELTA_WPARAM(wParam) / 600.0f;
        g_app.cameraDistance = Clamp(g_app.cameraDistance, 1.0f, 15.0f);
        InvalidateRect(hwnd, nullptr, FALSE);
        UpdateWindow(hwnd);
        return 0;
    case WM_PAINT: { PAINTSTRUCT ps{}; HDC hdc = BeginPaint(hwnd, &ps); PaintScene(hwnd, hdc); EndPaint(hwnd, &ps); return 0; }
    case WM_DESTROY: DestroySceneRenderer(); return 0;
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
    case WM_SIZE: LayoutControls(hwnd); return 0;
    case WM_TIMER:
        if (wParam == kTimerId) {
            g_app.autoRotation += 0.01f;
            if (g_app.autoRotation > 2.0f * kPi) g_app.autoRotation -= 2.0f * kPi;
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
            HWND combo = reinterpret_cast<HWND>(lParam);
            int idx = SendMessageW(combo, CB_GETCURSEL, 0, 0);
            if (id == IDC_TARGET_SELECT) {
                if (idx != CB_ERR && idx < static_cast<int>(TargetType::Count)) {
                    g_app.activeTarget = static_cast<TargetType>(idx);
                    SyncControlsFromState();
                }
            }
            else if (id == IDC_DIFFUSE_MAP_COMBO || id == IDC_NORMAL_MAP_COMBO) {
                if (idx != CB_ERR) {
                    const auto& files = (id == IDC_DIFFUSE_MAP_COMBO) ? g_diffuseFiles : g_normalFiles;
                    if (idx >= 0 && idx < static_cast<int>(files.size())) {
                        if (g_app.activeTarget <= TargetType::ModelTorus) {
                            int mIdx = static_cast<int>(g_app.activeTarget);
                            if (id == IDC_DIFFUSE_MAP_COMBO) g_app.objectMaterials[mIdx].diffuseMapFile = files[idx].second;
                            else if (id == IDC_NORMAL_MAP_COMBO) g_app.objectMaterials[mIdx].normalMapFile = files[idx].second;
                        }
                        else {
                            if (id == IDC_DIFFUSE_MAP_COMBO) g_app.walls[static_cast<int>(g_app.activeTarget) - 3].textureFile = files[idx].second;
                        }
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
        EnsureUiBrushes(); HDC hdc = reinterpret_cast<HDC>(wParam); RECT rc{}; GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, g_uiBackgroundBrush);
        RECT divider = rc;
        int dividerLeft = (std::max)(kSceneMinWidth, static_cast<int>(rc.right - 394));
        divider.left = dividerLeft; divider.right = divider.left + 1;
        FillRect(hdc, &divider, g_dividerBrush);
        return 1;
    }
    case WM_CTLCOLORSTATIC: {
        EnsureUiBrushes(); HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, RGB(230, 240, 248)); SetBkColor(hdc, RGB(18, 28, 40)); SetBkMode(hdc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(g_uiBackgroundBrush);
    }
    case WM_CTLCOLORBTN: {
        EnsureUiBrushes(); HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkColor(hdc, RGB(18, 28, 40)); SetBkMode(hdc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(g_uiBackgroundBrush);
    }
    case WM_PAINT: { PAINTSTRUCT ps{}; HDC hdc = BeginPaint(hwnd, &ps); SendMessageW(hwnd, WM_ERASEBKGND, reinterpret_cast<WPARAM>(hdc), 0); EndPaint(hwnd, &ps); return 0; }
    case WM_DESTROY:
        KillTimer(hwnd, kTimerId);
        if (g_uiBackgroundBrush) DeleteObject(g_uiBackgroundBrush);
        if (g_dividerBrush) DeleteObject(g_dividerBrush);
        if (g_app.sectionFont) DeleteObject(g_app.sectionFont);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool RegisterWindowClasses(HINSTANCE instance) {
    WNDCLASSW mainClass{};
    mainClass.style = CS_HREDRAW | CS_VREDRAW; mainClass.lpfnWndProc = MainWindowProc; mainClass.hInstance = instance;
    mainClass.hCursor = LoadCursorW(nullptr, IDC_ARROW); mainClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); mainClass.lpszClassName = kMainClassName;
    WNDCLASSW sceneClass{};
    sceneClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW; sceneClass.lpfnWndProc = SceneWindowProc; sceneClass.hInstance = instance;
    sceneClass.hCursor = LoadCursorW(nullptr, IDC_HAND); sceneClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); sceneClass.lpszClassName = kSceneClassName;
    return RegisterClassW(&mainClass) != 0 && RegisterClassW(&sceneClass) != 0;
}