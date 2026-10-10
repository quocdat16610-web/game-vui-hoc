#include "Gfx.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <initializer_list>
#include <map>
#include <vector>

namespace {
    const int SHADOW_SLOT = 10;    // khe texture dành riêng cho shadow map

    struct LitLocs {
        int viewPos, sunDir, sunColor, sky, ground, fogColor, fogRange, exposure;
        int lightCount, lightPos, lightColor, lightRange;
        int useShadow, lightVP, shadowMap, shadowTexel;
    };

    struct State {
        int quality = gfx::Q_HIGH;
        Shader lit = {};
        bool litOk = false;
        LitLocs L = {};

        RenderTexture2D shadow = {};
        int shadowRes = 0;
        Matrix lightVP = MatrixIdentity();
        bool shadowValid = false;

        Shader bright = {}, blur = {}, fin = {};
        bool postOk = false;
        int locThreshold = -1, locDir = -1;
        int locBloomTex = -1, locRes = -1, locBloom = -1, locVig = -1, locSat = -1, locCon = -1,
            locAberr = -1, locFlash = -1, locTime = -1;
        RenderTexture2D scene = {}, half = {}, blurA = {}, blurB = {};
        int rtW = 0, rtH = 0;
        bool inScene = false;

        gfx::Environment env;
        float aberration = 0, flash = 0;
    };
    State G;

    Vector3 ToVec3(Color c, float k) { return { c.r / 255.0f * k, c.g / 255.0f * k, c.b / 255.0f * k }; }

    bool ShaderOk(const Shader& s) { return s.id != 0 && s.id != rlGetShaderIdDefault(); }

    // Framebuffer chỉ có texture độ sâu (theo ví dụ shaders_shadowmap của raylib)
    RenderTexture2D LoadShadowTarget(int size) {
        RenderTexture2D t = {};
        t.id = rlLoadFramebuffer();
        t.texture.width = size;
        t.texture.height = size;
        if (t.id > 0) {
            rlEnableFramebuffer(t.id);
            t.depth.id = rlLoadTextureDepth(size, size, false);
            t.depth.width = size;
            t.depth.height = size;
            t.depth.format = 19;
            t.depth.mipmaps = 1;
            rlFramebufferAttach(t.id, t.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
            if (!rlFramebufferComplete(t.id)) TraceLog(LOG_WARNING, "GFX: shadow map framebuffer khong hoan chinh");
            rlDisableFramebuffer();
        }
        return t;
    }

    void UnloadShadowTarget() {
        if (G.shadow.id > 0) {
            rlUnloadTexture(G.shadow.depth.id);
            rlUnloadFramebuffer(G.shadow.id);
        }
        G.shadow = {};
        G.shadowRes = 0;
    }

    void UnloadPostTargets() {
        if (G.rtW == 0) return;
        UnloadRenderTexture(G.scene);
        UnloadRenderTexture(G.half);
        UnloadRenderTexture(G.blurA);
        UnloadRenderTexture(G.blurB);
        G.rtW = G.rtH = 0;
    }

    void EnsurePostTargets() {
        int w = GetRenderWidth(), h = GetRenderHeight();
        if (w == G.rtW && h == G.rtH) return;
        UnloadPostTargets();
        G.scene = LoadRenderTexture(w, h);
        G.half  = LoadRenderTexture(w / 2, h / 2);
        G.blurA = LoadRenderTexture(w / 4, h / 4);
        G.blurB = LoadRenderTexture(w / 4, h / 4);
        for (RenderTexture2D* t : { &G.scene, &G.half, &G.blurA, &G.blurB })
            SetTextureFilter(t->texture, TEXTURE_FILTER_BILINEAR);
        G.rtW = w;
        G.rtH = h;
    }

    // ---- Lưới hình cầu đơn vị, tính sẵn một lần ----
    const std::vector<Vector3>& UnitSphere(int rings, int slices) {
        static std::map<int, std::vector<Vector3>> cache;
        int key = rings * 1000 + slices;
        auto it = cache.find(key);
        if (it != cache.end()) return it->second;
        std::vector<Vector3> tris;
        auto P = [&](int i, int j) {
            float lat = -PI / 2 + PI * i / rings;
            float lon = 2 * PI * j / slices;
            return Vector3{ std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon) };
        };
        auto addTri = [&](Vector3 a, Vector3 b, Vector3 c) {
            // Đảm bảo tam giác quay mặt ra ngoài (thứ tự ngược chiều kim đồng hồ)
            Vector3 n = Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a));
            Vector3 mid = Vector3Scale(Vector3Add(Vector3Add(a, b), c), 1.0f / 3.0f);
            if (Vector3DotProduct(n, mid) < 0) std::swap(b, c);
            tris.push_back(a); tris.push_back(b); tris.push_back(c);
        };
        for (int i = 0; i < rings; ++i)
            for (int j = 0; j < slices; ++j) {
                Vector3 a = P(i, j), b = P(i + 1, j), c = P(i + 1, j + 1), d = P(i, j + 1);
                if (i > 0) addTri(a, b, d);
                if (i < rings - 1) addTri(b, c, d);
            }
        return cache[key] = tris;
    }
}

