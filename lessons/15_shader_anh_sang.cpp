// =============================================================================
//  BÀI 15 — SHADER & ÁNH SÁNG (GLSL)
//  Cần raylib: build bằng CMake của dự án -> build/lessons/bai_15_shader_anh_sang
//
//  Phím 1-4 đổi kiểu chiếu sáng, phím mũi tên xoay hướng mặt trời.
//  Shader được viết ngay trong file này (chuỗi R"(...)") rồi nạp bằng
//  LoadShaderFromMemory. Thử sửa công thức trong main() của fragment shader!
// =============================================================================
#include "raylib.h"
#include "raymath.h"
#include <initializer_list>

// ---- VERTEX SHADER: chạy cho mỗi đỉnh ----
static const char* VS = R"(
#version 330
in vec3 vertexPosition;
in vec3 vertexNormal;
uniform mat4 mvp;          // raylib tự gửi
uniform mat4 matModel;
uniform mat4 matNormal;
out vec3 fragPos;
out vec3 fragNormal;
void main() {
    fragPos = vec3(matModel * vec4(vertexPosition, 1.0));
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

// ---- FRAGMENT SHADER: chạy cho mỗi điểm ảnh ----
static const char* FS = R"(
#version 330
in vec3 fragPos;
in vec3 fragNormal;
uniform vec4 colDiffuse;   // màu truyền vào DrawModel(...)
uniform vec3 lightDir;     // hướng ánh sáng chiếu tới
uniform vec3 viewPos;      // vị trí camera
uniform int  mode;         // 0..3: kiểu chiếu sáng
out vec4 finalColor;
void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = -normalize(lightDir);
    vec3 V = normalize(viewPos - fragPos);
    vec3 base = colDiffuse.rgb;
    vec3 color;
    if (mode == 0) {                                   // không chiếu sáng: phẳng lì
        color = base;
    } else if (mode == 1) {                            // ambient + Lambert
        float diff = max(dot(N, L), 0.0);
        color = base * (0.2 + 0.8 * diff);
    } else if (mode == 2) {                            // + Blinn-Phong specular
        float diff = max(dot(N, L), 0.0);
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 64.0);
        color = base * (0.2 + 0.8 * diff) + vec3(spec);
    } else {                                           // toon / cel shading kiểu hoạt hình
        float diff = max(dot(N, L), 0.0);
        float step3 = diff > 0.6 ? 1.0 : (diff > 0.25 ? 0.6 : 0.3);
        float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0);
        color = base * step3 + (rim > 0.4 ? vec3(0.25) : vec3(0.0));
    }
    finalColor = vec4(color, 1.0);
}
)";

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1100, 620, "Bai 15 - Shader & anh sang");
    SetTargetFPS(60);

    Shader sh = LoadShaderFromMemory(VS, FS);
    int locLight = GetShaderLocation(sh, "lightDir");
    int locView = GetShaderLocation(sh, "viewPos");
    int locMode = GetShaderLocation(sh, "mode");

    // Mesh do raylib tạo sẵn đã có pháp tuyến (normal) cho từng đỉnh
    Model sphere = LoadModelFromMesh(GenMeshSphere(1.0f, 32, 32));
    Model cube = LoadModelFromMesh(GenMeshCube(1.6f, 1.6f, 1.6f));
    Model torus = LoadModelFromMesh(GenMeshTorus(0.4f, 2.0f, 24, 32));
    Model ground = LoadModelFromMesh(GenMeshPlane(20, 20, 1, 1));
    for (Model* m : { &sphere, &cube, &torus, &ground }) m->materials[0].shader = sh;

    Camera3D cam = {};
    cam.position = { 0, 4, 9 };
    cam.target = { 0, 1, 0 };
    cam.up = { 0, 1, 0 };
    cam.fovy = 45;
    cam.projection = CAMERA_PERSPECTIVE;

    float yaw = 0.8f, pitch = 0.9f;
    int mode = 2;
    const char* tenMode[4] = { "1: Khong chieu sang", "2: Ambient + Lambert", "3: + Blinn-Phong", "4: Toon (hoat hinh)" };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        for (int k = 0; k < 4; k++) if (IsKeyPressed(KEY_ONE + k)) mode = k;
        if (IsKeyDown(KEY_LEFT)) yaw -= dt;
        if (IsKeyDown(KEY_RIGHT)) yaw += dt;
        if (IsKeyDown(KEY_UP)) pitch = fminf(1.5f, pitch + dt);
        if (IsKeyDown(KEY_DOWN)) pitch = fmaxf(0.1f, pitch - dt);

        // Hướng ánh sáng từ 2 góc (toán vector, Bài 9)
        Vector3 light = { -cosf(pitch) * sinf(yaw), -sinf(pitch), -cosf(pitch) * cosf(yaw) };
        SetShaderValue(sh, locLight, &light, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, locView, &cam.position, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, locMode, &mode, SHADER_UNIFORM_INT);

        float t = (float)GetTime();
        BeginDrawing();
        ClearBackground(Color{ 24, 26, 40, 255 });
        BeginMode3D(cam);
            DrawModel(ground, { 0, 0, 0 }, 1.0f, Color{ 90, 96, 120, 255 });
            DrawModel(sphere, { -3, 1.2f, 0 }, 1.0f, Color{ 230, 80, 70, 255 });
            DrawModelEx(cube, { 0, 1.2f, 0 }, { 0, 1, 0 }, t * 30, { 1, 1, 1 }, Color{ 80, 170, 240, 255 });
            DrawModelEx(torus, { 3, 1.4f, 0 }, { 1, 0, 0 }, 60 + t * 40, { 1, 1, 1 }, Color{ 255, 190, 60, 255 });
            DrawLine3D({ 0, 1, 0 }, Vector3Subtract({ 0, 1, 0 }, Vector3Scale(light, 3)), YELLOW);   // tia nắng
        EndMode3D();
        DrawText(tenMode[mode], 20, 20, 24, RAYWHITE);
        DrawText("Phim 1-4: doi kieu chieu sang   Mui ten: xoay mat troi", 20, 52, 18, GRAY);
        DrawFPS(GetScreenWidth() - 100, 20);
        EndDrawing();
    }

    UnloadModel(sphere); UnloadModel(cube); UnloadModel(torus); UnloadModel(ground);
    UnloadShader(sh);
    CloseWindow();
    // BÀI TẬP: thêm mode 5 — tô màu theo pháp tuyến: color = N * 0.5 + 0.5;
    return 0;
}
