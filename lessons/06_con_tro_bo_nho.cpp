// =============================================================================
//  BÀI 6 — CON TRỎ, BỘ NHỚ & SMART POINTER
//  Chạy: g++ -std=c++17 06_con_tro_bo_nho.cpp -o bai06 && ./bai06
// =============================================================================
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct Fighter {
    std::string ten;
    int mau;
    Fighter(std::string t, int m) : ten(t), mau(m) { std::cout << "  [new] " << ten << "\n"; }
    ~Fighter() { std::cout << "  [delete] " << ten << "\n"; }
};

struct Particle {
    float x, y;       // vị trí
    float vx, vy;     // vận tốc
    float life;       // thời gian sống còn lại (giây)
};

int main() {
    // ---------- 1. Con trỏ cơ bản ----------
    int mau = 100;
    int* p = &mau;                     // p giữ ĐỊA CHỈ của mau
    *p = 50;                           // sửa giá trị qua con trỏ
    std::cout << "mau = " << mau << ", dia chi = " << p << "\n\n";

    // ---------- 2. new / delete thủ công (cách cũ) ----------
    std::cout << "Cap phat thu cong:\n";
    Fighter* bot = new Fighter("Bot thu cong", 100);
    bot->mau -= 10;                    // -> để truy cập qua con trỏ
    delete bot;                        // quên dòng này = rò rỉ bộ nhớ
    bot = nullptr;

    // ---------- 3. unique_ptr: tự động giải phóng (RAII) ----------
    std::cout << "\nunique_ptr:\n";
    {
        auto boss = std::make_unique<Fighter>("Boss", 300);
        boss->mau -= 50;
        std::cout << "  Boss con " << boss->mau << " mau\n";
    }   // <- ra khỏi { } là boss tự bị delete
    std::cout << "  (da ra khoi pham vi)\n";

    std::vector<std::unique_ptr<Fighter>> roster;
    roster.push_back(std::make_unique<Fighter>("Ninja", 90));
    roster.push_back(std::make_unique<Fighter>("Robot", 130));
    std::unique_ptr<Fighter> chuyen = std::move(roster[0]);   // chuyển quyền sở hữu
    std::cout << "  roster[0] sau khi move: " << (roster[0] ? "con" : "nullptr") << "\n";

    // ---------- 4. Hệ thống hạt: vector lưu theo GIÁ TRỊ ----------
    std::cout << "\nHe thong hat (tia lua):\n";
    std::vector<Particle> sparks;
    for (int i = 0; i < 30; i++) {                       // một cú đấm: 30 tia lửa
        float vx = (std::rand() % 200 - 100) / 20.0f;
        float vy = (std::rand() % 100) / 15.0f;
        float life = 0.2f + (std::rand() % 100) / 200.0f;
        sparks.push_back({ 0, 1.5f, vx, vy, life });
    }
    const float dt = 1.0f / 60.0f;
    for (int frame = 0; frame <= 60; frame += 10) {
        for (int k = 0; k < 10; k++) {                   // mô phỏng 10 frame
            for (auto& s : sparks) {                     // auto& để SỬA phần tử
                s.x += s.vx * dt;
                s.y += s.vy * dt;
                s.life -= dt;
            }
            // erase-remove: xóa hạt đã hết thời gian sống
            sparks.erase(std::remove_if(sparks.begin(), sparks.end(),
                                        [](const Particle& s) { return s.life <= 0; }),
                         sparks.end());
        }
        std::cout << "  Frame " << frame + 10 << ": con " << sparks.size() << " hat\n";
    }

    // BÀI TẬP: thêm trọng lực: s.vy -= 9.8f * dt; mỗi frame.
    std::cout << "\nKet thuc main (cac unique_ptr con lai tu giai phong):\n";
    return 0;
}