// =============================================================================
void gfx::Init() {
    G.lit = LoadShader("assets/shaders/lit.vs", "assets/shaders/lit.fs");
    G.litOk = ShaderOk(G.lit);
    if (G.litOk) {
        Shader& s = G.lit;
        G.L.viewPos = GetShaderLocation(s, "viewPos");
        G.L.sunDir = GetShaderLocation(s, "sunDir");
        G.L.sunColor = GetShaderLocation(s, "sunColor");
        G.L.sky = GetShaderLocation(s, "skyColor");
        G.L.ground = GetShaderLocation(s, "groundColor");
        G.L.fogColor = GetShaderLocation(s, "fogColor");
        G.L.fogRange = GetShaderLocation(s, "fogRange");
        G.L.exposure = GetShaderLocation(s, "exposure");
        G.L.lightCount = GetShaderLocation(s, "lightCount");
        G.L.lightPos = GetShaderLocation(s, "lightPos");
        G.L.lightColor = GetShaderLocation(s, "lightColor");
        G.L.lightRange = GetShaderLocation(s, "lightRange");
        G.L.useShadow = GetShaderLocation(s, "useShadow");
        G.L.lightVP = GetShaderLocation(s, "lightVP");
        G.L.shadowMap = GetShaderLocation(s, "shadowMap");
        G.L.shadowTexel = GetShaderLocation(s, "shadowTexel");
    } else {
        TraceLog(LOG_WARNING, "GFX: khong nap duoc shader anh sang, dung do hoa don gian");
    }

    G.bright = LoadShader(0, "assets/shaders/post_bright.fs");
    G.blur = LoadShader(0, "assets/shaders/post_blur.fs");
    G.fin = LoadShader(0, "assets/shaders/post_final.fs");
    G.postOk = ShaderOk(G.bright) && ShaderOk(G.blur) && ShaderOk(G.fin);
    if (G.postOk) {
        G.locThreshold = GetShaderLocation(G.bright, "threshold");
        G.locDir = GetShaderLocation(G.blur, "direction");
        G.locBloomTex = GetShaderLocation(G.fin, "bloomTex");
        G.locRes = GetShaderLocation(G.fin, "resolution");
        G.locBloom = GetShaderLocation(G.fin, "bloomStrength");
        G.locVig = GetShaderLocation(G.fin, "vignette");
        G.locSat = GetShaderLocation(G.fin, "saturation");
        G.locCon = GetShaderLocation(G.fin, "contrast");
        G.locAberr = GetShaderLocation(G.fin, "aberration");
        G.locFlash = GetShaderLocation(G.fin, "flash");
        G.locTime = GetShaderLocation(G.fin, "time");
    }
    SetQuality(G.quality);
}

void gfx::Shutdown() {
    UnloadShadowTarget();
    UnloadPostTargets();
    if (G.litOk) UnloadShader(G.lit);
    if (G.postOk) { UnloadShader(G.bright); UnloadShader(G.blur); UnloadShader(G.fin); }
}

int gfx::GetQuality() { return G.quality; }
bool gfx::ShadowsOn() { return G.shadowRes > 0 && G.litOk; }

const char* gfx::QualityName() {
    static const char* names[] = { "Thấp", "Trung bình", "Cao" };
    return names[G.quality];
}

