// =============================================================================
//  CODE VÕ ĐÀI — game học lập trình game C++
//
//  main.cpp: điểm bắt đầu chương trình. Chứa GAME LOOP chính (Bài 2):
//      while (cửa sổ còn mở) { 1. cập nhật (Update)  2. vẽ (Draw) }
//  và chuyển qua lại giữa các màn hình: Title -> Office <-> Lesson / Arena.
// =============================================================================
#include "raylib.h"
#include "Arena.h"
#include "Audio.h"
#include "Common.h"
#include "Gfx.h"
#include "LessonScreen.h"
#include "Lessons.h"
#include "Office.h"
#include "UI.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

static const char* INTRO =
    "Chào bạn! Mình là Minh, một dev indie. Mình đang làm game đối kháng 3D \"Code Võ Đài\" bằng C++ "
    "và bạn sẽ học cùng mình từ con số 0. Đi tới MÁY TÍNH ở bàn làm việc phía sau rồi nhấn E để học Bài 1. "
    "Mỗi bài học xong, game trên VÕ ĐÀI (bục tròn bên phải) sẽ có thêm một tính năng mới!";

static void DrawTitle(float time) {
    float s = ui::S();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    DrawRectangleGradientV(0, 0, (int)W, (int)H, Color{ 10, 12, 20, 120 }, Color{ 10, 12, 20, 230 });

    float y = H * 0.24f;
    ui::TextCentered("// học lập trình game bằng C++", W / 2, y - 40 * s, 20 * s, pal::accent2, FS_MONO);
    // Tiêu đề lớn có bóng nhiều lớp
    std::string title = "CODE VÕ ĐÀI";
    float ts = 96 * s;
    Vector2 m = ui::Measure(title, ts, FS_BOLD);
    for (int i = 6; i >= 1; --i)
        ui::Text(title, (W - m.x) / 2 + i * 1.5f * s, y + i * 1.5f * s, ts, Color{ 120, 60, 0, (unsigned char)(40 + i * 20) }, FS_BOLD);
    ui::Text(title, (W - m.x) / 2, y, ts, pal::accent, FS_BOLD);

    ui::TextCentered("Theo chân anh dev Minh viết một game đối kháng 3D, từ biến int đầu tiên đến AI và camera",
                     W / 2, y + ts + 12 * s, 20 * s, pal::text);

    float a = 0.55f + 0.45f * std::sin(time * 3.0f);
    ui::TextCentered("Nhấn ENTER để bắt đầu", W / 2, H * 0.68f, 26 * s, Fade(pal::text, a), FS_BOLD);
    if (g_progress.completed > 0)
        ui::TextCentered(TextFormat("Đã lưu: %d/%d bài · %d sao · %d trận thắng", g_progress.completed, F_COUNT,
                                    g_progress.TotalStars(), g_progress.wins),
                         W / 2, H * 0.68f + 40 * s, 17 * s, pal::dim);

    ui::Text(TextFormat("%d bài · Cơ bản → Trung cấp → Nâng cao", (int)F_COUNT), 20 * s, H - 34 * s, 15 * s, pal::dim);
    std::string r = "F2: đồ họa    M: âm thanh    F11: toàn màn hình";
    ui::Text(r, W - ui::Measure(r, 15 * s).x - 20 * s, H - 34 * s, 15 * s, pal::dim);
}

