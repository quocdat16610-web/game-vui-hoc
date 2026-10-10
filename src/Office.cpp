#include "Office.h"
#include "Arena.h"
#include "Common.h"
#include "Gfx.h"
#include "Lessons.h"
#include "PlayerCode.h"
#include "UI.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <vector>

namespace {
    const float ROOM_X = 8.0f, ROOM_Z = 6.0f;
    const Vector3 DESK_POS   = { -3.0f, 0, -4.9f };
    const Vector3 ARENA_POS  = {  3.8f, 0, -1.2f };
    const Vector3 SHELF_POS  = { -7.55f, 0, 1.6f };
    const Vector3 COFFEE_POS = {  6.9f, 0, -5.2f };

    enum class Zone { None, Computer, Arena, Shelf, Coffee };

    struct OfficeState {
        Fighter dev;                 // dùng lại model võ sĩ, nhưng ở tư thế đi lại
        float yaw = -90;             // độ
        Camera3D cam = {};
        Vector3 camPos = { 0, 8, 10 };
        RenderTexture2D screen = {};
        bool screenOk = false;
        float time = 0;
        Zone zone = Zone::None;
        std::string say;
        float sayTime = 0;
        bool roadmap = false;
        int tipIndex = 0;
        std::vector<BoundingBox> solids;   // vật cản
    };
    OfficeState O;

    const char* TIPS[] = {
        "Mẹo C++: dùng const& khi truyền std::string hay std::vector vào hàm để khỏi sao chép.",
        "Mẹo C++: bật cảnh báo trình biên dịch (-Wall -Wextra). Cảnh báo hôm nay là bug ngày mai.",
        "Mẹo C++: luôn khởi tạo biến. int mau; chứa giá trị rác, int mau = 0; thì không.",
        "Mẹo game: nhân mọi chuyển động với delta time, tốc độ sẽ không phụ thuộc FPS.",
        "Mẹo C++: ưu tiên std::unique_ptr hơn new/delete. Không còn lo quên delete.",
        "Mẹo game: tách Input khỏi logic. AI và người chơi dùng chung một class Fighter.",
        "Mẹo C++: enum class an toàn hơn enum thường vì không tự đổi sang int.",
        "Mẹo debug: khi bug khó, in giá trị biến ra (printf, TraceLog) hoặc dùng breakpoint trong IDE.",
        "Mẹo game: vẽ hitbox ra màn hình khi debug. Trong võ đài, nhấn H (sau Bài 11).",
        "Mẹo C++: dùng auto& trong vòng for khi muốn sửa phần tử: for (auto& p : particles).",
        "Mẹo hiệu năng: đừng cấp phát bộ nhớ (new, push_back liên tục) trong game loop nếu tránh được.",
        "Mẹo học: gõ lại code mẫu bằng tay thay vì copy. Ngón tay cũng cần học!",
        "Mẹo C++: chia code thành .h (khai báo) và .cpp (định nghĩa) như thư mục src/ của game này.",
        "Mẹo game: \"game feel\" đến từ chi tiết nhỏ: rung màn hình, hit-stop, hạt, âm thanh.",
    };
}

static bool Has(Feature f) { return g_progress.Has(f); }

static Color Darken(Color c, float k = 0.6f) {
    return Color{ (unsigned char)(c.r * k), (unsigned char)(c.g * k), (unsigned char)(c.b * k), c.a };
}
static void Part(Vector3 c, Vector3 s, Color col) { gfx::Box(c, s, col); }
// Trụ đứng: đáy tại base, cao h
static void Cyl(Vector3 base, float rTop, float rBot, float h, int slices, Color col) {
    gfx::Cylinder(base, { base.x, base.y + h, base.z }, rBot, rTop, col, slices);
}
static void Sph(Vector3 c, float r, int rings, int slices, Color col) { gfx::Sphere(c, r, col, rings, slices); }
static void AddSolid(Vector3 c, Vector3 s) {
    O.solids.push_back({ { c.x - s.x / 2, 0, c.z - s.z / 2 }, { c.x + s.x / 2, 3, c.z + s.z / 2 } });
}

void office::Init() {
    O.dev = Fighter{};
    O.dev.type = CharType::Dev;
    O.dev.casual = true;
    O.dev.pos = { -1.0f, 0, 1.5f };
    O.yaw = -90;
    O.screen = LoadRenderTexture(640, 400);
    O.screenOk = O.screen.id != 0;
    SetTextureFilter(O.screen.texture, TEXTURE_FILTER_BILINEAR);

    O.solids.clear();
    AddSolid({ DESK_POS.x, 0, DESK_POS.z }, { 3.2f, 1, 1.3f });       // bàn làm việc
    AddSolid({ SHELF_POS.x, 0, SHELF_POS.z }, { 0.8f, 1, 2.6f });     // kệ sách
    AddSolid({ COFFEE_POS.x, 0, COFFEE_POS.z }, { 1.6f, 1, 1.0f });   // quầy cà phê
    AddSolid({ ARENA_POS.x, 0, ARENA_POS.z }, { 3.4f, 1, 3.4f });      // bục võ đài
    AddSolid({ -6.8f, 0, 4.8f }, { 0.8f, 1, 0.8f });                   // chậu cây
    AddSolid({ 7.0f, 0, 4.6f }, { 0.8f, 1, 0.8f });
}