void gfx::SetQuality(int q) {
    if (q < Q_LOW) q = Q_LOW;
    if (q > Q_HIGH) q = Q_HIGH;
    G.quality = q;
    int want = (q == Q_HIGH) ? 2048 : (q == Q_MEDIUM ? 1024 : 0);
    if (want != G.shadowRes) {
        UnloadShadowTarget();
        if (want > 0) {
            G.shadow = LoadShadowTarget(want);
            G.shadowRes = G.shadow.id > 0 ? want : 0;
        }
    }
    G.shadowValid = false;
    if (q != Q_HIGH) UnloadPostTargets();
}

void gfx::SetEnvironment(const Environment& env) { G.env = env; }
gfx::Environment& gfx::Env() { return G.env; }

// =============================================================================
//  Khối hình
// =============================================================================
void gfx::Box(Vector3 c, Vector3 s, Color col) { DrawCubeV(c, s, col); }

void gfx::Sphere(Vector3 c, float r, Color col, int rings, int slices) {
    const std::vector<Vector3>& tris = UnitSphere(rings, slices);
    rlCheckRenderBatchLimit((int)tris.size());
    rlBegin(RL_TRIANGLES);
    rlColor4ub(col.r, col.g, col.b, col.a);
    for (const Vector3& n : tris) {
        rlNormal3f(n.x, n.y, n.z);
        rlVertex3f(c.x + n.x * r, c.y + n.y * r, c.z + n.z * r);
    }
    rlEnd();
}

void gfx::Cylinder(Vector3 a, Vector3 b, float ra, float rb, Color col, int slices, bool caps) {
    Vector3 axis = Vector3Subtract(b, a);
    float h = Vector3Length(axis);
    if (h < 1e-5f) return;
    Vector3 an = Vector3Scale(axis, 1.0f / h);
    Vector3 ref = (std::fabs(an.y) < 0.95f) ? Vector3{ 0, 1, 0 } : Vector3{ 1, 0, 0 };
    Vector3 u = Vector3Normalize(Vector3CrossProduct(an, ref));
    Vector3 v = Vector3CrossProduct(an, u);
    float slope = (ra - rb) / h;

    rlCheckRenderBatchLimit(slices * (6 + (caps ? 6 : 0)));
    rlBegin(RL_TRIANGLES);
    rlColor4ub(col.r, col.g, col.b, col.a);
    auto vert = [](Vector3 p, Vector3 n) { rlNormal3f(n.x, n.y, n.z); rlVertex3f(p.x, p.y, p.z); };
    for (int j = 0; j < slices; ++j) {
        float t0 = 2 * PI * j / slices, t1 = 2 * PI * (j + 1) / slices;
        Vector3 d0 = Vector3Add(Vector3Scale(u, std::cos(t0)), Vector3Scale(v, std::sin(t0)));
        Vector3 d1 = Vector3Add(Vector3Scale(u, std::cos(t1)), Vector3Scale(v, std::sin(t1)));
        Vector3 n0 = Vector3Normalize(Vector3Add(d0, Vector3Scale(an, slope)));
        Vector3 n1 = Vector3Normalize(Vector3Add(d1, Vector3Scale(an, slope)));
        Vector3 a0 = Vector3Add(a, Vector3Scale(d0, ra)), a1 = Vector3Add(a, Vector3Scale(d1, ra));
        Vector3 b0 = Vector3Add(b, Vector3Scale(d0, rb)), b1 = Vector3Add(b, Vector3Scale(d1, rb));
        // Thứ tự đỉnh: kiểm tra hướng để luôn quay mặt ra ngoài
        Vector3 fn = Vector3CrossProduct(Vector3Subtract(a1, a0), Vector3Subtract(b0, a0));
        bool flip = Vector3DotProduct(fn, d0) < 0;
        if (!flip) { vert(a0, n0); vert(a1, n1); vert(b0, n0); vert(b0, n0); vert(a1, n1); vert(b1, n1); }
        else       { vert(a0, n0); vert(b0, n0); vert(a1, n1); vert(b0, n0); vert(b1, n1); vert(a1, n1); }
        if (caps) {
            Vector3 down = Vector3Negate(an);
            Vector3 cn = Vector3CrossProduct(Vector3Subtract(a1, a), Vector3Subtract(a0, a));
            if (Vector3DotProduct(cn, down) >= 0) { vert(a, down); vert(a1, down); vert(a0, down); }
            else { vert(a, down); vert(a0, down); vert(a1, down); }
            Vector3 tn = Vector3CrossProduct(Vector3Subtract(b0, b), Vector3Subtract(b1, b));
            if (Vector3DotProduct(tn, an) >= 0) { vert(b, an); vert(b0, an); vert(b1, an); }
            else { vert(b, an); vert(b1, an); vert(b0, an); }
        }
    }
    rlEnd();
}

