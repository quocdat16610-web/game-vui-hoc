// =============================================================================
//  Audio.h — âm thanh TỔNG HỢP BẰNG CODE (không cần file .wav nào!)
//  Mỗi tiếng đấm, đá, đỡ... được tạo từ sóng sin, sóng vuông và nhiễu trắng.
//  (Bài 16: Âm thanh trong game giải thích cách làm.)
// =============================================================================
#pragma once
#include <vector>

namespace sfx {
    enum Id { PUNCH, KICK, BLOCK, KO, WHOOSH, JUMP, CLICK, CORRECT, WRONG, UNLOCK, STEP, COUNT };

    void Init();
    void Shutdown();
    void Play(Id id, float pitchVariation = 0.08f, float volume = 1.0f);

    void SetCustomPunch(const std::vector<float>& samples, int sampleRate);   // tiếng đấm do người chơi tạo (Bài 16)
    void Update(bool wantMusic);     // gọi mỗi frame: tự lặp lại nhạc nền
    void ToggleMute();
    bool Muted();
}
