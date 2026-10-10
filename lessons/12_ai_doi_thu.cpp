// =============================================================================
//  BÀI 12 — AI ĐỐI THỦ
//  Chạy: g++ -std=c++17 12_ai_doi_thu.cpp -o bai12 && ./bai12
//
//  Hai AI đấu với nhau trên một đường thẳng, in diễn biến ra màn hình.
//  AI chỉ tạo ra Input — phần di chuyển/đánh dùng chung cho mọi Fighter.
// =============================================================================
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

float Rand01() { return (std::rand() % 10001) / 10000.0f; }

struct Input { int moveX = 0; bool punch = false, block = false; };

struct Fighter {
    std::string ten;
    float x;
    int   mau = 100;
    float cooldown = 0;      // thời gian hồi đòn
    bool  dangDo = false;
    bool  vuaDam = false;
};

enum class AIMode { TiepCan, LuiLai, TanCong, PhongThu };
const char* TenMode(AIMode m) {
    switch (m) {
        case AIMode::TiepCan:  return "tiep can";
        case AIMode::LuiLai:   return "lui lai";
        case AIMode::TanCong:  return "tan cong";
        case AIMode::PhongThu: return "phong thu";
    }
    return "?";
}

struct AI {
    AIMode mode = AIMode::TiepCan;
    float timer = 0;
    float phanXa;            // thời gian phản xạ (độ khó)
    float tiLeDo;            // xác suất đỡ đòn
};

// 1. NGHĨ: chọn chế độ theo quan sát
void Think(AI& ai, const Fighter& me, const Fighter& foe, float dt) {
    ai.timer -= dt;
    if (ai.timer > 0) return;
    ai.timer = ai.phanXa;
    float d = std::fabs(foe.x - me.x);
    if (foe.vuaDam && d < 2.0f && Rand01() < ai.tiLeDo) ai.mode = AIMode::PhongThu;
    else if (d > 1.4f) ai.mode = AIMode::TiepCan;
    else ai.mode = (Rand01() < 0.7f) ? AIMode::TanCong : AIMode::LuiLai;
}

// 2. HÀNH ĐỘNG: tạo Input
Input Act(const AI& ai, const Fighter& me, const Fighter& foe) {
    Input in;
    int huong = (foe.x > me.x) ? 1 : -1;
    switch (ai.mode) {
        case AIMode::TiepCan:  in.moveX = huong; break;
        case AIMode::LuiLai:   in.moveX = -huong; break;
        case AIMode::PhongThu: in.block = true; break;
        case AIMode::TanCong:  in.punch = true; break;
    }
    return in;
}

// Logic Fighter dùng chung (không biết ai điều khiển)
void Apply(Fighter& f, Fighter& foe, const Input& in, float dt) {
    f.cooldown -= dt;
    f.dangDo = in.block;
    f.vuaDam = false;
    if (in.punch && f.cooldown <= 0) {
        f.cooldown = 0.5f;
        f.vuaDam = true;
        if (std::fabs(foe.x - f.x) < 1.3f) {
            int dmg = foe.dangDo ? 2 : 9;
            foe.mau -= dmg;
            std::cout << "    " << f.ten << " dam trung " << foe.ten << " -" << dmg
                      << (foe.dangDo ? " (bi do)" : "") << "\n";
        }
    }
    if (!in.block) f.x += in.moveX * 3.0f * dt;
}

int main() {
    std::srand((unsigned)std::time(nullptr));
    Fighter a{ "BotDe", -4 }, b{ "BotKho", 4 };
    AI aiA{ AIMode::TiepCan, 0, 0.45f, 0.15f };      // dễ: phản xạ chậm, ít đỡ
    AI aiB{ AIMode::TiepCan, 0, 0.18f, 0.60f };      // khó: phản xạ nhanh, hay đỡ

    const float DT = 0.05f;
    AIMode cuA = aiA.mode, cuB = aiB.mode;
    for (float t = 0; t < 30 && a.mau > 0 && b.mau > 0; t += DT) {
        Think(aiA, a, b, DT);
        Think(aiB, b, a, DT);
        Input ia = Act(aiA, a, b), ib = Act(aiB, b, a);
        Apply(a, b, ia, DT);
        Apply(b, a, ib, DT);
        if (aiA.mode != cuA || aiB.mode != cuB) {
            std::cout << "t=" << (int)(t * 100) / 100.0f << "s  " << a.ten << ": " << TenMode(aiA.mode)
                      << "  |  " << b.ten << ": " << TenMode(aiB.mode) << "\n";
            cuA = aiA.mode; cuB = aiB.mode;
        }
    }
    std::cout << "\nKet qua: " << a.ten << " " << a.mau << " mau, " << b.ten << " " << b.mau << " mau\n";

    // BÀI TẬP: thêm AIMode::PhanDon — đối thủ vừa đấm hụt thì đấm ngay.
    return 0;
}