void gfx::Capsule(Vector3 a, Vector3 b, float r, Color col, int slices) {
    Cylinder(a, b, r, r, col, slices, false);
    Sphere(a, r, col, 6, slices);
    Sphere(b, r, col, 6, slices);
}

// =============================================================================
//  Bóng đổ
// =============================================================================
void gfx::RenderShadowMap(const std::function<void()>& drawCasters) {
    G.shadowValid = false;
    if (G.shadowRes == 0 || !G.litOk) return;
    Vector3 dir = Vector3Normalize(G.env.sunDir);
    Camera3D lc = {};
    lc.target = G.env.shadowCenter;
    lc.position = Vector3Subtract(G.env.shadowCenter, Vector3Scale(dir, 30.0f));
    lc.up = (std::fabs(dir.y) > 0.99f) ? Vector3{ 0, 0, -1 } : Vector3{ 0, 1, 0 };
    lc.fovy = G.env.shadowSize;
    lc.projection = CAMERA_ORTHOGRAPHIC;

    rlSetClipPlanes(1.0, 70.0);            // vùng sâu hẹp -> độ chính xác cao hơn
    BeginTextureMode(G.shadow);
    ClearBackground(WHITE);
    BeginMode3D(lc);
    Matrix view = rlGetMatrixModelview();
    Matrix proj = rlGetMatrixProjection();
    drawCasters();
    EndMode3D();
    EndTextureMode();
    rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    G.lightVP = MatrixMultiply(view, proj);
    G.shadowValid = true;
}

