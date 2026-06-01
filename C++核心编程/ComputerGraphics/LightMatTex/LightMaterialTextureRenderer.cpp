#include "LightMaterialTexture.h"
#include "LightMaterialTextureMath.h"

#include <gl/GL.h>
#include <cmath>
#include <vector>
#include <objidl.h>
#include <gdiplus.h>

namespace {

struct GLRendererState {
    HDC dc = nullptr;
    HGLRC rc = nullptr;
    bool initialized = false;
    GLuint diffuseTexture = 0;
    float uploadedTextureBlend = -1.0f;
};

GLRendererState g_gl;

struct TextureData {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;
    std::wstring loadedFile;

    Vec3 Sample(float u, float v) const {
        if (width == 0 || height == 0 || pixels.empty()) return {1.0f, 1.0f, 1.0f};
        u = u - std::floor(u);
        v = v - std::floor(v);
        if (u < 0.0f) u += 1.0f;
        if (v < 0.0f) v += 1.0f;
        int x = static_cast<int>(u * width) % width;
        int y = static_cast<int>(v * height) % height;
        int idx = (y * width + x) * 4;
        return {
            pixels[idx + 2] / 255.0f, // R
            pixels[idx + 1] / 255.0f, // G
            pixels[idx + 0] / 255.0f  // B
        };
    }
};

TextureData LoadImageGDIPlus(const std::wstring& path) {
    TextureData data;
    if (path.empty()) return data;
    Gdiplus::Bitmap bmp(path.c_str());
    if (bmp.GetLastStatus() != Gdiplus::Ok) return data;

    data.width = bmp.GetWidth();
    data.height = bmp.GetHeight();
    Gdiplus::Rect rect(0, 0, data.width, data.height);
    Gdiplus::BitmapData bmpData;
    bmp.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData);

    data.pixels.resize(data.width * data.height * 4);
    const unsigned char* src = static_cast<const unsigned char*>(bmpData.Scan0);
    for (int y = 0; y < data.height; ++y) {
        for (int x = 0; x < data.width; ++x) {
            int sIdx = y * std::abs(bmpData.Stride) + x * 4;
            int dIdx = (y * data.width + x) * 4;
            data.pixels[dIdx + 0] = src[sIdx + 2]; // R
            data.pixels[dIdx + 1] = src[sIdx + 1]; // G
            data.pixels[dIdx + 2] = src[sIdx + 0]; // B
            data.pixels[dIdx + 3] = src[sIdx + 3]; // A
        }
    }
    bmp.UnlockBits(&bmpData);
    data.loadedFile = path;
    return data;
}

std::wstring g_currentDiffuseFile = L"";
bool g_diffuseProcedural = true;
TextureData g_normalData;

Vec3 ColorToVec3(COLORREF color) {
    return {
        GetRValue(color) / 255.0f,
        GetGValue(color) / 255.0f,
        GetBValue(color) / 255.0f
    };
}

void SetupPixelFormat(HDC hdc) {
    PIXELFORMATDESCRIPTOR pfd {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelFormat, &pfd);
}

void GenerateDiffuseTexture(float blend) {
    constexpr int size = 128;
    float t = Clamp(blend, 0.0f, 1.0f);
    std::vector<unsigned char> pixels(size * size * 3);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            int tile = ((x / 16) + (y / 16)) & 1;
            int idx = (y * size + x) * 3;
            int patternR = tile ? 220 : 92;
            int patternG = tile ? 232 : 118;
            int patternB = tile ? 255 : 148;
            pixels[idx + 0] = static_cast<unsigned char>(255.0f + (patternR - 255.0f) * t);
            pixels[idx + 1] = static_cast<unsigned char>(255.0f + (patternG - 255.0f) * t);
            pixels[idx + 2] = static_cast<unsigned char>(255.0f + (patternB - 255.0f) * t);
        }
    }

    if (g_gl.diffuseTexture == 0) {
        glGenTextures(1, &g_gl.diffuseTexture);
    }
    glBindTexture(GL_TEXTURE_2D, g_gl.diffuseTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    g_gl.uploadedTextureBlend = blend;
}