void office::Shutdown() {
    if (O.screenOk) UnloadRenderTexture(O.screen);
}

void office::Say(const std::string& msg, float seconds) {
    O.say = msg;
    O.sayTime = seconds;
}

bool office::ModalOpen() { return O.roadmap; }

// -----------------------------------------------------------------------------
// Màn hình máy tính: code của bài hiện tại tự "gõ" ra từng ký tự
// -----------------------------------------------------------------------------
static void RenderScreens() {
    if (!O.screenOk) return;
    const auto& lessons = GetLessons();
    int idx = std::min(g_progress.completed, (int)lessons.size() - 1);
    // Màn hình hiện code của CHÍNH BẠN (nếu đã viết), nếu chưa thì code mẫu của bài
    static std::string cached;
    static int cachedIdx = -1;
    static float reload = 0;
    reload -= GetFrameTime();
    int codeIdx = std::max(0, std::min(g_progress.completed - 1, (int)lessons.size() - 1));
    if (codeIdx != cachedIdx || reload <= 0) {
        cachedIdx = codeIdx;
        reload = 2.0f;
        cached = (g_progress.completed > 0 && pcode::HasSaved(codeIdx)) ? pcode::LoadSource(codeIdx) : lessons[idx].pages[0].code;
    }
    const std::string& code = cached;

    BeginTextureMode(O.screen);
    ClearBackground(Color{ 14, 16, 24, 255 });
    DrawRectangle(0, 0, 640, 30, Color{ 32, 36, 50, 255 });
    ui::Text((g_progress.completed > 0 && pcode::HasSaved(codeIdx)) ? TextFormat("code/bai_%02d (code của bạn)", codeIdx + 1) : lessons[idx].file.c_str(),
             12, 6, 18, pal::dim, FS_MONO);
    DrawCircle(600, 15, 6, pal::bad);
    DrawCircle(580, 15, 6, pal::accent);
    DrawCircle(560, 15, 6, pal::good);

    // Hiệu ứng gõ phím: hiện dần số ký tự theo thời gian
    size_t shown = (size_t)std::fmod(O.time * 28.0f, (float)code.size() + 120.0f);
    if (shown > code.size()) shown = code.size();
    std::string part = code.substr(0, shown);
    // Cắt lại cho hợp lệ UTF-8 (không cắt giữa một ký tự nhiều byte)
    while (!part.empty() && ((unsigned char)part.back() & 0xC0) == 0x80) part.pop_back();
    if (!part.empty() && ((unsigned char)part.back() & 0x80)) part.pop_back();
    float h = ui::Code(part, { 0, 0, 640, 0 }, 17, false);
    float y = 36;
    if (h > 400 - 40) y = 400 - h - 4;   // cuộn lên khi tràn
    ui::Code(part, { 0, y, 640, 0 }, 17, true);
    EndTextureMode();
}

// -----------------------------------------------------------------------------
static bool BlockedAt(Vector3 p) {
    const float R = 0.35f;
    for (const auto& b : O.solids)
        if (p.x + R > b.min.x && p.x - R < b.max.x && p.z + R > b.min.z && p.z - R < b.max.z) return true;
    return false;
}

static float DistXZ(Vector3 a, Vector3 b) { return std::hypot(a.x - b.x, a.z - b.z); }

