#include "LightMaterialTexture.h"
#include "LightMaterialTextureMath.h"

#include <gl/GL.h>
#include <cmath>
#include <vector>
#include <map>
#include <objidl.h>
#include <gdiplus.h>

namespace {

    struct GLRendererState {
        HDC dc = nullptr;
        HGLRC rc = nullptr;
        bool initialized = false;
    };
    GLRendererState g_gl;

    struct TextureData {
        int width = 0;
        int height = 0;
        std::vector<unsigned char> pixels;

        Vec3 Sample(float u, float v) const {
            if (width == 0 || height == 0 || pixels.empty()) return { 1.0f, 1.0f, 1.0f };
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
        return data;
    }

    std::map<std::wstring, GLuint> g_textureCache;
    std::map<std::wstring, TextureData> g_normalDataCache;

    GLuint GetTexture(const std::wstring& path) {
        if (path.empty()) return 0;
        if (g_textureCache.count(path)) return g_textureCache[path];
        TextureData td = LoadImageGDIPlus(path);
        if (td.width == 0) return 0;

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, td.width, td.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, td.pixels.data());
        g_textureCache[path] = tex;
        return tex;
    }

    const TextureData* GetNormalDataPtr(const std::wstring& path) {
        if (path.empty()) return nullptr;
        if (g_normalDataCache.find(path) == g_normalDataCache.end()) {
            g_normalDataCache[path] = LoadImageGDIPlus(path);
        }
        return &g_normalDataCache[path];
    }

    Vec3 ColorToVec3(COLORREF color) {
        return { GetRValue(color) / 255.0f, GetGValue(color) / 255.0f, GetBValue(color) / 255.0f };
    }

    void SetupPixelFormat(HDC hdc) {
        PIXELFORMATDESCRIPTOR pfd{};
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

    bool EnsureOpenGL(HWND hwnd) {
        if (g_gl.initialized) return true;
        g_gl.dc = GetDC(hwnd);
        if (!g_gl.dc) return false;
        SetupPixelFormat(g_gl.dc);
        g_gl.rc = wglCreateContext(g_gl.dc);
        if (!g_gl.rc) return false;
        if (!wglMakeCurrent(g_gl.dc, g_gl.rc)) return false;

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glEnable(GL_NORMALIZE);
        glShadeModel(GL_SMOOTH);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        g_gl.initialized = true;
        return true;
    }

    void SetProjection(int width, int height) {
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        glViewport(0, 0, width, height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float fovY = 45.0f;
        float nearPlane = 0.1f, farPlane = 100.0f;
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
            GLfloat diffuse[] = { c.x * light.intensity, c.y * light.intensity, c.z * light.intensity, 1.0f };
            GLfloat specular[] = { c.x * light.intensity, c.y * light.intensity, c.z * light.intensity, 1.0f };
            GLfloat position[] = { light.position.x, light.position.y, light.position.z, 1.0f }; // w=1 指明是点光源
            GLfloat zero[] = { 0.0f, 0.0f, 0.0f, 1.0f };
            glLightfv(id, GL_POSITION, position);
            glLightfv(id, GL_DIFFUSE, light.enabled ? diffuse : zero);
            glLightfv(id, GL_SPECULAR, light.enabled ? specular : zero);
            glLightf(id, GL_CONSTANT_ATTENUATION, 1.0f);
            glLightf(id, GL_LINEAR_ATTENUATION, 0.04f);
            glLightf(id, GL_QUADRATIC_ATTENUATION, 0.008f);
            };

        setupLight(GL_LIGHT0, g_app.light1);
        setupLight(GL_LIGHT1, g_app.light2);
    }

    Vec3 PerturbNormal(const Vec3& n, float u, float v, const MaterialState& mat) {
        if (!mat.normalMapEnabled) return n;
        float scale = mat.textureScale * 10.0f;
        float strength = mat.normalStrength;
        float nx = 0, ny = 0, nz = 0;

        const TextureData* ndata = GetNormalDataPtr(mat.normalMapFile);
        if (ndata && ndata->width > 0) {
            Vec3 rgb = ndata->Sample(u * scale, v * scale);
            nx = (rgb.x * 2.0f - 1.0f) * strength;
            ny = (rgb.y * 2.0f - 1.0f) * strength;
            nz = (rgb.z * 2.0f - 1.0f) * strength;
        }
        else {
            nx = std::sin(u * scale * 2.0f * kPi) * strength;
            ny = std::cos(v * scale * 2.0f * kPi) * strength;
        }
        return Normalize({ n.x + nx, n.y + ny, n.z + nx * ny });
    }

    void ApplyMaterial(const MaterialState& mat, bool applyTexture) {
        Vec3 base = ColorToVec3(mat.color);
        Vec3 ambCol = ColorToVec3(mat.ambientColor);
        Vec3 specCol = ColorToVec3(mat.specularColor);
        Vec3 emisCol = ColorToVec3(mat.emissiveColor);
        float alpha = mat.opacity;

        GLfloat ambient[] = { ambCol.x * mat.ambient, ambCol.y * mat.ambient, ambCol.z * mat.ambient, alpha };
        GLfloat diffuse[] = { base.x * mat.diffuse, base.y * mat.diffuse, base.z * mat.diffuse, alpha };
        GLfloat specular[] = { specCol.x * mat.specular, specCol.y * mat.specular, specCol.z * mat.specular, alpha };
        GLfloat emissive[] = { emisCol.x * mat.emissive, emisCol.y * mat.emissive, emisCol.z * mat.emissive, alpha };

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emissive);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, Clamp(mat.shininess, 1e-3f, 128.0f));