void EnsureDiffuseTexture() {
    if (g_app.material.diffuseMapEnabled) {
        if (!g_app.material.diffuseMapFile.empty()) {
            if (g_currentDiffuseFile != g_app.material.diffuseMapFile) {
                TextureData td = LoadImageGDIPlus(g_app.material.diffuseMapFile);
                if (td.width > 0) {
                    if (g_gl.diffuseTexture == 0) glGenTextures(1, &g_gl.diffuseTexture);
                    glBindTexture(GL_TEXTURE_2D, g_gl.diffuseTexture);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, td.width, td.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, td.pixels.data());
                    g_currentDiffuseFile = g_app.material.diffuseMapFile;
                    g_diffuseProcedural = false;
                } else {
                    if (!g_diffuseProcedural) {
                        GenerateDiffuseTexture(g_app.material.textureBlend);
                        g_currentDiffuseFile = L"";
                        g_diffuseProcedural = true;
                    }
                }
            }
        } else {
            if (!g_diffuseProcedural) {
                GenerateDiffuseTexture(g_app.material.textureBlend);
                g_currentDiffuseFile = L"";
                g_diffuseProcedural = true;
            }
        }
    } else {
        if (g_gl.diffuseTexture == 0 || std::fabs(g_gl.uploadedTextureBlend - g_app.material.textureBlend) > 0.001f) {
            GenerateDiffuseTexture(g_app.material.textureBlend);
        }
    }
}

void EnsureNormalTexture() {
    if (g_app.material.normalMapEnabled && !g_app.material.normalMapFile.empty()) {
        if (g_normalData.loadedFile != g_app.material.normalMapFile) {
            g_normalData = LoadImageGDIPlus(g_app.material.normalMapFile);
            if (g_normalData.width == 0) g_normalData.loadedFile = L"";
        }
    }
}

bool EnsureOpenGL(HWND hwnd) {
    if (g_gl.initialized) {
        return true;
    }

    g_gl.dc = GetDC(hwnd);
    if (!g_gl.dc) {
        return false;
    }

    SetupPixelFormat(g_gl.dc);
    g_gl.rc = wglCreateContext(g_gl.dc);
    if (!g_gl.rc) {
        return false;
    }

    if (!wglMakeCurrent(g_gl.dc, g_gl.rc)) {
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.05f, 0.09f, 0.14f, 1.0f);

    GenerateDiffuseTexture(g_app.material.textureBlend);

    g_gl.initialized = true;
    return true;
}

void SetProjection(int width, int height) {
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float fovY = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    float top = std::tan(fovY * 0.5f * kPi / 180.0f) * nearPlane;
    float right = top * aspect;
    glFrustum(-right, right, -top, top, nearPlane, farPlane);
    glMatrixMode(GL_MODELVIEW);
}

void ConfigureLights() {
    Vec3 ambient = { g_app.ambientLightIntensity, g_app.ambientLightIntensity, g_app.ambientLightIntensity };
    GLfloat ambientModel[] = { ambient.x, ambient.y, ambient.z, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientModel);

    auto setupLight = [](GLenum id, const LightState& light) {
        Vec3 c = ColorToVec3(light.color);
        GLfloat diffuse[] = {
            c.x * light.intensity,
            c.y * light.intensity,
            c.z * light.intensity,
            1.0f
        };
        GLfloat specular[] = {
            c.x * light.intensity,
            c.y * light.intensity,
            c.z * light.intensity,
            1.0f
        };
        GLfloat position[] = { light.position.x, light.position.y, light.position.z, 1.0f };
        GLfloat zero[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glLightfv(id, GL_POSITION, position);
        glLightfv(id, GL_DIFFUSE, light.enabled ? diffuse : zero);
        glLightfv(id, GL_SPECULAR, light.enabled ? specular : zero);
        glLightf(id, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(id, GL_LINEAR_ATTENUATION, 0.08f);
        glLightf(id, GL_QUADRATIC_ATTENUATION, 0.02f);
    };

    setupLight(GL_LIGHT0, g_app.light1);
    setupLight(GL_LIGHT1, g_app.light2);
}

Vec3 PerturbNormal(const Vec3& n, float u, float v) {
    if (!g_app.material.normalMapEnabled) {
        return n;
    }

    float scale = g_app.material.textureScale * 10.0f;
    float strength = g_app.material.normalStrength;
    float nx = 0, ny = 0, nz = 0;

    if (g_normalData.width > 0) {
        Vec3 rgb = g_normalData.Sample(u * scale, v * scale);
        nx = (rgb.x * 2.0f - 1.0f) * strength;
        ny = (rgb.y * 2.0f - 1.0f) * strength;
        nz = (rgb.z * 2.0f - 1.0f) * strength;
    } else {
        nx = std::sin(u * scale * 2.0f * kPi) * strength;
        ny = std::cos(v * scale * 2.0f * kPi) * strength;
    }

    Vec3 p = Normalize({ n.x + nx, n.y + ny, n.z + nx * ny });
    return p;
}

void SetObjectMaterial() {
    Vec3 base = ColorToVec3(g_app.material.color);
    Vec3 ambientColor = ColorToVec3(g_app.material.ambientColor);
    Vec3 specularColor = ColorToVec3(g_app.material.specularColor);
    Vec3 emissiveColor = ColorToVec3(g_app.material.emissiveColor);
    float alpha = g_app.material.opacity;
    GLfloat ambient[] = {
        ambientColor.x * g_app.material.ambient,
        ambientColor.y * g_app.material.ambient,
        ambientColor.z * g_app.material.ambient,
        alpha
    };
    GLfloat diffuse[] = {
        base.x * g_app.material.diffuse,
        base.y * g_app.material.diffuse,
        base.z * g_app.material.diffuse,
        alpha
    };
    float specularStrength = g_app.material.specular;
    GLfloat specular[] = {
        specularColor.x * specularStrength,
        specularColor.y * specularStrength,
        specularColor.z * specularStrength,
        alpha
    };
    GLfloat emissive[] = {
        emissiveColor.x * g_app.material.emissive,
        emissiveColor.y * g_app.material.emissive,
        emissiveColor.z * g_app.material.emissive,
        alpha
    };

    if (alpha < 0.999f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
    } else {
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emissive);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, Clamp(g_app.material.shininess, 2.0f, 128.0f));

    if (g_app.material.diffuseMapEnabled) {
        EnsureDiffuseTexture();
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_gl.diffuseTexture);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    } else {
        glDisable(GL_TEXTURE_2D);
    }
}