int main(int argc, char** argv) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Code Võ Đài - Học lập trình game C++");
    SetWindowMinSize(960, 540);
    SetExitKey(KEY_NULL);              // Esc dùng để thoát màn hình con, không thoát game
    SetTargetFPS(60);
    ChangeDirectory(GetApplicationDirectory());   // để tìm thấy assets/ dù chạy từ đâu

    ui::LoadFonts();
    g_progress.Load();
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--unlock-all") == 0) g_progress.completed = F_COUNT;
        if (std::strcmp(argv[i], "--reset") == 0) g_progress = Progress{};
        if (std::strncmp(argv[i], "--gfx=", 6) == 0) g_progress.gfxQuality = std::atoi(argv[i] + 6);
        if (std::strncmp(argv[i], "--lessons=", 10) == 0) g_progress.completed = std::clamp(std::atoi(argv[i] + 10), 0, (int)F_COUNT);
    }
    gfx::Init();
    gfx::SetQuality(g_progress.gfxQuality);
    sfx::Init();
    arena::Init();
    office::Init();

    Screen screen = Screen::Title;
    float time = 0;
    std::string toast;            // thông báo nhỏ góc màn hình
    float toastTime = 0;

    // ======================= GAME LOOP =======================
    while (!WindowShouldClose()) {
        // Delta time có giới hạn: tránh "nhảy cóc" khi game bị khựng (Bài 8)
        float dt = std::min(GetFrameTime(), 0.05f);
        time += dt;

        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        if (IsKeyPressed(KEY_F2)) {                    // đổi chất lượng đồ họa
            gfx::SetQuality((gfx::GetQuality() + 1) % 3);
            g_progress.gfxQuality = gfx::GetQuality();
            g_progress.Save();
            toast = std::string("Đồ họa: ") + gfx::QualityName();
            toastTime = 2.0f;
        }
        if (IsKeyPressed(KEY_M)) {
            sfx::ToggleMute();
            toast = sfx::Muted() ? "Âm thanh: TẮT" : "Âm thanh: BẬT";
            toastTime = 2.0f;
        }
        toastTime -= dt;

        // ---------------- 1. UPDATE ----------------
        switch (screen) {
        case Screen::Title:
            office::Update(dt, true);
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                screen = Screen::Office;
                sfx::Play(sfx::CLICK, 0);
                if (!g_progress.introSeen) {
                    office::Say(INTRO, 14);
                    g_progress.introSeen = true;
                    g_progress.Save();
                } else if (g_progress.completed < F_COUNT) {
                    office::Say(TextFormat("Chào mừng trở lại! Hôm nay mình học Bài %d: %s. Tới máy tính thôi!",
                                           g_progress.completed + 1, GetLessons()[g_progress.completed].title.c_str()), 6);
                } else {
                    office::Say("Dự án đã hoàn thành! Ra võ đài đánh vài trận, hoặc ôn lại bài ở máy tính.", 6);
                }
            }
            break;

        case Screen::Office: {
            office::Action act = office::Update(dt, false);
            if (IsKeyPressed(KEY_F10)) {        // phím bí mật cho giáo viên: mở khóa tất cả
                g_progress.completed = F_COUNT;
                g_progress.Save();
                office::Say("[F10] Đã mở khóa toàn bộ bài học và tính năng.", 4);
            }
            if (act != office::Action::None) sfx::Play(sfx::CLICK, 0);
            if (act == office::Action::OpenComputer) {
                lessonscreen::Open(std::min(g_progress.completed, (int)F_COUNT - 1));
                screen = Screen::Lesson;
            } else if (act == office::Action::EnterArena) {
                arena::Enter();
                screen = Screen::Arena;
            }
            break;
        }

        case Screen::Lesson:
            if (lessonscreen::Update(dt)) {
                screen = Screen::Office;
                std::string msg = lessonscreen::TakeMessage();
                if (!msg.empty()) office::Say(msg, 9);
            }
            break;

        case Screen::Arena:
            if (arena::Update(dt)) {
                screen = Screen::Office;
                if (g_progress.completed < F_COUNT)
                    office::Say("Chơi thử vui đấy! Nhưng game còn thiếu nhiều thứ. Quay lại máy tính code thêm tính năng nào.", 5);
            }
            break;
        }

        sfx::Update(screen == Screen::Arena && g_progress.Has(F_MUSIC));

        // ---------------- 2. DRAW ----------------
        // a) Các thứ vẽ "ngoài màn hình": bóng đổ, màn hình máy tính, bảng LED
        if (screen == Screen::Arena) arena::RenderOffscreen();
        else office::RenderOffscreen();

        // b) Cảnh 3D. Đồ họa cao: vẽ vào texture rồi xử lý hậu kỳ (bloom, FXAA...)
        auto draw3D = [&] {
            if (screen == Screen::Arena) arena::Draw3D();
            else office::Draw3D();
        };
        bool post = gfx::PostEnabled();
        if (post) {
            gfx::BeginScene();
            draw3D();
            gfx::EndScene();
        }

        BeginDrawing();
        ClearBackground(pal::bg);
        if (post) gfx::PresentScene();
        else draw3D();

        // c) Giao diện 2D vẽ sau cùng, nằm trên cảnh 3D
        switch (screen) {
        case Screen::Title:  DrawTitle(time); break;
        case Screen::Office: office::DrawHUD(false); break;
        case Screen::Lesson: lessonscreen::Draw(); break;
        case Screen::Arena:  arena::DrawHUD(); break;
        }
        if (toastTime > 0) {
            float s = ui::S();
            Vector2 m = ui::Measure(toast, 18 * s, FS_BOLD);
            Rectangle r = { GetScreenWidth() - m.x - 44 * s, 60 * s, m.x + 28 * s, 38 * s };
            ui::Panel(r, Fade(pal::bg, 0.92f), pal::accent, 0.3f);
            ui::Text(toast, r.x + 14 * s, r.y + 9 * s, 18 * s, pal::text, FS_BOLD);
        }
        EndDrawing();
    }

    office::Shutdown();
    arena::Shutdown();
    sfx::Shutdown();
    gfx::Shutdown();
    ui::UnloadFonts();
    CloseWindow();
    return 0;
}
