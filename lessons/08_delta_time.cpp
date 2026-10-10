// =============================================================================
//  BÀI 8 — GAME LOOP CHUẨN & DELTA TIME
//  Chạy: g++ -std=c++17 08_delta_time.cpp -o bai08 && ./bai08
//
//  So sánh: di chuyển "theo frame" (sai) và "theo thời gian" (đúng)
//  trên hai máy giả lập 30 FPS và 144 FPS. Rồi chạy một game loop thật
//  với std::chrono và fixed timestep.
// =============================================================================
#include <chrono>
#include <iostream>
#include <thread>

void MoPhong(int fps) {
    float dt = 1.0f / fps;
    float xSai = 0, xDung = 0;
    const float TOC_DO = 4.0f;               // 4 mét / giây
    for (int frame = 0; frame < fps; frame++) {   // chạy đúng 1 giây
        xSai += 0.0667f;                     // SAI: cộng cố định mỗi frame
        xDung += TOC_DO * dt;                // ĐÚNG: nhân với dt
    }
    std::cout << fps << " FPS sau 1 giay:  sai = " << xSai << " m   dung = " << xDung << " m\n";
}

int main() {
    MoPhong(30);
    MoPhong(60);
    MoPhong(144);
    std::cout << "=> Cach 'dung' luon ra 4 m du FPS la bao nhieu.\n\n";

    // ---------- Game loop thật với đồng hồ + fixed timestep ----------
    using Clock = std::chrono::steady_clock;
    const float STEP = 1.0f / 60.0f;         // vật lý chạy cố định 60 lần/giây
    float tichLuy = 0;
    float x = 0, dashTimer = 0, dashCooldown = 0;
    int soLanVatLy = 0, soLanVe = 0;

    auto truoc = Clock::now();
    auto batDau = truoc;
    while (std::chrono::duration<float>(Clock::now() - batDau).count() < 2.0f) {
        auto bayGio = Clock::now();
        float dt = std::chrono::duration<float>(bayGio - truoc).count();
        truoc = bayGio;
        if (dt > 0.1f) dt = 0.1f;            // chống "nhảy cóc" khi bị khựng
        tichLuy += dt;

        while (tichLuy >= STEP) {            // cập nhật vật lý
            // Bộ đếm thời gian: cứ hết hồi chiêu là lướt
            if (dashCooldown <= 0) { dashTimer = 0.18f; dashCooldown = 0.8f; }
            float tocDo = (dashTimer > 0) ? 18.0f : 4.0f;
            x += tocDo * STEP;
            dashTimer -= STEP;
            dashCooldown -= STEP;
            tichLuy -= STEP;
            soLanVatLy++;
        }
        soLanVe++;                           // "vẽ" (ở đây chỉ đếm)
        std::this_thread::sleep_for(std::chrono::milliseconds(7));   // giả lập máy ~144 FPS
    }
    std::cout << "Sau 2 giay: x = " << x << " m\n";
    std::cout << "Vat ly chay " << soLanVatLy << " lan (~120), ve " << soLanVe << " lan\n";

    // BÀI TẬP: thêm "đấm tích lực": giữ càng lâu (tối đa 1s) càng mạnh.
    return 0;
}