void DrawGround() {
    glDisable(GL_TEXTURE_2D);
    GLfloat groundAmbient[] = { 0.04f, 0.05f, 0.06f, 1.0f };
    GLfloat groundDiffuse[] = { 0.08f, 0.10f, 0.12f, 1.0f };
    GLfloat groundSpecular[] = { 0.03f, 0.03f, 0.03f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, groundAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, groundDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, groundSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 6.0f);

    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-5.5f, -1.7f, -5.5f);
    glVertex3f(5.5f, -1.7f, -5.5f);
    glVertex3f(5.5f, -1.7f, 5.5f);
    glVertex3f(-5.5f, -1.7f, 5.5f);
    glEnd();

    glDisable(GL_LIGHTING);
    glColor3f(0.25f, 0.32f, 0.38f);
    glBegin(GL_LINES);
    for (int i = -5; i <= 5; ++i) {
        glVertex3f(static_cast<float>(i), -1.69f, -5.0f);
        glVertex3f(static_cast<float>(i), -1.69f, 5.0f);
        glVertex3f(-5.0f, -1.69f, static_cast<float>(i));
        glVertex3f(5.0f, -1.69f, static_cast<float>(i));
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void DrawCube() {
    constexpr float s = 1.0f;
    struct Face {
        Vec3 normal;
        Vec3 verts[4];
    };
    const Face faces[] = {
        {{ 0, 0, 1 }, {{-s,-s, s}, { s,-s, s}, { s, s, s}, {-s, s, s}}},
        {{ 0, 0,-1 }, {{ s,-s,-s}, {-s,-s,-s}, {-s, s,-s}, { s, s,-s}}},
        {{ 1, 0, 0 }, {{ s,-s, s}, { s,-s,-s}, { s, s,-s}, { s, s, s}}},
        {{-1, 0, 0 }, {{-s,-s,-s}, {-s,-s, s}, {-s, s, s}, {-s, s,-s}}},
        {{ 0, 1, 0 }, {{-s, s, s}, { s, s, s}, { s, s,-s}, {-s, s,-s}}},
        {{ 0,-1, 0 }, {{-s,-s,-s}, { s,-s,-s}, { s,-s, s}, {-s,-s, s}}}
    };

    const float tex[4][2] = { {0,0}, {1,0}, {1,1}, {0,1} };
    glBegin(GL_QUADS);
    for (const auto& face : faces) {
        for (int i = 0; i < 4; ++i) {
            Vec3 n = PerturbNormal(face.normal, tex[i][0], tex[i][1]);
            glNormal3f(n.x, n.y, n.z);
            glTexCoord2f(tex[i][0] * g_app.material.textureScale, tex[i][1] * g_app.material.textureScale);
            glVertex3f(face.verts[i].x, face.verts[i].y, face.verts[i].z);
        }
    }
    glEnd();
}

void DrawSphere() {
    const int stacks = 28;
    const int slices = 32;
    const float radius = 1.2f;
    for (int i = 0; i < stacks; ++i) {
        float v0 = static_cast<float>(i) / stacks;
        float v1 = static_cast<float>(i + 1) / stacks;
        float phi0 = (v0 - 0.5f) * kPi;
        float phi1 = (v1 - 0.5f) * kPi;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float u = static_cast<float>(j) / slices;
            float theta = u * 2.0f * kPi;

            auto emit = [&](float phi, float v) {
                Vec3 n = {
                    std::cos(phi) * std::cos(theta),
                    std::sin(phi),
                    std::cos(phi) * std::sin(theta)
                };
                n = PerturbNormal(n, u, v);
                glNormal3f(n.x, n.y, n.z);
                glTexCoord2f(u * g_app.material.textureScale, v * g_app.material.textureScale);
                glVertex3f(radius * std::cos(phi) * std::cos(theta),
                           radius * std::sin(phi),
                           radius * std::cos(phi) * std::sin(theta));
            };

            emit(phi0, v0);
            emit(phi1, v1);
        }
        glEnd();
    }
}