office::Action office::Update(float dt, bool titleMode) {
    O.time += dt;
    O.sayTime -= dt;

    if (titleMode) {
        float a = O.time * 0.15f;
        O.cam.position = { std::sin(a) * 11.0f, 6.5f, std::cos(a) * 11.0f };
        O.cam.target = { 0, 1.0f, -1.0f };
        O.cam.up = { 0, 1, 0 };
        O.cam.fovy = 50;
        O.cam.projection = CAMERA_PERSPECTIVE;
        O.dev.ChangeState(FState::Idle);
        return Action::None;
    }

    if (O.roadmap) {
        if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) ||
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            O.roadmap = false;
        return Action::None;
    }

    // Di chuyển: WASD theo hướng camera
    Vector3 dir = { 0, 0, 0 };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    dir.z -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  dir.z += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  dir.x -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dir.x += 1;

    Fighter& d = O.dev;
    d.stateTime += dt;
    if (Vector3Length(dir) > 0) {
        dir = Vector3Normalize(dir);
        const float SPEED = 4.2f;
        Vector3 next = d.pos;
        next.x = Clamp(d.pos.x + dir.x * SPEED * dt, -ROOM_X + 0.5f, ROOM_X - 0.5f);
        if (!BlockedAt(next)) d.pos.x = next.x;
        next = d.pos;
        next.z = Clamp(d.pos.z + dir.z * SPEED * dt, -ROOM_Z + 0.5f, ROOM_Z - 0.5f);
        if (!BlockedAt(next)) d.pos.z = next.z;

        // Quay mặt mượt theo hướng đi (toán góc, Bài 9)
        float want = std::atan2(-dir.z, dir.x) * RAD2DEG;
        float diff = std::fmod(want - O.yaw + 540.0f, 360.0f) - 180.0f;
        O.yaw += diff * std::min(1.0f, 12.0f * dt);
        if (d.state != FState::Walk) d.ChangeState(FState::Walk);
        d.walkPhase += dt * 9.0f;
    } else if (d.state != FState::Idle) {
        d.ChangeState(FState::Idle);
    }

    // Camera bám theo nhân vật (Lerp)
    Vector3 want = { d.pos.x * 0.6f, 8.2f, d.pos.z + 9.5f };
    O.camPos = Vector3Lerp(O.camPos, want, std::min(1.0f, 3.0f * dt));
    O.cam.position = O.camPos;
    O.cam.target = { O.camPos.x, 0.8f, O.camPos.z - 9.0f };
    O.cam.up = { 0, 1, 0 };
    O.cam.fovy = 45;
    O.cam.projection = CAMERA_PERSPECTIVE;

    // Vùng tương tác
    O.zone = Zone::None;
    if (DistXZ(d.pos, { DESK_POS.x, 0, DESK_POS.z + 1.3f }) < 1.5f) O.zone = Zone::Computer;
    else if (DistXZ(d.pos, ARENA_POS) < 2.9f) O.zone = Zone::Arena;
    else if (DistXZ(d.pos, SHELF_POS) < 1.9f) O.zone = Zone::Shelf;
    else if (DistXZ(d.pos, COFFEE_POS) < 1.8f) O.zone = Zone::Coffee;

    if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER)) {
        switch (O.zone) {
        case Zone::Computer: return Action::OpenComputer;
        case Zone::Arena:
            if (Has(F_ARENA)) return Action::EnterArena;
            Say(g_progress.completed == 0
                    ? "Võ đài chưa có gì để chạy cả. Phải học Bài 1 và Bài 2 (game loop) trước đã!"
                    : "Chưa có game loop thì chưa chạy được võ đài. Học xong Bài 2 nhé!");
            break;
        case Zone::Shelf: O.roadmap = true; break;
        case Zone::Coffee: {
            int n = sizeof(TIPS) / sizeof(TIPS[0]);
            Say(std::string("*nhấp một ngụm cà phê* ") + TIPS[O.tipIndex % n], 7);
            O.tipIndex++;
            break;
        }
        default: break;
        }
    }
    return Action::None;
}

// -----------------------------------------------------------------------------
// Vẽ căn phòng
// -----------------------------------------------------------------------------

