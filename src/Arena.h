// =============================================================================
//  Arena.h — game đối kháng 3D mà anh dev đang viết.
//  Mỗi tính năng chỉ hoạt động khi bài học tương ứng đã hoàn thành
//  (xem g_progress.Has(F_...)).
// =============================================================================
#pragma once
#include "raylib.h"
#include <map>
#include <string>
#include <vector>

// Trạng thái của võ sĩ — một máy trạng thái hữu hạn (Bài 10)
enum class FState { Idle, Walk, Punch, Kick, Block, Hurt, KO, Victory };

enum class CharType { Dev, Ninja, Samurai, Robot, Dummy, Custom };

struct CharStats {
    const char* name;
    const char* desc;
    float speed;      // m/s
    float punch;      // sát thương đấm
    float kick;       // sát thương đá
    float maxHp;
    Color body, accent, skin;
};
const CharStats& GetCharacterStats(CharType t);

// Lệnh điều khiển. Bàn phím và AI đều tạo ra Input (Bài 10, 12)
struct Input {
    int  moveX = 0;
    bool jump = false, punch = false, kick = false, block = false, dash = false;
};

struct Fighter {
    CharType type = CharType::Dev;
    bool     isPlayer = false;
    bool     casual = false;      // tư thế đi lại bình thường (dùng trong văn phòng)
    int      modelSlot = -1;      // >= 0: vẽ bằng model 3D có xương (KayKit), < 0: khối hình tự dựng
    int      moveDir = 0;         // hướng đang đi (-1, 0, 1) để chọn hoạt ảnh đi tới / đi lùi
    float    showcaseYaw = 999;   // != 999: quay model theo góc này (màn chọn võ sĩ)
    Vector3  pos = { 0, 0, 0 };
    Vector3  vel = { 0, 0, 0 };
    float    facing = 1;          // +1 nhìn sang phải, -1 nhìn sang trái
    float    hp = 100, maxHp = 100;
    FState   state = FState::Idle;
    float    stateTime = 0;
    bool     hitDone = false;     // đòn hiện tại đã trúng chưa (Bài 11)
    bool     onGround = true;
    float    dashTimer = 0, dashCooldown = 0, dashDir = 1;
    int      combo = 0, bestCombo = 0;
    float    comboTimer = 0;
    float    flash = 0;           // nháy trắng khi trúng đòn
    float    walkPhase = 0;
    int      roundsWon = 0;
    std::map<std::string, int> stats;   // thống kê trận (Bài 7)
    std::vector<int> hits;               // sát thương từng đòn trúng

    void ChangeState(FState s) { state = s; stateTime = 0; hitDone = false; }
};

// Vẽ model võ sĩ dựng từ các khối hộp. animate = bật hoạt ảnh tay chân.
void DrawFighterModel(const Fighter& f, bool animate, float time);
// Cập nhật hoạt ảnh của model 3D (gọi 1 lần mỗi frame cho võ sĩ có modelSlot >= 0)
void UpdateFighterModel(const Fighter& f, bool animate, float dt);

namespace arena {
    void Init();
    void Shutdown();
    void Enter();
    bool Update(float dt);     // trả về true khi muốn quay lại văn phòng
    void RenderOffscreen();    // bóng đổ + màn hình LED (gọi trước khi vẽ cảnh)
    void Draw3D();
    void DrawHUD();
}
