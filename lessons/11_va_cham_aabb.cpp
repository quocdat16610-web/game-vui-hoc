// =============================================================================
//  BÀI 11 — VA CHẠM: HITBOX & HURTBOX (AABB)
//  Chạy: g++ -std=c++17 11_va_cham_aabb.cpp -o bai11 && ./bai11
// =============================================================================
#include <algorithm>
#include <cmath>
#include <iostream>

struct Vec3 { float x, y, z; };
struct AABB { Vec3 min, max; };

bool Overlap(const AABB& a, const AABB& b) {
    return a.min.x <= b.max.x && a.max.x >= b.min.x &&
           a.min.y <= b.max.y && a.max.y >= b.min.y &&
           a.min.z <= b.max.z && a.max.z >= b.min.z;
}

struct Fighter {
    const char* ten;
    Vec3  pos;
    float facing;          // +1 phải, -1 trái
    int   mau = 100;
    bool  daTrung = false; // đòn hiện tại đã trúng chưa
};

AABB Hurtbox(const Fighter& f) {        // thân người
    return { { f.pos.x - 0.35f, f.pos.y, -0.35f }, { f.pos.x + 0.35f, f.pos.y + 1.95f, 0.35f } };
}
AABB PunchBox(const Fighter& f) {       // nắm đấm, nằm trước mặt
    float cx = f.pos.x + f.facing * 0.85f;
    return { { cx - 0.25f, f.pos.y + 1.25f, -0.3f }, { cx + 0.25f, f.pos.y + 1.65f, 0.3f } };
}

void ThuDam(Fighter& a, Fighter& b, const char* moTa) {
    a.daTrung = false;
    std::cout << moTa << ": ";
    // Pha active kéo dài 6 frame — nhưng chỉ được gây sát thương 1 lần
    for (int frame = 0; frame < 6; frame++) {
        if (!a.daTrung && Overlap(PunchBox(a), Hurtbox(b))) {
            b.mau -= 8;
            a.daTrung = true;
            std::cout << "TRUNG! ";
        }
    }
    if (!a.daTrung) std::cout << "truot. ";
    std::cout << "(" << b.ten << " con " << b.mau << " mau)\n";
}

void DayNhau(Fighter& a, Fighter& b) {
    float d = b.pos.x - a.pos.x;
    const float MIN_DIST = 0.75f;
    if (std::fabs(d) < MIN_DIST) {
        float day = (MIN_DIST - std::fabs(d)) / 2;
        float dau = (d >= 0) ? 1.0f : -1.0f;
        a.pos.x -= dau * day;
        b.pos.x += dau * day;
    }
    a.pos.x = std::clamp(a.pos.x, -7.5f, 7.5f);
    b.pos.x = std::clamp(b.pos.x, -7.5f, 7.5f);
}

int main() {
    Fighter p = { "Minh", { 0, 0, 0 }, 1.0f };
    Fighter bot = { "Bot", { 1.0f, 0, 0 }, -1.0f };

    ThuDam(p, bot, "1. Dung gan, quay mat ve bot");
    bot.pos.x = 3.0f;
    ThuDam(p, bot, "2. Bot dung xa 3m        ");
    bot.pos.x = -0.9f;
    ThuDam(p, bot, "3. Bot o SAU LUNG        ");
    p.facing = -1.0f;
    ThuDam(p, bot, "4. Quay lai roi dam      ");
    bot.pos = { -0.9f, 2.5f, 0 };
    ThuDam(p, bot, "5. Bot dang nhay cao     ");

    std::cout << "\nDay nhau: truoc x = " << p.pos.x << " / " << -0.2f;
    bot.pos = { -0.2f, 0, 0 };
    DayNhau(p, bot);
    std::cout << "  ->  sau x = " << p.pos.x << " / " << bot.pos.x << "\n";

    // BÀI TẬP: thêm LowPunchBox ở độ cao 0.3 - 0.7m. Nó có trúng bot đang nhảy?
    return 0;
}