static void DrawRoom() {
    // Sàn gỗ
    for (int x = -8; x < 8; ++x) {
        Color c = (x & 1) ? Color{ 150, 110, 78, 255 } : Color{ 140, 102, 72, 255 };
        DrawCube({ x + 0.5f, -0.05f, 0 }, 1.0f, 0.1f, ROOM_Z * 2, c);
    }
    // Thảm
    DrawCube({ -2.4f, 0.01f, 1.2f }, 5.0f, 0.02f, 3.2f, Color{ 62, 84, 120, 255 });
    DrawCube({ -2.4f, 0.015f, 1.2f }, 4.6f, 0.02f, 2.8f, Color{ 74, 100, 142, 255 });
    // Tường sau & tường trái
    Part({ 0, 2.0f, -ROOM_Z - 0.1f }, { ROOM_X * 2, 4.0f, 0.2f }, Color{ 214, 206, 190, 255 });
    Part({ -ROOM_X - 0.1f, 2.0f, 0 }, { 0.2f, 4.0f, ROOM_Z * 2 }, Color{ 200, 192, 176, 255 });
    DrawCube({ 0, 0.12f, -ROOM_Z + 0.02f }, ROOM_X * 2, 0.24f, 0.05f, Color{ 120, 90, 64, 255 });
    DrawCube({ -ROOM_X + 0.02f, 0.12f, 0 }, 0.05f, 0.24f, ROOM_Z * 2, Color{ 120, 90, 64, 255 });

    // Cửa sổ nhìn ra thành phố
    Part({ 3.2f, 2.4f, -ROOM_Z + 0.02f }, { 3.4f, 1.9f, 0.06f }, Color{ 90, 70, 52, 255 });
    DrawCube({ 3.2f, 2.4f, -ROOM_Z + 0.09f }, 0.06f, 1.6f, 0.02f, Color{ 90, 70, 52, 255 });

    // Bảng trắng trên tường trái: kế hoạch dự án
    Part({ -ROOM_X + 0.05f, 2.2f, -2.4f }, { 0.06f, 1.5f, 2.6f }, Color{ 236, 238, 240, 255 });
    const Color marks[4] = { pal::bad, pal::accent2, pal::good, pal::purple };
    for (int i = 0; i < 6; ++i) {
        float w = 0.6f + ((i * 53) % 9) * 0.15f;
        DrawCube({ -ROOM_X + 0.09f, 2.75f - i * 0.2f, -3.3f + w / 2 }, 0.01f, 0.05f, w, marks[i % 4]);
    }
    // Đánh dấu tiến độ trên bảng
    for (int i = 0; i < F_COUNT; ++i) {
        Color c = (i < g_progress.completed) ? pal::good : Color{ 190, 190, 195, 255 };
        DrawCube({ -ROOM_X + 0.09f, 1.65f, -3.5f + i * 0.17f }, 0.01f, 0.12f, 0.12f, c);
    }

    // Bàn làm việc
    Vector3 d = DESK_POS;
    Part({ d.x, 0.92f, d.z }, { 3.0f, 0.08f, 1.2f }, Color{ 96, 70, 50, 255 });
    for (int sx = -1; sx <= 1; sx += 2)
        Part({ d.x + sx * 1.4f, 0.45f, d.z }, { 0.1f, 0.9f, 1.1f }, Color{ 60, 60, 66, 255 });
    // Màn hình chính
    Part({ d.x, 1.02f, d.z - 0.25f }, { 0.5f, 0.04f, 0.3f }, Color{ 40, 40, 46, 255 });
    Part({ d.x, 1.2f, d.z - 0.3f }, { 0.08f, 0.35f, 0.08f }, Color{ 40, 40, 46, 255 });
    Part({ d.x, 1.75f, d.z - 0.34f }, { 1.72f, 1.06f, 0.06f }, Color{ 28, 28, 32, 255 });
    // Màn hình phụ dọc
    Part({ d.x + 1.25f, 1.6f, d.z - 0.2f }, { 0.5f, 0.85f, 0.05f }, Color{ 28, 28, 32, 255 });
    // Đèn bàn
    Cyl({ d.x + 0.95f, 0.96f, d.z + 0.1f }, 0.12f, 0.14f, 0.04f, 12, Color{ 40, 40, 46, 255 });
    gfx::Cylinder({ d.x + 0.95f, 1.0f, d.z + 0.1f }, { d.x + 0.85f, 1.55f, d.z - 0.05f }, 0.02f, 0.02f, Color{ 40, 40, 46, 255 }, 6);
    gfx::Cylinder({ d.x + 0.85f, 1.55f, d.z - 0.05f }, { d.x + 0.62f, 1.45f, d.z + 0.1f }, 0.05f, 0.13f, Color{ 230, 120, 50, 255 }, 12);
    // Bàn phím, chuột, cốc cà phê, giấy note
    Part({ d.x, 0.98f, d.z + 0.25f }, { 0.9f, 0.04f, 0.3f }, Color{ 50, 52, 60, 255 });
    Part({ d.x + 0.7f, 0.98f, d.z + 0.3f }, { 0.1f, 0.04f, 0.16f }, Color{ 50, 52, 60, 255 });
    Cyl({ d.x - 1.1f, 0.96f, d.z + 0.2f }, 0.09f, 0.08f, 0.2f, 10, Color{ 230, 230, 235, 255 });
    Cyl({ d.x - 1.1f, 1.15f, d.z + 0.2f }, 0.075f, 0.075f, 0.01f, 10, Color{ 80, 50, 30, 255 });
    DrawCube({ d.x - 0.75f, 2.1f, d.z - 0.3f }, 0.16f, 0.16f, 0.01f, Color{ 255, 230, 110, 255 });
    DrawCube({ d.x + 0.8f, 2.12f, d.z - 0.3f }, 0.16f, 0.16f, 0.01f, Color{ 255, 150, 190, 255 });
    // Ghế
    Vector3 ch = { d.x, 0, d.z + 1.1f };
    Part({ ch.x, 0.55f, ch.z }, { 0.6f, 0.1f, 0.6f }, Color{ 40, 44, 56, 255 });
    Part({ ch.x, 1.0f, ch.z + 0.28f }, { 0.6f, 0.8f, 0.08f }, Color{ 40, 44, 56, 255 });
    Cyl({ ch.x, 0.1f, ch.z }, 0.04f, 0.04f, 0.45f, 6, DARKGRAY);
    Cyl({ ch.x, 0.02f, ch.z }, 0.3f, 0.3f, 0.06f, 8, Color{ 30, 30, 34, 255 });

    // Kệ sách: khung mở phía trước để thấy sách
    Vector3 s = SHELF_POS;
    Color wood = { 110, 78, 54, 255 };
    Part({ s.x - 0.25f, 1.2f, s.z }, { 0.1f, 2.4f, 2.4f }, Darken(wood, 0.8f));     // tấm lưng
    Part({ s.x, 1.2f, s.z - 1.2f }, { 0.6f, 2.4f, 0.08f }, wood);                   // hai vách
    Part({ s.x, 1.2f, s.z + 1.2f }, { 0.6f, 2.4f, 0.08f }, wood);
    for (int row = 0; row < 5; ++row)
        Part({ s.x, 0.05f + row * 0.58f, s.z }, { 0.6f, 0.06f, 2.4f }, wood);       // các tầng
    for (int row = 0; row < 4; ++row) {
        for (int b = 0; b < 9; ++b) {
            float h = 0.34f + ((b * 7 + row * 3) % 5) * 0.03f;
            Color bc = Color{ (unsigned char)(80 + (b * 47 + row * 31) % 170), (unsigned char)(60 + (b * 83 + row * 17) % 150),
                              (unsigned char)(70 + (b * 29 + row * 61) % 160), 255 };
            Part({ s.x + 0.02f, 0.08f + row * 0.58f + h / 2, s.z - 1.0f + b * 0.23f }, { 0.4f, h, 0.17f }, bc);
        }
    }

    // Quầy cà phê
    Vector3 cf = COFFEE_POS;
    Part({ cf.x, 0.45f, cf.z }, { 1.6f, 0.9f, 0.9f }, Color{ 230, 230, 234, 255 });
    Part({ cf.x - 0.2f, 1.2f, cf.z - 0.1f }, { 0.5f, 0.6f, 0.45f }, Color{ 50, 50, 56, 255 });
    DrawCube({ cf.x - 0.2f, 1.3f, cf.z + 0.13f }, 0.2f, 0.1f, 0.01f, pal::bad);
    Cyl({ cf.x + 0.45f, 0.9f, cf.z }, 0.07f, 0.06f, 0.16f, 8, Color{ 200, 80, 60, 255 });

    // Cây xanh
    for (Vector3 p : { Vector3{ -6.8f, 0, 4.8f }, Vector3{ 7.0f, 0, 4.6f } }) {
        Cyl(p, 0.32f, 0.26f, 0.55f, 10, Color{ 170, 100, 70, 255 });
        Sph({ p.x, 0.95f, p.z }, 0.45f, 8, 8, Color{ 70, 150, 80, 255 });
        Sph({ p.x + 0.2f, 1.3f, p.z }, 0.3f, 8, 8, Color{ 80, 170, 90, 255 });
    }
}

