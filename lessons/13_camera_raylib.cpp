// =============================================================================
//  BÀI 13 — RAYLIB: CAMERA 3D & GAME FEEL
//  File này cần thư viện raylib. Cách dễ nhất: build bằng CMake của dự án
//  (xem README.md), file chạy sẽ là build/lessons/bai_13_camera_raylib
//
//  Điều khiển: A/D đi, J đấm. Nhấn 1/2/3 để bật/tắt: camera động, rung, hit-stop
//  và cảm nhận sự khác biệt của "game feel".
// =============================================================================
#include "raylib.h"
#include "raymath.h"
#include <cmath>

struct Box { Vector3 pos; Color color; float flash; };

int main() {
    InitWindow(1100, 620, "Bai 13 - Camera 3D & game feel");
    SetTargetFPS(60);

    Camera3D cam = {};
    cam.position = { 0, 4, 12 };
    cam.target = { 0, 1.2f, 0 };
    cam.up = { 0, 1, 0 };
    cam.fovy = 45;
    cam.projection = CAMERA_PERSPECTIVE;

    Box p1 = { { -2, 0.9f, 0 }, RED, 0 };
    Box p2 = { { 2, 0.9f, 0 }, BLUE, 0 };
    bool camDong = true, rung = true, dungHinh = true;
    float shake = 0, hitStop = 0, punchTimer = 0;
    Vector3 camPos = cam.position, camTarget = cam.target;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (IsKeyPressed(KEY_ONE)) camDong = !camDong;
        if (IsKeyPressed(KEY_TWO)) rung = !rung;
        if (IsKeyPressed(KEY_THREE)) dungHinh = !dungHinh;

        // Hit-stop: logic dừng, nhưng vẫn vẽ
        float gdt = dt;
        if (hitStop > 0) { hitStop -= dt; gdt = 0; }

        // ---- Update ----
        if (IsKeyDown(KEY_A)) p1.pos.x -= 4 * gdt;
        if (IsKeyDown(KEY_D)) p1.pos.x += 4 * gdt;
        p2.pos.x = 2.5f + sinf((float)GetTime() * 0.7f) * 3.0f;     // đối thủ đi qua lại
        punchTimer -= gdt;
        if (IsKeyPressed(KEY_J) && punchTimer <= 0) {
            punchTimer = 0.3f;
            if (fabsf(p2.pos.x - p1.pos.x) < 1.6f) {                // trúng!
                p2.flash = 1;
                if (dungHinh) hitStop = 0.08f;
                if (rung) shake = 0.35f;
            }
        }
        p2.flash = fmaxf(0, p2.flash - gdt * 4);

        // ---- Camera ----
        if (camDong) {
            Vector3 giua = Vector3Lerp(p1.pos, p2.pos, 0.5f);
            float xa = 8.0f + fabsf(p1.pos.x - p2.pos.x) * 0.6f;
            camPos = Vector3Lerp(camPos, { giua.x, 3.5f, xa }, 5.0f * dt);
            camTarget = Vector3Lerp(camTarget, { giua.x, 1.2f, 0 }, 5.0f * dt);
        } else {
            camPos = { 0, 4, 12 };
            camTarget = { 0, 1.2f, 0 };
        }
        cam.position = camPos;
        cam.target = camTarget;
        if (shake > 0) {
            cam.position.x += GetRandomValue(-100, 100) / 100.0f * shake * 0.5f;
            cam.position.y += GetRandomValue(-100, 100) / 100.0f * shake * 0.5f;
            shake -= dt * 2.0f;
        }

        // ---- Draw ----
        BeginDrawing();
        ClearBackground(Color{ 24, 26, 40, 255 });
        BeginMode3D(cam);
            DrawGrid(30, 1.0f);
            DrawCube(p1.pos, 0.8f, 1.8f, 0.6f, p1.color);
            if (punchTimer > 0.15f)
                DrawCube({ p1.pos.x + 0.8f, 1.4f, 0 }, 0.8f, 0.2f, 0.2f, ORANGE);   // "nắm đấm"
            DrawCube(p2.pos, 0.8f, 1.8f, 0.6f, ColorLerp(p2.color, WHITE, p2.flash));
            DrawCubeWires(p2.pos, 0.8f, 1.8f, 0.6f, BLACK);
        EndMode3D();
        // UI 2D vẽ SAU EndMode3D
        DrawText("A/D: di   J: dam", 20, 20, 20, RAYWHITE);
        DrawText(TextFormat("[1] Camera dong: %s", camDong ? "BAT" : "tat"), 20, 50, 20, camDong ? GREEN : GRAY);
        DrawText(TextFormat("[2] Rung man hinh: %s", rung ? "BAT" : "tat"), 20, 75, 20, rung ? GREEN : GRAY);
        DrawText(TextFormat("[3] Hit-stop: %s", dungHinh ? "BAT" : "tat"), 20, 100, 20, dungHinh ? GREEN : GRAY);
        DrawFPS(GetScreenWidth() - 100, 20);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