        if (alpha < 0.99f) {
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
        }
        else {
            glDisable(GL_BLEND); glDepthMask(GL_TRUE);
        }

        if (applyTexture && mat.diffuseMapEnabled && !mat.diffuseMapFile.empty()) {
            GLuint tex = GetTexture(mat.diffuseMapFile);
            if (tex) {
                glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            }
            else glDisable(GL_TEXTURE_2D);
        }
        else {
            glDisable(GL_TEXTURE_2D);
        }
    }

    void ApplyWallMaterial(const WallState& state) {
        Vec3 c = ColorToVec3(state.color);
        GLfloat ambient[] = { c.x * 0.2f, c.y * 0.2f, c.z * 0.2f, 1.0f };
        GLfloat diffuse[] = { c.x, c.y, c.z, 1.0f };
        GLfloat zero[] = { 0, 0, 0, 1 };

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
        glDisable(GL_BLEND); glDepthMask(GL_TRUE);

        if (state.textureEnabled && !state.textureFile.empty()) {
            GLuint tex = GetTexture(state.textureFile);
            if (tex) {
                glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            }
            else glDisable(GL_TEXTURE_2D);
        }
        else {
            glDisable(GL_TEXTURE_2D);
        }
    }

    void DrawCornellBox() {
        const float s = 5.0f; // 盒子半尺寸
        struct Vertex { Vec3 p, n; float u, v; };

        // 增强版 drawQuad：绘制墙面的同时，向模板缓冲写入对应的墙面 Stencil ID
        auto drawQuad = [](const Vertex v[4], const WallState& state, int stencilId) {
            glStencilFunc(GL_ALWAYS, stencilId, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

            ApplyWallMaterial(state);
            glBegin(GL_QUADS);
            for (int i = 0; i < 4; ++i) {
                glNormal3f(v[i].n.x, v[i].n.y, v[i].n.z);
                glTexCoord2f(v[i].u * 2.0f, v[i].v * 2.0f);
                glVertex3f(v[i].p.x, v[i].p.y, v[i].p.z);
            }
            glEnd();
            };

        // 左墙 (+X 朝内) -> 标记为 ID 1
        Vertex left[4] = { {{-s,-s,s},{1,0,0},0,0}, {{-s,-s,-s},{1,0,0},1,0}, {{-s,s,-s},{1,0,0},1,1}, {{-s,s,s},{1,0,0},0,1} };
        drawQuad(left, g_app.walls[0], 1);

        // 右墙 (-X 朝内) -> 标记为 ID 2
        Vertex right[4] = { {{s,-s,-s},{-1,0,0},0,0}, {{s,-s,s},{-1,0,0},1,0}, {{s,s,s},{-1,0,0},1,1}, {{s,s,-s},{-1,0,0},0,1} };
        drawQuad(right, g_app.walls[1], 2);

        // 后墙 (+Z 朝内) -> 标记为 ID 3
        Vertex back[4] = { {{-s,-s,-s},{0,0,1},0,0}, {{s,-s,-s},{0,0,1},1,0}, {{s,s,-s},{0,0,1},1,1}, {{-s,s,-s},{0,0,1},0,1} };
        drawQuad(back, g_app.walls[2], 3);

        // 顶墙 (-Y 朝内) -> 标记为 ID 4
        Vertex top[4] = { {{-s,s,-s},{0,-1,0},0,1}, {{s,s,-s},{0,-1,0},1,1}, {{s,s,s},{0,-1,0},1,0}, {{-s,s,s},{0,-1,0},0,0} };
        drawQuad(top, g_app.walls[3], 4);

        // 底墙/地板 (+Y 朝内) -> 标记为 ID 5
        Vertex bottom[4] = { {{-s,-s,s},{0,1,0},0,0}, {{s,-s,s},{0,1,0},1,0}, {{s,-s,-s},{0,1,0},1,1}, {{-s,-s,-s},{0,1,0},0,1} };
        drawQuad(bottom, g_app.walls[4], 5);
    }
    // 构建平面阴影矩阵 (Blinn's Shadow Matrix)
    // ground: 平面方程 Ax + By + Cz + D = 0 的四个系数
    // light: 光源位置 [X, Y, Z, W]
    void BuildShadowMatrix(const float ground[4], const float light[4], float shadowMat[16]) {
        float dot = ground[0] * light[0] + ground[1] * light[1] + ground[2] * light[2] + ground[3] * light[3];
        shadowMat[0] = dot - light[0] * ground[0];
        shadowMat[1] = 0.0f - light[1] * ground[0];
        shadowMat[2] = 0.0f - light[2] * ground[0];
        shadowMat[3] = 0.0f - light[3] * ground[0];

        shadowMat[4] = 0.0f - light[0] * ground[1];
        shadowMat[5] = dot - light[1] * ground[1];
        shadowMat[6] = 0.0f - light[2] * ground[1];
        shadowMat[7] = 0.0f - light[3] * ground[1];

        shadowMat[8] = 0.0f - light[0] * ground[2];
        shadowMat[9] = 0.0f - light[1] * ground[2];
        shadowMat[10] = dot - light[2] * ground[2];
        shadowMat[11] = 0.0f - light[3] * ground[2];

        shadowMat[12] = 0.0f - light[0] * ground[3];
        shadowMat[13] = 0.0f - light[1] * ground[3];
        shadowMat[14] = 0.0f - light[2] * ground[3];
        shadowMat[15] = dot - light[3] * ground[3];
    }

    void DrawCube(const MaterialState& mat) {
        constexpr float s = 1.0f;
        struct Face { Vec3 normal; Vec3 verts[4]; };
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
                Vec3 n = PerturbNormal(face.normal, tex[i][0], tex[i][1], mat);
                glNormal3f(n.x, n.y, n.z);
                glTexCoord2f(tex[i][0] * mat.textureScale, tex[i][1] * mat.textureScale);
                glVertex3f(face.verts[i].x, face.verts[i].y, face.verts[i].z);
            }
        }
        glEnd();
    }

    void DrawSphere(const MaterialState& mat) {
        const int stacks = 28, slices = 32;
        const float radius = 1.2f;
        for (int i = 0; i < stacks; ++i) {
            float v0 = static_cast<float>(i) / stacks, v1 = static_cast<float>(i + 1) / stacks;
            float phi0 = (v0 - 0.5f) * kPi, phi1 = (v1 - 0.5f) * kPi;
            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= slices; ++j) {
                float u = static_cast<float>(j) / slices;
                float theta = u * 2.0f * kPi;
                auto emit = [&](float phi, float v) {
                    Vec3 n = { std::cos(phi) * std::cos(theta), std::sin(phi), std::cos(phi) * std::sin(theta) };
                    n = PerturbNormal(n, u, v, mat);
                    glNormal3f(n.x, n.y, n.z);
                    glTexCoord2f(u * mat.textureScale, v * mat.textureScale);
                    glVertex3f(radius * std::cos(phi) * std::cos(theta), radius * std::sin(phi), radius * std::cos(phi) * std::sin(theta));
                    };
                emit(phi0, v0); emit(phi1, v1);
            }
            glEnd();
        }
    }

    void DrawTorus(const MaterialState& mat) {
        const int rings = 40, sides = 20;
        const float majorRadius = 1.35f, minorRadius = 0.42f;
        for (int i = 0; i < rings; ++i) {
            float u0 = static_cast<float>(i) / rings, u1 = static_cast<float>(i + 1) / rings;
            float theta0 = u0 * 2.0f * kPi, theta1 = u1 * 2.0f * kPi;
            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= sides; ++j) {
                float v = static_cast<float>(j) / sides;
                float phi = v * 2.0f * kPi;
                auto emit = [&](float theta, float u) {
                    float c = std::cos(theta), s = std::sin(theta);
                    float cp = std::cos(phi), sp = std::sin(phi);
                    float r = majorRadius + minorRadius * cp;
                    Vec3 n = PerturbNormal({ c * cp, sp, s * cp }, u, v, mat);
                    glNormal3f(n.x, n.y, n.z);
                    glTexCoord2f(u * mat.textureScale, v * mat.textureScale);
                    glVertex3f(c * r, minorRadius * sp, s * r);
                    };
                emit(theta0, u0); emit(theta1, u1);
            }
            glEnd();
        }
    }

    void DrawSceneObjects() {
        float rotY = (g_app.autoRotation) * 180.0f / kPi;

        // 模型 0: Cube (左侧放置)
        glPushMatrix();
        glTranslatef(-2.5f, -2.0f, 1.0f);
        glRotatef(rotY, 0.0f, 1.0f, 0.0f);
        ApplyMaterial(g_app.objectMaterials[0], true);
        DrawCube(g_app.objectMaterials[0]);
        glPopMatrix();

        // 模型 1: Sphere (居中靠后放置)
        glPushMatrix();
        glTranslatef(0.0f, -1.8f, -1.0f);
        glRotatef(rotY, 0.0f, 1.0f, 0.0f);
        ApplyMaterial(g_app.objectMaterials[1], true);
        DrawSphere(g_app.objectMaterials[1]);
        glPopMatrix();

        // 模型 2: Torus (右侧放置)
        glPushMatrix();
        glTranslatef(2.5f, -2.58f, 1.0f); // 底部紧贴盒底 (-5 + 0.42 minorRadius)
        glRotatef(rotY, 0.0f, 1.0f, 0.0f);
        ApplyMaterial(g_app.objectMaterials[2], true);
        DrawTorus(g_app.objectMaterials[2]);
        glPopMatrix();

        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }
    // 为指定光源在各个墙面上渲染平面阴影
    void DrawPlanarShadowsForLight(const LightState& light) {
        // 【动态控制】：如果光源未开启，或者强度小于等于 0，则直接不绘制任何阴影
        if (!light.enabled || light.intensity <= 0.0f) return;

        // 点光源位置 W = 1.0f
        float lpos[4] = { light.position.x, light.position.y, light.position.z, 1.0f };

        // 对应 Cornell Box 的各面墙壁方程 (半尺寸 s = 5.0f)
        struct ShadowPlane {
            int stencilId;
            float eq[4];
        };
        ShadowPlane planes[] = {
            { 1, { 1.0f,  0.0f,  0.0f, 5.0f} }, // 左墙: x = -5  ->  1x + 5 = 0
            { 2, {-1.0f,  0.0f,  0.0f, 5.0f} }, // 右墙: x = 5   -> -1x + 5 = 0
            { 3, { 0.0f,  0.0f,  1.0f, 5.0f} }, // 后墙: z = -5  ->  1z + 5 = 0
            { 5, { 0.0f,  1.0f,  0.0f, 5.0f} }  // 地板: y = -5  ->  1y + 5 = 0
        };

        // 关闭光照和纹理，开启混合用来渲染半透明黑色阴影
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // 锁死深度缓冲区更新，防止阴影几何体自身或相互之间破坏深度测试
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);

        // 开启多边形偏移（Polygon Offset），完美解决阴影与墙面共面导致的闪烁（Z-Fighting）
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);

        float shadowMat[16];
        float rotY = g_app.autoRotation * 180.0f / kPi;

        // 【优化 1】：使阴影浓度（Alpha）随灯光强度（intensity）动态线性变化
        // 乘以 0.4f 映射系数，并设置 0.75f 的最高上限防止影子完全死黑。
        float shadowAlpha = Clamp(light.intensity * 0.4f, 0.0f, 0.75f);

        for (const auto& p : planes) {
            // 计算当前光源打在特定墙面上的压扁投影矩阵
            BuildShadowMatrix(p.eq, lpos, shadowMat);

            // =================================================================
            // 阶段 1：同一光源下的阴影区域模板标记（闭合颜色缓冲区，只去重不着色）
            // =================================================================
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // 禁止写入颜色

            // 只有属于当前墙面 ID 的像素才进行测试
            glStencilFunc(GL_EQUAL, p.stencilId, 0xFF);
            // 核心魔法：通过测试后，将模板值加 1（变为 stencilId + 1）
            // 如果立方体和球体的影子在此处重叠，后续几何体进来时由于值已变成 stencilId + 1 导致测试失败，
            // 从而实现单光源下多物体阴影的完美去重，不会产生二次重叠产生的黑色色块。
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

            glPushMatrix();
            glMultMatrixf(shadowMat);

            // 绘制三大物体的阴影几何体以更新模板缓冲
            glPushMatrix(); glTranslatef(-2.5f, -2.0f, 1.0f); glRotatef(rotY, 0.0f, 1.0f, 0.0f);
            DrawCube(g_app.objectMaterials[0]); glPopMatrix();

            glPushMatrix(); glTranslatef(0.0f, -1.8f, -1.0f); glRotatef(rotY, 0.0f, 1.0f, 0.0f);
            DrawSphere(g_app.objectMaterials[1]); glPopMatrix();

            glPushMatrix(); glTranslatef(2.5f, -2.58f, 1.0f); glRotatef(rotY, 0.0f, 1.0f, 0.0f);
            DrawTorus(g_app.objectMaterials[2]); glPopMatrix();

            glPopMatrix();

            // =================================================================
            // 阶段 2：真正绘制当前光源的阴影颜色，并完美还原模板缓冲值
            // =================================================================
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // 重新打开颜色写入

            // 只有刚才被成功标记为阴影（值为 stencilId + 1）的有效像素才执行着色
            glStencilFunc(GL_EQUAL, p.stencilId + 1, 0xFF);
            // 关键还原：着色完成之后将模板缓冲值减 1（干净地还原回墙面原本的 stencilId）
            // 这步操作清空了现场，使得下一个光源（如 light2）进来时能基于相同的原墙面 ID 独立计算！
            glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);

            glPushMatrix();
            glMultMatrixf(shadowMat);

            // 使用受到灯光强度联动控制的半透明黑色
            glColor4f(0.0f, 0.0f, 0.0f, shadowAlpha);

            // 再次绘制三大物体，将计算好的单层阴影颜色输出到屏幕
            glPushMatrix(); glTranslatef(-2.5f, -2.0f, 1.0f); glRotatef(rotY, 0.0f, 1.0f, 0.0f);
            DrawCube(g_app.objectMaterials[0]); glPopMatrix();

            glPushMatrix(); glTranslatef(0.0f, -1.8f, -1.0f); glRotatef(rotY, 0.0f, 1.0f, 0.0f);
            DrawSphere(g_app.objectMaterials[1]); glPopMatrix();

            glPushMatrix(); glTranslatef(2.5f, -2.58f, 1.0f); glRotatef(rotY, 0.0f, 1.0f, 0.0f);
            DrawTorus(g_app.objectMaterials[2]); glPopMatrix();

            glPopMatrix();
        }

        // 恢复原有渲染状态，避免污染正常的场景绘制
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }
} // namespace