static void DrawArenaPad() {
    Vector3 a = ARENA_POS;
    Cyl(a, 2.0f, 2.1f, 0.2f, 24, Color{ 40, 44, 60, 255 });
    Cyl({ a.x, 0.2f, a.z }, 1.85f, 1.85f, 0.01f, 24, Color{ 52, 58, 80, 255 });

    if (!Has(F_ARENA)) {
        // Đang thi công: cọc chắn màu cam
        for (int i = 0; i < 4; ++i) {
            float ang = i * PI / 2 + PI / 4;
            Vector3 p = { a.x + std::cos(ang) * 1.3f, 0.2f, a.z + std::sin(ang) * 1.3f };
            Cyl(p, 0.03f, 0.18f, 0.45f, 8, Color{ 255, 130, 30, 255 });
            Cyl({ p.x, p.y + 0.18f, p.z }, 0.11f, 0.12f, 0.06f, 8, WHITE);
        }
        Part({ a.x, 0.55f, a.z + 1.3f }, { 2.0f, 0.08f, 0.04f }, Color{ 255, 210, 40, 255 });
        return;
    }

    // Hai võ sĩ mini đang tập — hiển thị đúng những gì bạn đã code!
    Fighter f1, f2;
    f1.type = Has(F_CHARSELECT) ? CharType::Ninja : CharType::Dev;
    f2.type = Has(F_AI) ? (Has(F_CHARSELECT) ? CharType::Robot : CharType::Dev) : CharType::Dummy;
    f1.pos = { -0.75f, 0, 0 }; f1.facing = 1;
    f2.pos = { 0.75f, 0, 0 };  f2.facing = -1;
    float cyc = std::fmod(O.time, 2.4f);
    if (Has(F_PUNCH) && cyc < 0.35f) { f1.state = FState::Punch; f1.stateTime = cyc; }
    if (Has(F_KICK_BLOCK) && cyc >= 0.8f && cyc < 1.35f) { f1.state = FState::Kick; f1.stateTime = cyc - 0.8f; }
    if (Has(F_PUNCH) && (cyc < 0.15f || (cyc >= 0.98f && cyc < 1.1f))) f2.flash = 0.8f;
    if (Has(F_AI) && cyc >= 1.6f && cyc < 1.95f) { f2.state = FState::Punch; f2.stateTime = cyc - 1.6f; }
    if (Has(F_KICK_BLOCK) && Has(F_AI) && cyc >= 1.6f && cyc < 2.0f) f1.state = FState::Block;

    rlPushMatrix();
    rlTranslatef(a.x, 0.21f, a.z);
    rlScalef(0.7f, 0.7f, 0.7f);
    bool anim = Has(F_ANIM_STATES);
    DrawFighterModel(f1, anim, O.time);
    DrawFighterModel(f2, anim, O.time + 1.0f);
    rlPopMatrix();
}

