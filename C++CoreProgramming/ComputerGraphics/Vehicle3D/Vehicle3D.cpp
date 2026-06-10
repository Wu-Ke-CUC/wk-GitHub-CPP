// Vehicle3D.cpp - Application Entry Point
#include "Include/stdafx.h"
#include "Include/Vehicle3D.h"
#include "Include/Renderer.h"
#include "Include/Scene.h"
#include "Include/Vehicle.h"
#include "Include/Camera.h"
#include "Include/InputManager.h"
#include "Include/UI.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

// Global variables
HINSTANCE g_hInstance;
HWND g_hWnd;
HDC g_hDC;
HGLRC g_hRC;
bool g_isRunning = true;
bool g_hasFocus = true;

// Scene components
Scene* g_scene = nullptr;
Target* g_targets[2];
Target* g_currentTarget = nullptr;
Camera* g_camera = nullptr;
InputManager* g_input = nullptr;
UI* g_ui = nullptr;
Renderer* g_renderer = nullptr;

// Function declarations
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void SetupPixelFormat(HDC hDC);
bool InitializeOpenGL(HWND hWnd);
void ResizeScene(int width, int height);
void UpdateFrame();
void RenderFrame();
void Cleanup();

// Timing
LARGE_INTEGER g_performanceFrequency;
LARGE_INTEGER g_lastTime;
float g_deltaTime = 0.0f;

// Reset camera function (used by UI)
void ResetCameraCallback()
{
    if (g_camera)
        g_camera->ResetCamera();
}