// =============================================================================
//  Shader ánh sáng
// =============================================================================
void gfx::BeginLit(const Camera3D& cam) {
    if (!G.litOk) return;
    Shader& s = G.lit;
    const Environment& e = G.env;
    Vector3 sd = Vector3Normalize(e.sunDir);
    Vector3 sc = ToVec3(e.sunColor, e.sunIntensity);
    Vector3 sky = ToVec3(e.sky, e.ambient), gr = ToVec3(e.ground, e.ambient);
    Vector3 fog = ToVec3(e.fog, 1.0f);
    float fogR[2] = { e.fogNear, e.fogFar };
    SetShaderValue(s, G.L.viewPos, &cam.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(s, G.L.sunDir, &sd, SHADER_UNIFORM_VEC3);
    SetShaderValue(s, G.L.sunColor, &sc, SHADER_UNIFORM_VEC3);
    SetShaderValue(s, G.L.sky, &sky, SHADER_UNIFORM_VEC3);
    SetShaderValue(s, G.L.ground, &gr, SHADER_UNIFORM_VEC3);
    SetShaderValue(s, G.L.fogColor, &fog, SHADER_UNIFORM_VEC3);
    SetShaderValue(s, G.L.fogRange, fogR, SHADER_UNIFORM_VEC2);
    SetShaderValue(s, G.L.exposure, &e.exposure, SHADER_UNIFORM_FLOAT);

    Vector3 lp[8], lcol[8];
    float lr[8];
    int n = e.lightCount > 8 ? 8 : e.lightCount;
    for (int i = 0; i < n; ++i) {
        lp[i] = e.lights[i].pos;
        lcol[i] = ToVec3(e.lights[i].color, e.lights[i].intensity);
        lr[i] = e.lights[i].range;
    }
    SetShaderValue(s, G.L.lightCount, &n, SHADER_UNIFORM_INT);
    if (n > 0) {
        SetShaderValueV(s, G.L.lightPos, lp, SHADER_UNIFORM_VEC3, n);
        SetShaderValueV(s, G.L.lightColor, lcol, SHADER_UNIFORM_VEC3, n);
        SetShaderValueV(s, G.L.lightRange, lr, SHADER_UNIFORM_FLOAT, n);
    }

    int useShadow = G.shadowValid ? 1 : 0;
    SetShaderValue(s, G.L.useShadow, &useShadow, SHADER_UNIFORM_INT);
    if (useShadow) {
        float texel = 1.0f / G.shadowRes;
        SetShaderValueMatrix(s, G.L.lightVP, G.lightVP);
        SetShaderValue(s, G.L.shadowTexel, &texel, SHADER_UNIFORM_FLOAT);
        rlEnableShader(s.id);
        int slot = SHADOW_SLOT;
        rlActiveTextureSlot(SHADOW_SLOT);
        rlEnableTexture(G.shadow.depth.id);
        rlSetUniform(G.L.shadowMap, &slot, SHADER_UNIFORM_INT, 1);
        rlActiveTextureSlot(0);
    }
    BeginShaderMode(s);
}

void gfx::EndLit() {
    if (!G.litOk) return;
    EndShaderMode();
}

void gfx::BeginGlow() {
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();
}

void gfx::EndGlow() {
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}

// =============================================================================
//  Hậu kỳ
// =============================================================================
bool gfx::PostEnabled() { return G.quality == Q_HIGH && G.postOk; }

void gfx::SetAberration(float a) { G.aberration = a; }
void gfx::SetFlash(float f) { G.flash = f; }

void gfx::BeginScene() {
    if (!PostEnabled()) return;
    EnsurePostTargets();
    BeginTextureMode(G.scene);
    ClearBackground(BLACK);          // xóa cả bộ đệm độ sâu (depth buffer) của texture
    G.inScene = true;
}

static void Blit(RenderTexture2D src, RenderTexture2D dst) {
    BeginTextureMode(dst);
    DrawTexturePro(src.texture, { 0, 0, (float)src.texture.width, (float)-src.texture.height },
                   { 0, 0, (float)dst.texture.width, (float)dst.texture.height }, { 0, 0 }, 0, WHITE);
    EndTextureMode();
}

void gfx::EndScene() {
    if (!G.inScene) return;
    EndTextureMode();
    G.inScene = false;

    // Bloom: lọc điểm sáng (1/2 kích thước) -> mờ ngang -> mờ dọc (1/4 kích thước)
    float thr = G.env.bloomThreshold;
    SetShaderValue(G.bright, G.locThreshold, &thr, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(G.bright);
    Blit(G.scene, G.half);
    EndShaderMode();

    BeginShaderMode(G.blur);
    for (int pass = 0; pass < 2; ++pass) {
        Vector2 dh = { 1.0f / G.blurA.texture.width, 0 };
        SetShaderValue(G.blur, G.locDir, &dh, SHADER_UNIFORM_VEC2);
        Blit(pass == 0 ? G.half : G.blurB, G.blurA);
        Vector2 dv = { 0, 1.0f / G.blurA.texture.height };
        SetShaderValue(G.blur, G.locDir, &dv, SHADER_UNIFORM_VEC2);
        Blit(G.blurA, G.blurB);
    }
    EndShaderMode();
}

void gfx::PresentScene() {
    if (!PostEnabled() || G.rtW == 0) return;
    Vector2 res = { (float)G.rtW, (float)G.rtH };
    float t = (float)GetTime();
    SetShaderValue(G.fin, G.locRes, &res, SHADER_UNIFORM_VEC2);
    SetShaderValue(G.fin, G.locBloom, &G.env.bloom, SHADER_UNIFORM_FLOAT);
    SetShaderValue(G.fin, G.locVig, &G.env.vignette, SHADER_UNIFORM_FLOAT);
    SetShaderValue(G.fin, G.locSat, &G.env.saturation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(G.fin, G.locCon, &G.env.contrast, SHADER_UNIFORM_FLOAT);
    SetShaderValue(G.fin, G.locAberr, &G.aberration, SHADER_UNIFORM_FLOAT);
    SetShaderValue(G.fin, G.locFlash, &G.flash, SHADER_UNIFORM_FLOAT);
    SetShaderValue(G.fin, G.locTime, &t, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(G.fin);
    SetShaderValueTexture(G.fin, G.locBloomTex, G.blurB.texture);
    DrawTexturePro(G.scene.texture, { 0, 0, (float)G.rtW, (float)-G.rtH },
                   { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() }, { 0, 0 }, 0, WHITE);
    EndShaderMode();
    G.aberration = 0;
    G.flash = 0;
}
