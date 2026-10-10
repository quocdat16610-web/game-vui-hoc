// =============================================================================
//  BÀI 16 — ÂM THANH TRONG GAME (tổng hợp âm thanh bằng code)
//  Cần raylib: build bằng CMake của dự án -> build/lessons/bai_16_am_thanh
//
//  Phím 1-5 phát các âm thanh được TẠO BẰNG TOÁN, màn hình vẽ dạng sóng.
//  Không cần file .wav nào!
// =============================================================================
#include "raylib.h"
#include <cmath>
#include <cstdint>
#include <vector>

const int RATE = 22050;                       // mẫu / giây
const float TAU = 6.2831853f;

float Nhieu() { return GetRandomValue(-10000, 10000) / 10000.0f; }   // nhiễu trắng

// Đổi mảng số thực [-1, 1] thành Sound của raylib
Sound TaoSound(const std::vector<float>& mau) {
    std::vector<int16_t> pcm(mau.size());
    for (size_t i = 0; i < mau.size(); i++) pcm[i] = (int16_t)(std::tanh(mau[i]) * 32000);
    Wave w = {};
    w.frameCount = (unsigned int)pcm.size();
    w.sampleRate = RATE;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = pcm.data();
    return LoadSoundFromWave(w);              // raylib sao chép dữ liệu
}

std::vector<float> Beep() {                   // sóng sin 440Hz (nốt La)
    std::vector<float> s(RATE / 3);
    for (size_t i = 0; i < s.size(); i++) {
        float t = (float)i / RATE;
        s[i] = std::sin(TAU * 440 * t) * std::exp(-t * 6) * 0.8f;
    }
    return s;
}

std::vector<float> Coin() {                   // sóng vuông 2 nốt: tiếng nhặt xu 8-bit
    std::vector<float> s(RATE / 4);
    for (size_t i = 0; i < s.size(); i++) {
        float t = (float)i / RATE;
        float f = (t < 0.07f) ? 988 : 1319;
        float sq = std::fmod(f * t, 1.0f) < 0.5f ? 0.4f : -0.4f;
        s[i] = sq * std::exp(-t * 8);
    }
    return s;
}

std::vector<float> NoKhoi() {                 // nhiễu lọc thông thấp: tiếng nổ
    std::vector<float> s(RATE);
    float lp = 0;
    for (size_t i = 0; i < s.size(); i++) {
        float t = (float)i / RATE;
        lp += (Nhieu() - lp) * 0.08f;         // bộ lọc 1 cực: làm tiếng "đục" hơn
        s[i] = lp * std::exp(-t * 4) * 2.5f;
    }
    return s;
}

std::vector<float> Dam() {                    // "thump" trượt tần số + nhiễu: tiếng đấm
    std::vector<float> s(RATE / 5);
    float pha = 0;
    for (size_t i = 0; i < s.size(); i++) {
        float t = (float)i / RATE;
        float f = 70 + 110 * std::exp(-t * 18);
        pha += f / RATE;
        s[i] = std::sin(TAU * pha) * std::exp(-t * 22) + Nhieu() * std::exp(-t * 40) * 0.6f;
    }
    return s;
}

std::vector<float> Nhay() {                   // sóng vuông trượt lên: tiếng nhảy
    std::vector<float> s(RATE / 6);
    float pha = 0;
    for (size_t i = 0; i < s.size(); i++) {
        float t = (float)i / RATE;
        pha += (280 + 2400 * t) / RATE;
        s[i] = (std::fmod(pha, 1.0f) < 0.5f ? 0.35f : -0.35f) * (1 - t * 6);
    }
    return s;
}

int main() {
    InitWindow(1000, 560, "Bai 16 - Am thanh tong hop bang code");
    InitAudioDevice();
    SetTargetFPS(60);

    std::vector<float> data[5] = { Beep(), Coin(), NoKhoi(), Dam(), Nhay() };
    const char* ten[5] = { "1: Beep (song sin)", "2: Xu 8-bit (song vuong)", "3: No (nhieu loc)", "4: Dam (thump + nhieu)", "5: Nhay (truot tan so)" };
    Sound snd[5];
    for (int i = 0; i < 5; i++) snd[i] = TaoSound(data[i]);
    int dangXem = 3;

    while (!WindowShouldClose()) {
        for (int k = 0; k < 5; k++)
            if (IsKeyPressed(KEY_ONE + k)) {
                dangXem = k;
                // Đổi cao độ ngẫu nhiên ±8%: nghe tự nhiên hơn khi lặp lại
                SetSoundPitch(snd[k], 0.92f + GetRandomValue(0, 16) / 100.0f);
                PlaySound(snd[k]);
            }

        BeginDrawing();
        ClearBackground(Color{ 20, 22, 34, 255 });
        DrawText("Nhan phim 1-5 de phat am thanh", 20, 20, 24, RAYWHITE);
        for (int i = 0; i < 5; i++)
            DrawText(ten[i], 20, 60 + i * 26, 20, i == dangXem ? ORANGE : GRAY);

        // Vẽ dạng sóng (waveform) của âm thanh đang xem
        const std::vector<float>& s = data[dangXem];
        int x0 = 20, y0 = 380, w = GetScreenWidth() - 40, h = 140;
        DrawRectangleLines(x0, y0 - h, w, h * 2, DARKGRAY);
        DrawLine(x0, y0, x0 + w, y0, DARKGRAY);
        for (int x = 0; x < w - 1; x++) {
            size_t a = s.size() * x / w, b = s.size() * (x + 1) / w;
            float va = std::tanh(s[a]), vb = std::tanh(s[b]);
            DrawLine(x0 + x, (int)(y0 - va * h), x0 + x + 1, (int)(y0 - vb * h), SKYBLUE);
        }
        DrawText(TextFormat("%d mau = %.2f giay", (int)s.size(), s.size() / (float)RATE), x0, y0 + h + 10, 18, GRAY);
        EndDrawing();
    }

    for (Sound& x : snd) UnloadSound(x);
    CloseAudioDevice();
    CloseWindow();
    // BÀI TẬP: thêm phím 6: tiếng "laser" — sóng vuông trượt từ 1500Hz xuống 200Hz trong 0.3 giây.
    return 0;
}