static void DrawPrompt(Vector3 world, const std::string& text) {
    float s = ui::S();
    Vector2 p = GetWorldToScreen(world, O.cam);
    Vector2 m = ui::Measure(text, 18 * s, FS_BOLD);
    float bob = std::sin(O.time * 5) * 3 * s;
    Rectangle r = { p.x - m.x / 2 - 34 * s, p.y - 20 * s + bob, m.x + 46 * s, 36 * s };
    DrawRectangleRounded(r, 0.4f, 6, Color{ 15, 17, 26, 225 });
    DrawRectangleRoundedLinesEx(r, 0.4f, 6, 1.5f, pal::accent);
    Rectangle key = { r.x + 6 * s, r.y + 6 * s, 24 * s, 24 * s };
    DrawRectangleRounded(key, 0.3f, 4, pal::accent);
    ui::Text("E", key.x + 6.5f * s, key.y + 1.5f * s, 19 * s, pal::bg, FS_BOLD);
    ui::Text(text, r.x + 38 * s, r.y + 8 * s, 18 * s, pal::text, FS_BOLD);
}

static void DrawRoadmap() {
    float s = ui::S();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    DrawRectangle(0, 0, (int)W, (int)H, Color{ 0, 0, 0, 160 });
    float pw = std::min(W - 40 * s, 820 * s), ph = std::min(H - 40 * s, 640 * s);
    Rectangle r = { (W - pw) / 2, (H - ph) / 2, pw, ph };
    ui::Panel(r, pal::panel, pal::accent, 0.03f);
    ui::Text("LỘ TRÌNH DỰ ÁN \"CODE VÕ ĐÀI\"", r.x + 24 * s, r.y + 18 * s, 26 * s, pal::accent, FS_BOLD);
    ui::Text("Mỗi bài học thêm một tính năng vào game đối kháng", r.x + 24 * s, r.y + 52 * s, 16 * s, pal::dim);
    const auto& L = GetLessons();
    float y = r.y + 86 * s;
    float rowH = (ph - 130 * s) / L.size();
    for (size_t i = 0; i < L.size(); ++i) {
        bool done = (int)i < g_progress.completed;
        bool next = (int)i == g_progress.completed;
        Color c = done ? pal::good : (next ? pal::accent : pal::dim);
        DrawCircle((int)(r.x + 36 * s), (int)(y + rowH * 0.45f), 8 * s, done ? pal::good : pal::panel2);
        DrawCircleLines((int)(r.x + 36 * s), (int)(y + rowH * 0.45f), 8 * s, c);
        ui::Text(TextFormat("Bài %d", (int)i + 1), r.x + 56 * s, y + rowH * 0.45f - 9 * s, 16 * s, c, FS_BOLD);
        ui::Text(L[i].title, r.x + 120 * s, y + rowH * 0.45f - 9 * s, 16 * s, done ? pal::text : pal::dim);
        std::string u = std::string("→ ") + L[i].unlock;
        ui::Text(u, r.x + pw * 0.52f, y + rowH * 0.45f - 9 * s, 15 * s, c);
        y += rowH;
    }
    ui::TextCentered("Nhấn E hoặc Esc để đóng", W / 2, r.y + ph - 32 * s, 15 * s, pal::dim);
}

// -----------------------------------------------------------------------------
// Ánh sáng của văn phòng (Bài 15)
// -----------------------------------------------------------------------------
static void SetupEnvironment() {
    gfx::Environment e;
    e.sunDir = { 0.45f, -1.0f, -0.35f };
    e.sunColor = { 255, 236, 205, 255 };
    e.sunIntensity = 0.95f;
    e.sky = { 172, 186, 218, 255 };
    e.ground = { 124, 92, 70, 255 };
    e.ambient = 0.55f;
    e.fog = { 24, 26, 38, 255 };
    e.fogNear = 24; e.fogFar = 48;
    Vector3 d = DESK_POS;
    float flicker = 1.0f + std::sin(O.time * 7.0f) * 0.03f;
    e.lights[0] = { { d.x + 0.62f, 1.35f, d.z + 0.25f }, Color{ 255, 176, 96, 255 }, 1.7f * flicker, 4.5f };   // đèn bàn
    e.lights[1] = { { d.x, 1.75f, d.z + 0.25f }, Color{ 90, 160, 255, 255 }, 0.9f, 3.5f };                   // màn hình
    e.lights[2] = { { 3.2f, 2.4f, -ROOM_Z + 0.6f }, Color{ 150, 190, 255, 255 }, 1.0f, 6.5f };              // cửa sổ
    e.lightCount = 3;
    if (Has(F_ARENA)) e.lights[e.lightCount++] = { { ARENA_POS.x, 0.8f, ARENA_POS.z }, pal::accent, 1.1f, 3.8f };
    e.shadowCenter = { 0, 0, -0.5f };
    e.shadowSize = 22;
    e.bloom = 0.4f; e.bloomThreshold = 0.82f; e.vignette = 0.42f;
    gfx::SetEnvironment(e);
}