// WinMain
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                       _In_opt_ HINSTANCE hPrevInstance,
                       _In_ LPTSTR lpCmdLine,
                       _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    g_hInstance = hInstance;

    // Register window class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VEHICLE3D));
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"Vehicle3DClass";

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL, L"Failed to register window class!", L"Error", MB_ICONERROR);
        return 1;
    }

    // Get screen size
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create window
    g_hWnd = CreateWindow(
        L"Vehicle3DClass",
        L"3D Vehicle Camera Control",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        screenWidth, screenHeight,
        NULL, NULL,
        hInstance, NULL);

    if (!g_hWnd)
    {
        MessageBox(NULL, L"Failed to create window!", L"Error", MB_ICONERROR);
        return 1;
    }

    // Initialize OpenGL
    if (!InitializeOpenGL(g_hWnd))
    {
        MessageBox(NULL, L"Failed to initialize OpenGL!", L"Error", MB_ICONERROR);
        return 1;
    }

    // Initialize timer
    QueryPerformanceFrequency(&g_performanceFrequency);
    QueryPerformanceCounter(&g_lastTime);

    // Initialize scene
    g_renderer = new Renderer();
    g_scene = new Scene();
    g_targets[0] = new Vehicle();
    g_targets[1] = new Aircraft();
    g_currentTarget = g_targets[0];
    g_camera = new Camera();
    g_input = new InputManager();
    g_ui = new UI(g_hWnd);

    g_scene->Initialize();
    g_targets[0]->Initialize();
    g_targets[1]->Initialize();
    g_camera->Initialize();
    g_input->Initialize(g_hWnd);

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    // Message loop
    MSG msg;
    while (g_isRunning)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                g_isRunning = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (g_isRunning && g_hasFocus)
        {
            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);
            g_deltaTime = (float)(currentTime.QuadPart - g_lastTime.QuadPart) / g_performanceFrequency.QuadPart;
            g_lastTime = currentTime;

            UpdateFrame();
            RenderFrame();
        }
        else
        {
            Sleep(10);
        }
    }

    Cleanup();

    return (int)msg.wParam;
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        if (g_hDC && g_hRC)
        {
            ResizeScene(LOWORD(lParam), HIWORD(lParam));
        }
        break;

    case WM_SETFOCUS:
        g_hasFocus = true;
        break;

    case WM_KILLFOCUS:
        g_hasFocus = false;
        g_input->ReleaseAllKeys();
        break;

    case WM_KEYDOWN:
        if (g_input)
        {
            g_input->KeyDown((int)wParam);
        }
        if (wParam == VK_F2)
        {
            if (g_camera)
                g_camera->ResetCamera();
        }
        if (wParam == VK_ESCAPE)
        {
            int result = MessageBox(hWnd, L"Are you sure you want to exit?", L"Exit", MB_YESNO | MB_ICONQUESTION);
            if (result == IDYES)
            {
                g_isRunning = false;
                PostQuitMessage(0);
            }
        }
        break;

    case WM_KEYUP:
        if (g_input)
        {
            g_input->KeyUp((int)wParam);
        }
        break;

    case WM_LBUTTONDOWN:
        if (g_ui)
        {
            g_ui->OnMouseDown(LOWORD(lParam), HIWORD(lParam));
        }
        break;

    case WM_LBUTTONUP:
        if (g_ui)
        {
            g_ui->OnMouseUp();
        }
        break;

    case WM_MOUSEMOVE:
        if (g_ui && g_ui->IsDragging())
        {
            g_ui->OnMouseMove(LOWORD(lParam), HIWORD(lParam));
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Setup pixel format
void SetupPixelFormat(HDC hDC)
{
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        32,
        0,
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    int pixelFormat = ChoosePixelFormat(hDC, &pfd);
    SetPixelFormat(hDC, pixelFormat, &pfd);
}

// Initialize OpenGL
bool InitializeOpenGL(HWND hWnd)
{
    g_hDC = GetDC(hWnd);
    if (!g_hDC)
    {
        return false;
    }

    SetupPixelFormat(g_hDC);

    g_hRC = wglCreateContext(g_hDC);
    if (!g_hRC)
    {
        ReleaseDC(hWnd, g_hDC);
        return false;
    }

    if (!wglMakeCurrent(g_hDC, g_hRC))
    {
        wglDeleteContext(g_hRC);
        ReleaseDC(hWnd, g_hDC);
        return false;
    }

    glClearColor(0.529f, 0.808f, 0.922f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    return true;
}

// Resize scene
void ResizeScene(int width, int height)
{
    if (height == 0) height = 1;

    glViewport(0, 0, width, height);

    if (g_camera)
    {
        g_camera->SetAspectRatio((float)width / (float)height);
    }

    if (g_ui)
    {
        g_ui->Resize(width, height);
    }
}

// Update frame
void UpdateFrame()
{
    g_input->Update();
    // 切换目标 (T键)
    static bool lastTState = false;
    bool currentT = (GetAsyncKeyState('T') & 0x8000) != 0;
    if (currentT && !lastTState)
    {
        // 切换
        if (g_currentTarget == g_targets[0])
            g_currentTarget = g_targets[1];
        else
            g_currentTarget = g_targets[0];
    }
    lastTState = currentT;

    // 只更新当前激活的目标的输入
    if (g_currentTarget)
        g_currentTarget->Update(g_input, g_deltaTime);

    // 相机跟随当前目标
    g_camera->Update(g_currentTarget, g_input, g_deltaTime);
    g_ui->Update(g_currentTarget, g_camera);   // UI需要修改为接受Target*
}

// Render frame
void RenderFrame()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    g_camera->ApplyProjection();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    g_camera->ApplyView(g_currentTarget);

    GLfloat lightPos[] = { 50.0f, 100.0f, 50.0f, 0.0f };
    GLfloat lightAmbient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat lightDiffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    g_scene->Render();
    // 渲染所有目标（车辆和飞行器都可见）
    for (int i = 0; i < 2; ++i)
        g_targets[i]->Render();
    g_ui->Render(g_renderer);

    SwapBuffers(g_hDC);
}

// Cleanup
void Cleanup()
{
    delete g_renderer;
    delete g_scene;
    delete g_targets[0];
    delete g_targets[1];
    delete g_camera;
    delete g_input;
    delete g_ui;

    if (g_hRC)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(g_hRC);
    }

    if (g_hDC)
    {
        ReleaseDC(g_hWnd, g_hDC);
    }
}