void DrawTorus() {
    const int rings = 40;
    const int sides = 20;
    const float majorRadius = 1.35f;
    const float minorRadius = 0.42f;

    for (int i = 0; i < rings; ++i) {
        float u0 = static_cast<float>(i) / rings;
        float u1 = static_cast<float>(i + 1) / rings;
        float theta0 = u0 * 2.0f * kPi;
        float theta1 = u1 * 2.0f * kPi;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= sides; ++j) {
            float v = static_cast<float>(j) / sides;
            float phi = v * 2.0f * kPi;

            auto emit = [&](float theta, float u) {
                float c = std::cos(theta);
                float s = std::sin(theta);
                float cp = std::cos(phi);
                float sp = std::sin(phi);
                float r = majorRadius + minorRadius * cp;
                Vec3 n = { c * cp, sp, s * cp };
                n = PerturbNormal(n, u, v);
                glNormal3f(n.x, n.y, n.z);
                glTexCoord2f(u * g_app.material.textureScale, v * g_app.material.textureScale);
                glVertex3f(c * r, minorRadius * sp, s * r);
            };

            emit(theta0, u0);
            emit(theta1, u1);
        }
        glEnd();
    }
}

void DrawActiveObject() {
    SetObjectMaterial();
    EnsureDiffuseTexture();
    EnsureNormalTexture();

    glPushMatrix();
    glTranslatef(0.0f, -0.05f, 0.0f);
    glRotatef(g_app.userPitch * 0.65f * 180.0f / kPi, 1.0f, 0.0f, 0.0f);
    glRotatef((g_app.autoRotation + g_app.userYaw * 0.45f) * 180.0f / kPi, 0.0f, 1.0f, 0.0f);

    switch (g_app.activeObject) {
    case ObjectType::Cube:
        DrawCube();
        break;
    case ObjectType::Sphere:
        DrawSphere();
        break;
    case ObjectType::Torus:
        DrawTorus();
        break;
    }

    glPopMatrix();
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

} // namespace

void PaintScene(HWND hwnd, HDC) {
    if (!EnsureOpenGL(hwnd)) {
        return;
    }

    RECT rc {};
    GetClientRect(hwnd, &rc);
    int width = (std::max)(1, static_cast<int>(rc.right - rc.left));
    int height = (std::max)(1, static_cast<int>(rc.bottom - rc.top));

    SetProjection(width, height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0.0f, 0.15f, -g_app.cameraDistance);

    ConfigureLights();
    DrawGround();
    DrawActiveObject();

    SwapBuffers(g_gl.dc);
}

void DestroySceneRenderer() {
    if (!g_gl.initialized) {
        return;
    }

    if (g_gl.rc) {
        wglMakeCurrent(g_gl.dc, g_gl.rc);
        if (g_gl.diffuseTexture != 0) {
            glDeleteTextures(1, &g_gl.diffuseTexture);
            g_gl.diffuseTexture = 0;
        }
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(g_gl.rc);
        g_gl.rc = nullptr;
    }

    if (g_gl.dc && g_app.controls.scene) {
        ReleaseDC(g_app.controls.scene, g_gl.dc);
        g_gl.dc = nullptr;
    }

    g_gl.initialized = false;
    g_gl.uploadedTextureBlend = -1.0f;
}
