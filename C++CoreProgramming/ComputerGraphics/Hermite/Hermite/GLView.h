#pragma once

#include <windows.h>
#include <gl/GL.h>
#include <vector>
#include "HermiteCurve.h"

// ==================== 应用全局状态 ====================
// 存储在主窗口 GWLP_USERDATA 中，两个子窗口共享访问
class AppState {
public:
    // 关键帧数据
    std::vector<Keyframe> keyframes;
    std::vector<Point2f> curvePoints;

    // 动画状态
    bool isPlaying;
    float animationTime;      // 0.0 ~ 1.0
    float animationSpeed;     // 速度倍率

    // 交互状态
    EditStep editStep;
    int selectedIndex;
    int editingIndex;         // 正编辑的关键帧索引

    // 新增关键帧临时数据
    bool hasPendingKeyframe;
    Keyframe pendingKeyframe;

    // 运动路径模式
    int trajectoryMode;       // 0=折线段轨迹, 1=Hermite曲线

    // UI状态字符串（供GDI+面板显示）
    WCHAR statusText[256];
    int statusType;           // 0=normal, 1=editing-pos, 2=editing-tangent, 3=adding

    // 防闪烁：跟踪上次发送到面板的状态
    WCHAR lastPanelText[256];
    int lastPanelType;
    int lastSliderPos;        // 跟踪滑块上次值
    int lastSpeedSliderPos;   // 跟踪速度滑块上次值
    int lastTrajectoryMode;   // 跟踪上次轨迹模式

    // UI数值
    int sliderPos;            // 0~1000
    int speedSliderPos;       // 10~5000

    // 窗口句柄（用于跨窗口通信）
    HWND hGLView;
    HWND hUIPanel;

    AppState() {
        isPlaying = false;
        animationTime = 0.0f;
        animationSpeed = 1.0f;
        editStep = EditStep::Idle;
        selectedIndex = -1;
        editingIndex = -1;
        hasPendingKeyframe = false;
        sliderPos = 0;
        speedSliderPos = 100;
        trajectoryMode = 0;    // 默认折线段轨迹
        statusType = 0;
        lastPanelType = -1;
        lastPanelText[0] = 0;
        lastSliderPos = -1;
        lastSpeedSliderPos = -1;
        lastTrajectoryMode = -1;
        wcscpy_s(statusText, L"就绪：点击关键帧选中并拖拽，或点击空白处新增");
        hGLView = nullptr;
        hUIPanel = nullptr;
    }

    // 初始化示例关键帧（仅提供4个参考点，曲线生成由学生完成）
    void InitExample() {
        keyframes.clear();
        // 4个示例关键帧（位置 + 时间参数 0/0.33/0.66/1.0）
        keyframes.push_back(Keyframe(150, 250, 0.0f));
        keyframes.push_back(Keyframe(300, 150, 0.33f));
        keyframes.push_back(Keyframe(450, 200, 0.66f));
        keyframes.push_back(Keyframe(600, 300, 1.0f));

        // 设置初始切向量为Catmull-Rom风格方向（而非默认朝上），使箭头指向曲线走势
        keyframes[0].tangentX =  3;  keyframes[0].tangentY = -2;   // 右上方
        keyframes[1].tangentX =  6;  keyframes[1].tangentY = -1;   // 偏右上
        keyframes[2].tangentX =  2;  keyframes[2].tangentY =  1;   // 右下方
        keyframes[3].tangentX =  3;  keyframes[3].tangentY =  2;   // 右下方

        keyframes[0].selected = true;
        selectedIndex = 0;

        // TODO 学生: 当完成 generateCurvePoints() 和 generateCurveThroughPoints()
        //           实现后，取消下面注释以生成示例曲线：
        // curvePoints = generateCurvePoints(keyframes);
        // 或使用 generateCurveThroughPoints(keyframes, sw, sh) 生成完整曲线

        wcscpy_s(statusText, L"课堂作业：请实现 HermiteCurve.h 中的 TODO 函数");
    }

    void DeleteSelected() {
        if (selectedIndex >= 0 && selectedIndex < (int)keyframes.size()) {
            keyframes.erase(keyframes.begin() + selectedIndex);
            selectedIndex = -1;
            editingIndex = -1;
            editStep = EditStep::Idle;
            hasPendingKeyframe = false;
            sortKeyframes(keyframes);
            wcscpy_s(statusText, L"关键帧已删除");
        }
    }

    void ClearAll() {
        keyframes.clear();
        curvePoints.clear();
        hasPendingKeyframe = false;
        editingIndex = -1;
        selectedIndex = -1;
        editStep = EditStep::Idle;
        animationTime = 0.0f;
        isPlaying = false;
        sliderPos = 0;
        wcscpy_s(statusText, L"已清空所有关键帧");
    }
};

// ==================== OpenGL渲染视图窗口 ====================

// 注册OpenGL子窗口类
void RegisterGLViewClass(HINSTANCE hInstance);

// 创建OpenGL渲染子窗口
HWND CreateGLView(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h, AppState* state);

// ==================== 消息常量 ====================
#define WM_UPDATE_STATUS    (WM_USER + 100)
#define WM_PLAYBACK_TOGGLE  (WM_USER + 101)
#define WM_RESET_ANIM       (WM_USER + 102)
#define WM_DELETE_SELECTED  (WM_USER + 103)
#define WM_CLEAR_ALL        (WM_USER + 104)
#define WM_SET_ANIM_TIME    (WM_USER + 105)
#define WM_SET_ANIM_SPEED   (WM_USER + 106)
#define WM_SET_TRAJECTORY   (WM_USER + 107)
