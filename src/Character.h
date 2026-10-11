// =============================================================================
//  Character.h — nhân vật 3D có khung xương & hoạt ảnh (model KayKit, CC0).
//
//  Kiến thức game: SKELETAL ANIMATION. Model có "bộ xương" 41 khớp; mỗi đỉnh
//  của lưới gắn với vài khớp kèm trọng số. Mỗi khung hình của hoạt ảnh lưu
//  vị trí/góc xoay của từng khớp; raylib tính lại vị trí đỉnh (skinning).
//  Tất cả nhân vật dùng chung một bộ xương nên dùng chung 76 hoạt ảnh.
// =============================================================================
#pragma once
#include "raylib.h"
#include <string>
#include <vector>

enum class CharModel { Knight, Barbarian, Mage, Rogue, RogueHooded, COUNT };

namespace chars {
    bool Init();                  // nạp hoạt ảnh dùng chung. false nếu thiếu file
    void Shutdown();
    bool Ready();
}

class AnimChar {
public:
    AnimChar() = default;
    ~AnimChar();
    AnimChar(const AnimChar&) = delete;
    AnimChar& operator=(const AnimChar&) = delete;

    bool Load(CharModel m);
    bool Loaded() const { return loaded_; }
    CharModel Kind() const { return kind_; }

    // Chơi hoạt ảnh theo tên (vd "Walking_A"). Nếu đang chơi rồi thì không làm gì.
    void Play(const char* name, bool loop = true, float speed = 1.0f);
    // Đặt khung hình theo tiến độ 0..1 trong đoạn [from, to] của hoạt ảnh (đồng bộ với FSM)
    void Scrub(const char* name, float progress, float from = 0.0f, float to = 1.0f);
    void Update(float dt);
    bool Finished() const;

    // yawDeg: 0 = nhìn về +Z. scale: 1 = cao khoảng 2.4 đơn vị
    void Draw(Vector3 pos, float yawDeg, float scale, float flash = 0.0f, float tiltDeg = 0.0f);

private:
    Model model_ = {};
    std::vector<bool> hidden_;
    bool loaded_ = false;
    CharModel kind_ = CharModel::Knight;
    int anim_ = -1, lastFrame_ = -1;
    float time_ = 0, speed_ = 1;
    bool loop_ = true, scrubbed_ = false;
    int scrubFrame_ = 0;
    int CurrentFrame() const;
};