void PaintScene(HWND hwnd, HDC) {
    if (!EnsureOpenGL(hwnd)) return;
    RECT rc{}; GetClientRect(hwnd, &rc);
    int width = (std::max)(1, static_cast<int>(rc.right - rc.left));
    int height = (std::max)(1, static_cast<int>(rc.bottom - rc.top));

    SetProjection(width, height);

    // 【修改】：开启模板测试，并清除 颜色、深度 以及 模板 缓冲区
    glEnable(GL_STENCIL_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity();

    // 应用摄像机变换
    glTranslatef(0.0f, 0.0f, -g_app.cameraDistance);
    glRotatef(g_app.userPitch * 180.0f / kPi, 1.0f, 0.0f, 0.0f);
    glRotatef(g_app.userYaw * 180.0f / kPi, 0.0f, 1.0f, 0.0f); // 沿用上一轮修改的房间旋转

    // 在固定世界坐标下设置光源位置
    ConfigureLights();

    // 1. 绘制 Cornell 盒子（在此期间墙体 ID 写入了 Stencil 缓冲）
    DrawCornellBox();

    // 2. 正常绘制 3D 核心物体
    DrawSceneObjects();

    // 3. 【新增】：为开启的光源分别绘制投影阴影
    DrawPlanarShadowsForLight(g_app.light1);
    DrawPlanarShadowsForLight(g_app.light2);

    glDisable(GL_STENCIL_TEST);

    SwapBuffers(g_gl.dc);
}

void DestroySceneRenderer() {
    if (!g_gl.initialized) return;
    if (g_gl.rc) {
        wglMakeCurrent(g_gl.dc, g_gl.rc);
        for (auto& pair : g_textureCache) glDeleteTextures(1, &pair.second);
        g_textureCache.clear();
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(g_gl.rc);
        g_gl.rc = nullptr;
    }
    if (g_gl.dc && g_app.controls.scene) { ReleaseDC(g_app.controls.scene, g_gl.dc); g_gl.dc = nullptr; }
    g_gl.initialized = false;
}