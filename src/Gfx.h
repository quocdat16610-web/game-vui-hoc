// =============================================================================
//  Gfx.h — đồ họa nâng cao: shader ánh sáng, bóng đổ (shadow map),
//  hậu kỳ (bloom, FXAA, vignette) và các khối hình có pháp tuyến.
//  (Bài 15: Shader & ánh sáng giải thích mọi thứ trong file này.)
// =============================================================================
#pragma once
#include "raylib.h"
#include <functional>

namespace gfx {
    enum Quality { Q_LOW = 0, Q_MEDIUM = 1, Q_HIGH = 2 };

    struct PointLight {
        Vector3 pos;
        Color   color;
        float   intensity;
        float   range;
    };

    // Toàn bộ "không khí" ánh sáng của một cảnh
    struct Environment {
        Vector3 sunDir = { 0.4f, -1.0f, -0.3f };
        Color   sunColor = { 255, 244, 225, 255 };
        float   sunIntensity = 1.0f;
        Color   sky = { 150, 170, 205, 255 };
        Color   ground = { 80, 66, 56, 255 };
        float   ambient = 0.55f;
        Color   fog = { 20, 22, 32, 255 };
        float   fogNear = 30, fogFar = 70;
        float   exposure = 1.0f;
        PointLight lights[8] = {};
        int     lightCount = 0;
        Vector3 shadowCenter = { 0, 0, 0 };
        float   shadowSize = 24;               // kích thước vùng có bóng đổ (mét)
        float   bloom = 0.6f, bloomThreshold = 0.72f;
        float   vignette = 0.35f, saturation = 1.08f, contrast = 1.04f;
    };

    void Init();
    void Shutdown();

    int  GetQuality();
    void SetQuality(int q);
    const char* QualityName();
    bool ShadowsOn();

    void SetEnvironment(const Environment& env);
    Environment& Env();

    // Khối hình có pháp tuyến (để được chiếu sáng đúng)
    void Box(Vector3 center, Vector3 size, Color col);
    void Sphere(Vector3 center, float r, Color col, int rings = 8, int slices = 12);
    void Cylinder(Vector3 a, Vector3 b, float ra, float rb, Color col, int slices = 12, bool caps = true);
    void Capsule(Vector3 a, Vector3 b, float r, Color col, int slices = 10);

    // Bóng đổ: vẽ cảnh từ góc nhìn của mặt trời vào shadow map
    void RenderShadowMap(const std::function<void()>& drawCasters);

    // Bật shader ánh sáng (gọi trong BeginMode3D)
    void BeginLit(const Camera3D& cam);
    void EndLit();

    // Vẽ vật phát sáng / trong suốt cộng màu (tia đèn, hạt...)
    void BeginGlow();
    void EndGlow();

    // Hậu kỳ: vẽ cảnh 3D vào texture rồi xử lý
    bool PostEnabled();
    void BeginScene();
    void EndScene();
    void PresentScene();

    // Hiệu ứng tức thời cho khung hình kế tiếp
    void SetAberration(float a);
    void SetFlash(float f);
}