static void DrawDev() {
    rlPushMatrix();
    rlTranslatef(O.dev.pos.x, 0, O.dev.pos.z);
    rlRotatef(O.yaw, 0, 1, 0);
    Fighter local = O.dev;
    local.pos = { 0, 0, 0 };
    local.facing = 1;
    DrawFighterModel(local, true, O.time);
    rlPopMatrix();
}

// Quad có texture, mặt quay về +z
static void Quad(Texture2D tex, Vector3 c, float w, float h, Color tint = WHITE) {
    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);
    rlNormal3f(0, 0, 1);
    rlTexCoord2f(0, 0); rlVertex3f(c.x - w / 2, c.y - h / 2, c.z);
    rlTexCoord2f(1, 0); rlVertex3f(c.x + w / 2, c.y - h / 2, c.z);
    rlTexCoord2f(1, 1); rlVertex3f(c.x + w / 2, c.y + h / 2, c.z);
    rlTexCoord2f(0, 1); rlVertex3f(c.x - w / 2, c.y + h / 2, c.z);
    rlEnd();
    rlSetTexture(0);
}

// Vật phát sáng: màn hình, cửa sổ, bóng đèn, tia nắng, bụi bay
static void DrawGlow() {
    Vector3 d = DESK_POS;
    if (O.screenOk) Quad(O.screen.texture, { d.x, 1.75f, d.z - 0.305f }, 1.6f, 1.0f);
    // Màn hình phụ
    DrawCube({ d.x + 1.25f, 1.6f, d.z - 0.17f }, 0.44f, 0.78f, 0.01f, Color{ 16, 44, 40, 255 });
    for (int i = 0; i < 8; ++i)
        DrawCube({ d.x + 1.18f + (i % 3) * 0.03f, 1.9f - i * 0.08f, d.z - 0.16f }, 0.2f + (i % 4) * 0.04f, 0.025f, 0.01f, pal::good);
    // Cửa sổ: bầu trời + nhà cao tầng
    rlBegin(RL_QUADS);
    float wz = -ROOM_Z + 0.06f;
    rlColor4ub(150, 200, 250, 255); rlVertex3f(1.65f, 1.6f, wz); rlVertex3f(4.75f, 1.6f, wz);
    rlColor4ub(80, 140, 220, 255);  rlVertex3f(4.75f, 3.2f, wz); rlVertex3f(1.65f, 3.2f, wz);
    rlEnd();
    for (int i = 0; i < 7; ++i) {
        float h = 0.3f + ((i * 37) % 7) * 0.12f;
        DrawCube({ 1.9f + i * 0.43f, 1.6f + h / 2, wz + 0.02f }, 0.36f, h, 0.01f, Color{ 70, 96, 140, 255 });
        for (int k = 0; k < 3; ++k)
            if ((i + k) % 2 == 0) DrawCube({ 1.84f + i * 0.43f + (k % 2) * 0.1f, 1.7f + k * 0.12f, wz + 0.03f }, 0.05f, 0.05f, 0.005f, Color{ 255, 230, 150, 255 });
    }
    DrawCube({ 3.2f, 2.4f, wz + 0.04f }, 0.06f, 1.6f, 0.02f, Color{ 90, 70, 52, 255 });
    DrawCube({ 3.2f, 2.4f, wz + 0.04f }, 3.1f, 0.05f, 0.02f, Color{ 90, 70, 52, 255 });

    gfx::BeginGlow();
    DrawSphere({ d.x + 0.66f, 1.43f, d.z + 0.15f }, 0.06f, Color{ 255, 220, 160, 255 });    // bóng đèn bàn
    // Tia nắng chiếu xiên qua cửa sổ
    rlBegin(RL_QUADS);
    rlColor4ub(255, 240, 200, 28); rlVertex3f(1.7f, 3.15f, wz + 0.05f);
    rlColor4ub(255, 240, 200, 0);  rlVertex3f(2.4f, 0.02f, -2.2f);
    rlColor4ub(255, 240, 200, 0);  rlVertex3f(5.4f, 0.02f, -2.2f);
    rlColor4ub(255, 240, 200, 28); rlVertex3f(4.7f, 3.15f, wz + 0.05f);
    rlEnd();
    // Bụi bay lơ lửng trong nắng
    for (int i = 0; i < 40; ++i) {
        float t = O.time * 0.15f + i * 0.37f;
        float x = 2.0f + std::fmod(i * 0.61f + std::sin(t) * 0.3f, 3.2f);
        float y = 0.3f + std::fmod(i * 0.53f + t * 0.4f, 2.8f);
        float z = -5.3f + std::fmod(i * 0.29f, 2.6f) + (3.0f - y) * 0.2f;
        DrawCube({ x, y, z }, 0.02f, 0.02f, 0.02f, Color{ 255, 240, 200, 90 });
    }
    if (Has(F_ARENA)) {
        float pulse = 0.6f + 0.4f * std::sin(O.time * 2.5f);
        for (int i = 0; i < 48; ++i) {                                          // vòng đèn quanh võ đài
            float a = i * 2 * PI / 48;
            DrawCube({ ARENA_POS.x + std::cos(a) * 2.05f, 0.21f, ARENA_POS.z + std::sin(a) * 2.05f }, 0.08f, 0.03f, 0.08f, Fade(pal::accent, pulse));
        }
    }
    gfx::EndGlow();
}

void office::RenderOffscreen() {
    SetupEnvironment();
    RenderScreens();
    gfx::RenderShadowMap([] { DrawRoom(); DrawArenaPad(); DrawDev(); });
}

void office::Draw3D() {
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    DrawRectangleGradientV(0, 0, (int)W, (int)H, Color{ 34, 38, 54, 255 }, Color{ 14, 16, 24, 255 });
    BeginMode3D(O.cam);
    gfx::BeginLit(O.cam);
    DrawRoom();
    DrawArenaPad();
    DrawDev();
    gfx::EndLit();
    DrawGlow();
    EndMode3D();
}

void office::DrawHUD(bool titleMode) {
    float s = ui::S();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    if (titleMode) return;

    // Nhãn nổi trên các đồ vật
    switch (O.zone) {
    case Zone::Computer: DrawPrompt({ DESK_POS.x, 2.6f, DESK_POS.z }, "Mở máy tính: học code"); break;
    case Zone::Arena:    DrawPrompt({ ARENA_POS.x, 2.2f, ARENA_POS.z }, Has(F_ARENA) ? "Vào võ đài: chơi thử game" : "Võ đài (chưa xây xong)"); break;
    case Zone::Shelf:    DrawPrompt({ SHELF_POS.x, 2.8f, SHELF_POS.z }, "Xem lộ trình dự án"); break;
    case Zone::Coffee:   DrawPrompt({ COFFEE_POS.x, 2.0f, COFFEE_POS.z }, "Uống cà phê (mẹo code)"); break;
    default: break;
    }

    // HUD tiến độ
    Rectangle hud = { 16 * s, 16 * s, 330 * s, 104 * s };
    ui::Panel(hud, Fade(pal::bg, 0.88f), pal::line, 0.08f);
    ui::Text("CODE VÕ ĐÀI", hud.x + 14 * s, hud.y + 10 * s, 22 * s, pal::accent, FS_BOLD);
    ui::Text(TextFormat("Tiến độ: %d/%d bài", g_progress.completed, F_COUNT), hud.x + 14 * s, hud.y + 40 * s, 16 * s, pal::text);
    ui::Text(TextFormat("★ %d   Thắng: %d", g_progress.TotalStars(), g_progress.wins), hud.x + 190 * s, hud.y + 40 * s, 16 * s, pal::accent);
    ui::Bar({ hud.x + 14 * s, hud.y + 68 * s, hud.width - 28 * s, 10 * s }, (float)g_progress.completed / F_COUNT, pal::good, pal::panel2);
    if (g_progress.completed < F_COUNT)
        ui::Text(TextFormat("Tiếp theo: Bài %d", g_progress.completed + 1), hud.x + 14 * s, hud.y + 82 * s, 13 * s, pal::dim);
    else
        ui::Text("Hoàn thành dự án!", hud.x + 14 * s, hud.y + 82 * s, 13 * s, pal::good);

    ui::Text("WASD: đi lại    E: tương tác", W - 270 * s, 20 * s, 15 * s, pal::dim);

    // Lời thoại của anh dev
    if (O.sayTime > 0 && !O.say.empty()) {
        float a = Clamp01(O.sayTime / 0.4f);
        float bw = std::min(W - 40 * s, 760 * s);
        Rectangle box = { (W - bw) / 2, 0, bw, 0 };
        float th = ui::Wrapped(O.say, { box.x + 20 * s, 0, bw - 40 * s, 0 }, 18 * s, pal::text, false);
        box.height = th + 50 * s;
        box.y = H - box.height - 20 * s;
        DrawRectangleRounded(box, 0.12f, 6, Fade2(Color{ 15, 17, 26, 235 }, a));
        DrawRectangleRoundedLinesEx(box, 0.12f, 6, 1.5f, Fade2(pal::accent2, a));
        ui::Text("Minh (dev)", box.x + 20 * s, box.y + 10 * s, 16 * s, Fade2(pal::accent2, a), FS_BOLD);
        ui::Wrapped(O.say, { box.x + 20 * s, box.y + 36 * s, bw - 40 * s, 0 }, 18 * s, Fade2(pal::text, a));
    }

    if (O.roadmap) DrawRoadmap();
}
