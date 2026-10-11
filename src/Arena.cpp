// =============================================================================
//  Arena.cpp — VÕ ĐÀI: game đối kháng 3D "Code Võ Đài".
//
//  Đọc file này sau khi học xong các bài: mọi khái niệm trong bài học đều
//  xuất hiện ở đây (FSM, delta time, AABB, AI, camera, particles...).
//  Các dòng if (g_progress.Has(F_...)) là "công tắc" mở khóa tính năng.
// =============================================================================
#include "Arena.h"
#include "Audio.h"
#include "PlayerCode.h"
#include "Gfx.h"
#include "Common.h"
#include "Lessons.h"
#include "UI.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <vector>

// -----------------------------------------------------------------------------
// Chỉ số nhân vật (Bài 5: mỗi nhân vật một bộ chỉ số)
// -----------------------------------------------------------------------------
static const CharStats CHAR_STATS[] = {
    { "Dev",     "Anh dev cân bằng mọi mặt",     4.0f,  8, 12, 100, {  58, 104, 196, 255 }, { 236, 236, 240, 255 }, { 240, 200, 160, 255 } },
    { "Ninja",   "Nhanh như gió, đòn nhẹ",        5.6f,  6, 10,  90, {  44,  44,  56, 255 }, { 222,  52,  64, 255 }, { 235, 195, 155, 255 } },
    { "Samurai", "Cân bằng, đòn đá rất mạnh",     4.2f,  8, 14, 105, { 150,  42,  42, 255 }, { 232, 190,  72, 255 }, { 230, 190, 150, 255 } },
    { "Robot",   "Chậm chạp nhưng cực kỳ trâu",   3.0f, 11, 16, 130, { 138, 148, 164, 255 }, {  64, 196, 255, 255 }, { 172, 182, 198, 255 } },
    { "Bao cát", "Chỉ biết đứng chịu đòn",        0.0f,  0,  0, 100, { 176, 124,  72, 255 }, { 120,  80,  46, 255 }, { 176, 124,  72, 255 } },
};

// Bài 5: võ sĩ do người chơi tự thiết kế bằng class VoSiCuaBan : public Character
static CharStats g_custom = { "Của bạn", "Do chính bạn lập trình (Bài 5)", 4.0f, 8, 12, 100,
                              { 70, 52, 140, 255 }, { 92, 214, 130, 255 }, { 236, 196, 160, 255 } };

const CharStats& GetCharacterStats(CharType t) {
    if (t == CharType::Custom) {
        float sp; int pu, ki, hp;
        if (pcode::VoSiCuaBan(sp, pu, ki, hp)) {
            g_custom.speed = Clamp(sp, 2.5f, 6.5f);
            g_custom.punch = (float)std::clamp(pu, 4, 14);
            g_custom.kick = (float)std::clamp(ki, 6, 18);
            g_custom.maxHp = (float)std::clamp(hp, 70, 140);
        }
        return g_custom;
    }
    return CHAR_STATS[(int)t];
}

static int CharCount() { return pcode::Active(F_CHARSELECT) ? 4 : 3; }   // có võ sĩ "Của bạn" không

static bool Has(Feature f) { return g_progress.Has(f); }
static float Rand01() { return GetRandomValue(0, 10000) / 10000.0f; }

namespace {
    // Hằng số vật lý & frame data (Bài 9, 10)
    const float GRAVITY    = -30.0f;
    const float JUMP_VEL   = 10.0f;
    const float ARENA_HALF = 7.5f;
    const float PUNCH_TIME = 0.35f, PUNCH_ON = 0.10f, PUNCH_OFF = 0.20f;
    const float KICK_TIME  = 0.55f, KICK_ON  = 0.22f, KICK_OFF  = 0.34f;
    const float HURT_TIME  = 0.32f;

    // Bài 14: các pha của trận đấu + luật chơi dạng dữ liệu
    enum class Phase { Select, Intro, Fight, RoundOver, MatchOver };
    struct MatchRules {
        int   roundsToWin = 1;
        float roundTime   = 0;   // 0 = không giới hạn
        int   difficulty  = 1;   // 0 dễ, 1 thường, 2 khó
    };

    struct Particle { Vector3 pos, vel; float life, maxLife, size; Color color; bool confetti = false; float spin = 0; };
    struct Popup    { Vector3 pos; std::string text; float life; Color color; float size; };

    // Bài 12: bộ não AI
    enum class AIMode { Approach, Retreat, Attack, Defend };
    struct AIBrain { AIMode mode = AIMode::Approach; float timer = 0, attackCd = 0, blockHold = 0; };

    struct ArenaState {
        Fighter p1, p2;
        Phase phase = Phase::Intro;
        float phaseTime = 0;
        int   round = 1;
        float timer = 0;
        MatchRules rules;
        std::vector<Particle> particles;
        std::vector<Popup> popups;
        AIBrain ai;
        Camera3D cam = {};
        Vector3 camPos = { 0, 2.5f, 10.0f }, camTarget = { 0, 1.55f, 0 };
        float shake = 0, hitStop = 0;
        bool  showHitbox = false, paused = false;
        int   selChar = 1, selDiff = 1;
        int   roundWinner = 0;           // 0 hòa, 1 người chơi, 2 đối thủ
        bool  timeUp = false;
        float time = 0;
        float trail1 = 1, trail2 = 1;    // vệt máu trắng chạy chậm
        std::string hint;
        float hintTime = 0;
        bool  wantExit = false;
        int   stage = 0;                 // 0 sân đấu ban ngày, 1 sân khấu Neon (Bài 15)
        float slowMo = 0;                // thời gian chạy chậm sau cú K.O.
        float flash = 0;                 // chớp trắng màn hình
        Vector3 hitLightPos = { 0, 0, 0 };
        float hitLightTime = 0;          // đèn chớp tại điểm trúng đòn
        RenderTexture2D screen = {};     // màn hình LED lớn phía sau
        bool  screenOk = false;
    };
    ArenaState S;
}

// -----------------------------------------------------------------------------
// Gợi ý khi bấm phím của tính năng chưa mở khóa
// -----------------------------------------------------------------------------
static void ShowHint(const std::string& msg, float t = 2.5f) { S.hint = msg; S.hintTime = t; }

static void LockedHint(Feature f) {
    const Lesson& l = GetLessons()[f];
    ShowHint(TextFormat("\"%s\" chưa được code! Học Bài %d: %s", l.unlock.c_str(), (int)f + 1, l.title.c_str()));
}

// -----------------------------------------------------------------------------
// Khởi tạo
// -----------------------------------------------------------------------------
static CharType SelType(int sel) { return sel == 4 ? CharType::Custom : (CharType)sel; }

static void ResetFighter(Fighter& f, CharType t, bool player) {
    int won = f.roundsWon;
    auto stats = f.stats;
    int best = f.bestCombo;
    auto hits = f.hits;
    f = Fighter{};
    f.type = t;
    f.isPlayer = player;
    f.modelSlot = player ? 0 : 1;
    f.maxHp = f.hp = GetCharacterStats(t).maxHp;
    f.pos = { player ? -2.2f : 2.2f, 0, 0 };
    f.facing = player ? 1.0f : -1.0f;
    f.roundsWon = won;
    f.stats = stats;
    f.bestCombo = best;
    f.hits = hits;
}

static void StartRound() {
    ResetFighter(S.p1, S.p1.type, true);
    ResetFighter(S.p2, S.p2.type, false);
    S.particles.clear();
    S.popups.clear();
    S.ai = AIBrain{};
    S.timer = S.rules.roundTime;
    S.timeUp = false;
    S.trail1 = S.trail2 = 1;
    S.phase = Phase::Intro;
    S.phaseTime = 0;
}

static void StartMatch() {
    S.rules = MatchRules{};
    if (Has(F_ROUNDS)) {
        S.rules.roundsToWin = 2;
        S.rules.roundTime = 60;
        S.rules.difficulty = S.selDiff;
    }
    CharType me = Has(F_CHARSELECT) ? SelType(S.selChar) : CharType::Dev;
    CharType foe = CharType::Dummy;
    if (Has(F_AI)) foe = Has(F_CHARSELECT) ? (CharType)GetRandomValue(1, 3) : CharType::Dev;
    S.p1 = Fighter{}; S.p2 = Fighter{};
    S.p1.type = me; S.p2.type = foe;
    S.round = 1;
    StartRound();
}

void arena::Enter() {
    S.time = 0;
    S.paused = false;
    S.wantExit = false;
    S.shake = S.hitStop = 0;
    S.camPos = { 0, 2.5f, 10.0f };
    S.camTarget = { 0, 1.55f, 0 };
    S.hint.clear();
    S.hintTime = 0;
    if (Has(F_CHARSELECT)) {
        S.phase = Phase::Select;
        S.phaseTime = 0;
        S.p1 = Fighter{};
        S.p1.type = SelType(S.selChar);
        S.p1.isPlayer = true;
        S.p1.modelSlot = 0;
        S.p2 = Fighter{};
    } else {
        StartMatch();
    }
}

// -----------------------------------------------------------------------------
// Input người chơi (Bài 10: bàn phím -> struct Input)
// -----------------------------------------------------------------------------
static Input ReadPlayerInput() {
    Input in;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  in.moveX -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) in.moveX += 1;
    in.jump  = IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP);
    in.punch = IsKeyPressed(KEY_J);
    in.kick  = IsKeyPressed(KEY_K);
    in.block = IsKeyDown(KEY_L);
    in.dash  = IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT);

    // Tính năng chưa học thì chưa dùng được
    if (!Has(F_PUNCH) && in.punch) { in.punch = false; LockedHint(F_PUNCH); }
    if (!Has(F_KICK_BLOCK) && (in.kick || IsKeyPressed(KEY_L))) { LockedHint(F_KICK_BLOCK); }
    if (!Has(F_KICK_BLOCK)) { in.kick = false; in.block = false; }
    if (!Has(F_DASH) && in.dash) { in.dash = false; LockedHint(F_DASH); }
    if (!Has(F_JUMP_FACING) && in.jump) { in.jump = false; LockedHint(F_JUMP_FACING); }
    return in;
}

// -----------------------------------------------------------------------------
// AI (Bài 12): nghĩ theo nhịp, rồi tạo Input giống bàn phím
// -----------------------------------------------------------------------------
static Input AIUpdate(AIBrain& ai, const Fighter& me, const Fighter& foe, float dt) {
    static const float REACTION[3] = { 0.45f, 0.30f, 0.17f };
    static const float BLOCK_P[3]  = { 0.15f, 0.35f, 0.60f };
    static const float AGGRO[3]    = { 0.50f, 0.65f, 0.80f };
    int d = S.rules.difficulty;

    Input in;
    ai.timer -= dt;
    ai.attackCd -= dt;
    ai.blockHold -= dt;
    float dist = std::fabs(foe.pos.x - me.pos.x);

    if (ai.timer <= 0) {                                   // AIThink
        ai.timer = REACTION[d] * (0.7f + Rand01() * 0.6f);
        bool foeAttacking = (foe.state == FState::Punch || foe.state == FState::Kick) && foe.stateTime < 0.2f;
        AIMode fallback;
        if (foeAttacking && dist < 2.2f && Rand01() < BLOCK_P[d]) fallback = AIMode::Defend;
        else if (dist > 1.5f) fallback = (Rand01() < 0.85f) ? AIMode::Approach : AIMode::Retreat;
        else fallback = (Rand01() < AGGRO[d]) ? AIMode::Attack : AIMode::Retreat;
        // Bài 12: bộ não AI do người chơi viết (0 tiếp cận, 1 lùi, 2 tấn công, 3 phòng thủ)
        static const AIMode MAP[4] = { AIMode::Approach, AIMode::Retreat, AIMode::Attack, AIMode::Defend };
        int pick = pcode::ChonHanhDong(dist, foeAttacking, Rand01(), (int)fallback);
        ai.mode = (pick >= 0 && pick < 4) ? MAP[pick] : fallback;
        if (ai.mode == AIMode::Defend) ai.blockHold = 0.45f;
    }

    int dir = (foe.pos.x > me.pos.x) ? 1 : -1;               // AIAct
    switch (ai.mode) {
    case AIMode::Approach:
        in.moveX = dir;
        if (Has(F_DASH) && dist > 4.0f && Rand01() < 0.02f) in.dash = true;
        if (Has(F_JUMP_FACING) && Rand01() < 0.004f) in.jump = true;
        break;
    case AIMode::Retreat:
        in.moveX = -dir;
        break;
    case AIMode::Defend:
        in.block = ai.blockHold > 0;
        if (!in.block) ai.mode = AIMode::Approach;
        break;
    case AIMode::Attack:
        if (ai.attackCd <= 0) {
            if (dist < 1.25f || Rand01() < 0.5f) in.punch = true;
            else in.kick = true;
            ai.attackCd = REACTION[d] * 1.6f;
        } else if (dist > 1.1f) {
            in.moveX = dir;
        }
        break;
    }
    return in;
}

// -----------------------------------------------------------------------------
// Hitbox / hurtbox (Bài 11)
// -----------------------------------------------------------------------------
static BoundingBox HurtBox(const Fighter& f) {
    return { { f.pos.x - 0.35f, f.pos.y, -0.35f }, { f.pos.x + 0.35f, f.pos.y + 1.95f, 0.35f } };
}

static BoundingBox AttackBox(const Fighter& f) {
    if (f.state == FState::Kick) {
        float cx = f.pos.x + f.facing * 1.0f;
        return { { cx - 0.3f, f.pos.y + 0.55f, -0.35f }, { cx + 0.3f, f.pos.y + 1.1f, 0.35f } };
    }
    float cx = f.pos.x + f.facing * 0.85f;
    return { { cx - 0.25f, f.pos.y + 1.25f, -0.3f }, { cx + 0.25f, f.pos.y + 1.65f, 0.3f } };
}

static bool AttackActive(const Fighter& f) {
    // Bài 10: frame data do người chơi viết (DAM = 1, DA = 2)
    if (f.state == FState::Punch) return pcode::DangGayDon(1, f.stateTime, f.stateTime >= PUNCH_ON && f.stateTime < PUNCH_OFF);
    if (f.state == FState::Kick)  return pcode::DangGayDon(2, f.stateTime, f.stateTime >= KICK_ON && f.stateTime < KICK_OFF);
    return false;
}

// -----------------------------------------------------------------------------
// Hạt tia lửa (Bài 6)
// -----------------------------------------------------------------------------
static void SpawnSparks(Vector3 at, int count, Color c, float dir) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = at;
        p.vel = { dir * (2.0f + Rand01() * 6.0f), -1.0f + Rand01() * 7.0f, -3.0f + Rand01() * 6.0f };
        p.maxLife = p.life = 0.25f + Rand01() * 0.4f;
        p.size = 0.05f + Rand01() * 0.08f;
        p.color = (i % 3 == 0) ? WHITE : c;
        S.particles.push_back(p);
    }
}

static void UpdateParticles(float dt) {
    for (auto& p : S.particles) {
        if (p.confetti) {
            p.vel.x += std::sin(S.time * 3 + p.spin) * dt * 2;   // pháo giấy bay lả tả
            p.spin += dt * 400;
            if (p.pos.y < 0.02f) { p.pos.y = 0.02f; p.vel = { 0, 0, 0 }; }
            p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
            p.life -= dt;
        } else if (pcode::CapNhatHat(p.pos.x, p.pos.y, p.vel.x, p.vel.y, p.life, dt)) {   // Bài 6: code người chơi
            p.pos.z += p.vel.z * dt;
        } else {
            p.vel.y += GRAVITY * 0.5f * dt;
            p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
            p.life -= dt;
        }
    }
    S.particles.erase(std::remove_if(S.particles.begin(), S.particles.end(),
                                     [](const Particle& p) { return p.life <= 0; }),
                      S.particles.end());
    for (auto& pp : S.popups) { pp.pos.y += dt * 1.2f; pp.life -= dt; }
    S.popups.erase(std::remove_if(S.popups.begin(), S.popups.end(),
                                  [](const Popup& p) { return p.life <= 0; }),
                   S.popups.end());
}

// -----------------------------------------------------------------------------
// Gây sát thương (Bài 3: hàm, Bài 7: combo + thống kê)
// -----------------------------------------------------------------------------
static void ApplyHit(Fighter& a, Fighter& b, bool kick) {
    const CharStats& cs = GetCharacterStats(a.type);
    int lucDanh = (int)(kick ? cs.kick : cs.punch);

    if (Has(F_COMBO)) {
        a.combo = (a.comboTimer > 0) ? a.combo + 1 : 1;
        a.comboTimer = 1.2f;
        lucDanh = (int)std::round(lucDanh * (1.0f + 0.1f * (a.combo - 1)));
        a.bestCombo = std::max(a.bestCombo, a.combo);
    }

    bool blocked = (b.state == FState::Block);
    if (blocked) b.stats["Đỡ đòn thành công"]++;

    // Bài 3: hàm TinhSatThuong của người chơi
    int dmg = pcode::TinhSatThuong(lucDanh, blocked, blocked ? lucDanh / 4 : lucDanh);
    if (dmg < 0) dmg = 0;

    // Bài 4: hàm thành viên Fighter::NhanDon của người chơi
    int mau = (int)std::ceil(b.hp);
    bool conSong = mau > 0;
    if (pcode::NhanDon(mau, conSong, dmg)) {
        b.hp = (float)std::clamp(mau, 0, (int)b.maxHp);
        if (!conSong) b.hp = 0;
    } else {
        b.hp = std::max(0.0f, b.hp - dmg);
    }
    a.stats[kick ? "Đá trúng" : "Đấm trúng"]++;
    a.stats["Sát thương gây ra"] += dmg;
    a.hits.push_back(dmg);

    float dir = (b.pos.x >= a.pos.x) ? 1.0f : -1.0f;
    if (b.type != CharType::Dummy)                   // bao cát cắm cố định, không bị đẩy lùi
        b.vel.x = dir * (blocked ? 2.0f : (kick ? 6.0f : 4.0f));
    b.flash = 1.0f;

    bool ko = b.hp <= 0;
    if (ko) {
        b.ChangeState(FState::KO);
        if (b.type != CharType::Dummy) {
            b.vel.y = 6.0f;
            b.vel.x = dir * 6.0f;
            b.onGround = false;
        }
    } else if (!blocked && Has(F_ANIM_STATES)) {
        b.ChangeState(FState::Hurt);                 // choáng (hitstun)
    }

    Vector3 contact = { a.pos.x + a.facing * (kick ? 0.95f : 0.8f), a.pos.y + (kick ? 0.85f : 1.45f), 0.3f };
    S.popups.push_back({ { b.pos.x, b.pos.y + 2.2f, 0 },
                         blocked ? "ĐỠ!" : TextFormat("-%d", dmg),
                         0.9f, blocked ? pal::accent2 : (kick ? pal::accent : WHITE), blocked ? 26.0f : 30.0f });

    S.hitLightPos = contact;
    S.hitLightTime = 0.15f;
    if (Has(F_PARTICLES))
        SpawnSparks(contact, blocked ? 12 : (kick ? 34 : 24), blocked ? pal::accent2 : pal::accent, dir);

    if (Has(F_CAMERA_FX)) {
        S.hitStop = ko ? 0.25f : (kick ? 0.10f : 0.06f);
        S.shake   = ko ? 0.60f : (blocked ? 0.10f : (kick ? 0.40f : 0.22f));
        if (ko) { S.slowMo = 1.1f; S.flash = 0.6f; }      // K.O.: chớp trắng + quay chậm
    }

    // Âm thanh (Bài 16)
    if (ko) sfx::Play(sfx::KO, 0.03f);
    else if (blocked) sfx::Play(sfx::BLOCK);
    else sfx::Play(kick ? sfx::KICK : sfx::PUNCH, 0.1f);
}

static void CheckAttack(Fighter& a, Fighter& b) {
    if (a.hitDone || b.state == FState::KO || !AttackActive(a)) return;
    bool kick = (a.state == FState::Kick);
    bool hit;
    if (Has(F_HITBOX)) {
        BoundingBox ab = AttackBox(a), hb = HurtBox(b);
        hit = pcode::VaCham(ab.min.x, ab.min.y, ab.max.x, ab.max.y, hb.min.x, hb.min.y, hb.max.x, hb.max.y,
                            CheckCollisionBoxes(ab, hb));                     // Bài 11: code người chơi
    } else {
        // Trước Bài 11: chỉ đo khoảng cách -> đấm sau lưng vẫn trúng!
        float reach = kick ? 1.6f : 1.4f;
        hit = std::fabs(b.pos.x - a.pos.x) < reach && std::fabs(b.pos.y - a.pos.y) < 1.2f;
    }
    if (hit) {
        a.hitDone = true;
        ApplyHit(a, b, kick);
    }
}

// -----------------------------------------------------------------------------
// Cập nhật một võ sĩ: máy trạng thái (Bài 10) + vật lý (Bài 8, 9)
// -----------------------------------------------------------------------------
static void UpdateFighter(Fighter& f, const Input& in, const Fighter& foe, float dt) {
    const CharStats& cs = GetCharacterStats(f.type);
    f.stateTime += dt;
    f.flash = std::max(0.0f, f.flash - dt * 4.0f);
    f.dashCooldown -= dt;
    f.comboTimer -= dt;
    if (f.comboTimer <= 0) f.combo = 0;

    bool free = (f.state == FState::Idle || f.state == FState::Walk || f.state == FState::Block);

    // Bài 9: tự quay mặt về phía đối thủ
    if (Has(F_JUMP_FACING) && free && f.type != CharType::Dummy)
        f.facing = (foe.pos.x >= f.pos.x) ? 1.0f : -1.0f;

    switch (f.state) {
    case FState::Idle:
    case FState::Walk:
    case FState::Block:
        if (in.block && f.onGround) {
            if (f.state != FState::Block) f.ChangeState(FState::Block);
        } else if (in.punch) {
            f.ChangeState(FState::Punch);
            f.stats["Số lần ra đòn"]++;
            sfx::Play(sfx::WHOOSH, 0.15f, 0.35f);
        } else if (in.kick) {
            f.ChangeState(FState::Kick);
            f.stats["Số lần ra đòn"]++;
            sfx::Play(sfx::WHOOSH, 0.15f, 0.5f);
        } else {
            FState next = (in.moveX != 0) ? FState::Walk : FState::Idle;
            if (next != f.state) f.ChangeState(next);
            if (in.jump && f.onGround) { f.vel.y = JUMP_VEL; f.onGround = false; sfx::Play(sfx::JUMP, 0.05f, 0.6f); }
            if (in.dash && f.dashCooldown <= 0 && in.moveX != 0) {
                f.dashTimer = 0.18f;
                f.dashCooldown = 0.8f;
                f.dashDir = (float)in.moveX;
                sfx::Play(sfx::WHOOSH, 0.1f, 0.8f);
            }
        }
        break;
    case FState::Punch: if (f.stateTime >= PUNCH_TIME) f.ChangeState(FState::Idle); break;
    case FState::Kick:  if (f.stateTime >= KICK_TIME)  f.ChangeState(FState::Idle); break;
    case FState::Hurt:  if (f.stateTime >= HURT_TIME)  f.ChangeState(FState::Idle); break;
    case FState::KO:
    case FState::Victory: break;
    }

    // Di chuyển ngang: tốc độ * dt (Bài 8)
    int huong = 0;
    float tocDo = cs.speed;
    if (f.state == FState::Walk) huong = in.moveX;
    if (!f.onGround && (f.state == FState::Punch || f.state == FState::Kick)) { huong = in.moveX; tocDo *= 0.6f; }
    if (f.dashTimer > 0) {
        huong = (int)f.dashDir;
        tocDo = 18.0f;
        f.dashTimer -= dt;
    }
    f.moveDir = huong;
    if (huong != 0 && dt > 0)                          // Bài 8: hàm DiChuyen của người chơi
        f.pos.x = pcode::DiChuyen(f.pos.x, huong, tocDo, dt, f.pos.x + huong * tocDo * dt);
    f.pos.x += f.vel.x * dt;
    f.vel.x = Approach(f.vel.x, 0, (f.onGround ? 14.0f : 4.0f) * dt);   // knockback giảm dần

    // Trọng lực (Bài 9)
    if (!f.onGround || f.vel.y > 0) {
        if (dt > 0 && !pcode::TrongLuc(f.pos.y, f.vel.y, dt)) {    // Bài 9: hàm TrongLuc của người chơi
            f.vel.y += GRAVITY * dt;
            f.pos.y += f.vel.y * dt;
        }
        if (f.pos.y <= 0) { f.pos.y = 0; f.vel.y = 0; f.onGround = true; }
    }
    f.pos.x = Clamp(f.pos.x, -ARENA_HALF, ARENA_HALF);

    if (f.state == FState::Walk) f.walkPhase += cs.speed * dt * 2.2f;
}

// -----------------------------------------------------------------------------
// Camera (Bài 13)
// -----------------------------------------------------------------------------
static void UpdateCam(float dt) {
    if (S.phase == Phase::Select) {
        S.camPos = { 0.6f, 2.1f, 6.0f };                 // cận cảnh võ sĩ đang chọn
        S.camTarget = { 0, 1.2f, 0 };
    } else if (Has(F_CAMERA_FX)) {
        float mid = (S.p1.pos.x + S.p2.pos.x) * 0.5f;
        float dist = std::fabs(S.p1.pos.x - S.p2.pos.x);
        Vector3 wantPos = { mid * 0.85f, 3.0f + dist * 0.08f, 8.0f + dist * 0.55f };
        Vector3 wantTarget = { mid * 0.9f, 1.3f, 0 };
        float k = std::min(1.0f, 4.0f * dt);
        auto L = [k](float a, float b) { return pcode::Lerp(a, b, k, a + (b - a) * k); };   // Bài 13: hàm Lerp của người chơi
        S.camPos = { L(S.camPos.x, wantPos.x), L(S.camPos.y, wantPos.y), L(S.camPos.z, wantPos.z) };
        S.camTarget = { L(S.camTarget.x, wantTarget.x), L(S.camTarget.y, wantTarget.y), L(S.camTarget.z, wantTarget.z) };
    } else {
        S.camPos = { 0, 2.5f, 10.0f };
        S.camTarget = { 0, 1.55f, 0 };
    }
    Vector3 off = { 0, 0, 0 };
    if (S.shake > 0) {
        off = { (Rand01() * 2 - 1) * S.shake * 0.5f, (Rand01() * 2 - 1) * S.shake * 0.5f, 0 };
        S.shake -= dt * 2.0f;
    }
    S.cam.position = Vector3Add(S.camPos, off);
    S.cam.target = Vector3Add(S.camTarget, Vector3Scale(off, 0.5f));
    S.cam.up = { 0, 1, 0 };
    S.cam.fovy = 45;
    S.cam.projection = CAMERA_PERSPECTIVE;
}

// -----------------------------------------------------------------------------
// Vòng cập nhật võ đài
// -----------------------------------------------------------------------------
static void UpdateSelect() {
    int n = CharCount();
    if (S.selChar > n) S.selChar = 1;
    if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))  S.selChar = (S.selChar == 1) ? n : S.selChar - 1;
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) S.selChar = (S.selChar == n) ? 1 : S.selChar + 1;
    if (Has(F_ROUNDS)) {
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))   S.selDiff = std::max(0, S.selDiff - 1);
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) S.selDiff = std::min(2, S.selDiff + 1);
    }
    if (Has(F_NEON_STAGE) && (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_E))) { S.stage = 1 - S.stage; sfx::Play(sfx::CLICK); }
    if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) sfx::Play(sfx::CLICK);
    S.p1.type = SelType(S.selChar);
    S.p1.pos = { 0, 0, 0 };
    S.p1.facing = 1;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_J)) StartMatch();
}

bool arena::Update(float dt) {
    S.time += dt;
    S.hintTime -= dt;

    if (IsKeyPressed(KEY_ESCAPE)) return true;

    if (S.phase == Phase::Select) {
        UpdateSelect();
        UpdateFighterModel(S.p1, true, dt);
        UpdateCam(dt);
        return S.wantExit;
    }

    if (IsKeyPressed(KEY_H)) {
        if (Has(F_HITBOX)) S.showHitbox = !S.showHitbox;
        else LockedHint(F_HITBOX);
    }
    if (IsKeyPressed(KEY_P)) {
        if (Has(F_ROUNDS)) S.paused = !S.paused;
        else LockedHint(F_ROUNDS);
    }
    if (S.paused) return S.wantExit;

    if (S.phase == Phase::MatchOver) {
        UpdateFighterModel(S.p1, Has(F_ANIM_STATES), dt);
        UpdateFighterModel(S.p2, Has(F_ANIM_STATES), dt);
        UpdateParticles(dt);
        UpdateCam(dt);
        if (IsKeyPressed(KEY_ENTER)) arena::Enter();
        return S.wantExit;
    }

    // Hit-stop: logic dừng lại vài khung hình (Bài 13)
    float gdt = dt;
    if (S.hitStop > 0) { S.hitStop -= dt; gdt = 0; }
    if (S.slowMo > 0) { S.slowMo -= dt; gdt *= 0.3f; }    // quay chậm sau cú K.O.
    S.flash = std::max(0.0f, S.flash - dt * 2.5f);
    S.hitLightTime -= dt;

    S.phaseTime += gdt;

    Input in1, in2;
    if (S.phase == Phase::Fight) {
        in1 = ReadPlayerInput();
        if (Has(F_AI) && S.p2.type != CharType::Dummy) in2 = AIUpdate(S.ai, S.p2, S.p1, gdt);
    }
    UpdateFighter(S.p1, in1, S.p2, gdt);
    UpdateFighter(S.p2, in2, S.p1, gdt);
    UpdateFighterModel(S.p1, Has(F_ANIM_STATES), gdt);   // hoạt ảnh model 3D (dừng theo hit-stop)
    UpdateFighterModel(S.p2, Has(F_ANIM_STATES), gdt);

    CheckAttack(S.p1, S.p2);
    CheckAttack(S.p2, S.p1);

    // Bài 11: hai võ sĩ đẩy nhau, không đi xuyên qua
    if (Has(F_HITBOX) && S.p1.state != FState::KO && S.p2.state != FState::KO &&
        std::fabs(S.p1.pos.y - S.p2.pos.y) < 1.5f) {
        float d = S.p2.pos.x - S.p1.pos.x;
        const float MIN_DIST = 0.75f;
        if (std::fabs(d) < MIN_DIST) {
            float push = (MIN_DIST - std::fabs(d)) * 0.5f;
            float sgn = (d >= 0) ? 1.0f : -1.0f;
            S.p1.pos.x = Clamp(S.p1.pos.x - sgn * push, -ARENA_HALF, ARENA_HALF);
            S.p2.pos.x = Clamp(S.p2.pos.x + sgn * push, -ARENA_HALF, ARENA_HALF);
        }
    }

    UpdateParticles(gdt);
    UpdateCam(dt);

    S.trail1 = Approach(S.trail1, S.p1.hp / S.p1.maxHp, dt * 0.5f);
    S.trail2 = Approach(S.trail2, S.p2.hp / S.p2.maxHp, dt * 0.5f);

    // Các pha của trận đấu (Bài 2: if/else, Bài 14: Phase)
    switch (S.phase) {
    case Phase::Intro:
        if (S.phaseTime > 1.6f) { S.phase = Phase::Fight; S.phaseTime = 0; }
        break;
    case Phase::Fight: {
        bool ko1 = S.p1.state == FState::KO, ko2 = S.p2.state == FState::KO;
        if (S.rules.roundTime > 0) {
            S.timer -= gdt;
            if (S.timer <= 0 && !ko1 && !ko2) {
                S.timer = 0;
                S.timeUp = true;
                float a = S.p1.hp / S.p1.maxHp, b = S.p2.hp / S.p2.maxHp;
                S.roundWinner = pcode::NguoiThang(a, b, (a > b) ? 1 : (b > a ? 2 : 0));   // Bài 14
                S.phase = Phase::RoundOver; S.phaseTime = 0;
            }
        }
        // Bài 2: hàm KetThucHiep của người chơi quyết định khi nào hết hiệp
        bool over = pcode::KetThucHiep((int)std::ceil(S.p1.hp), (int)std::ceil(S.p2.hp), ko1 || ko2);
        if (over && S.phase == Phase::Fight) {
            ko1 = ko1 || S.p1.hp <= 0;
            ko2 = ko2 || S.p2.hp <= 0;
            S.roundWinner = ko2 && !ko1 ? 1 : (ko1 && !ko2 ? 2 : 0);
            S.phase = Phase::RoundOver; S.phaseTime = 0;
        }
        if (S.phase == Phase::RoundOver) {
            if (S.roundWinner == 1) S.p1.roundsWon++;
            if (S.roundWinner == 2) S.p2.roundsWon++;
        }
        break;
    }
    case Phase::RoundOver: {
        Fighter* w = S.roundWinner == 1 ? &S.p1 : (S.roundWinner == 2 ? &S.p2 : nullptr);
        if (w && S.phaseTime > 0.8f && w->state != FState::Victory && w->type != CharType::Dummy && w->onGround)
            w->ChangeState(FState::Victory);
        if (S.phaseTime > 2.8f) {
            bool done = S.p1.roundsWon >= S.rules.roundsToWin || S.p2.roundsWon >= S.rules.roundsToWin || S.round >= 5;
            if (done) {
                S.phase = Phase::MatchOver; S.phaseTime = 0;
                if (S.p1.roundsWon > S.p2.roundsWon) {
                    g_progress.wins++;
                    g_progress.Save();
                    // Pháo giấy ăn mừng
                    const Color cols[5] = { pal::accent, pal::accent2, pal::good, pal::bad, pal::purple };
                    for (int i = 0; i < 260; ++i) {
                        Particle p;
                        p.pos = { S.p1.pos.x + (Rand01() * 2 - 1) * 6, 7.0f + Rand01() * 3, -1.5f + Rand01() * 3 };
                        p.vel = { (Rand01() * 2 - 1) * 1.5f, -1.0f - Rand01() * 1.5f, 0 };
                        p.maxLife = p.life = 4.0f + Rand01() * 2;
                        p.size = 0.09f;
                        p.color = cols[i % 5];
                        p.confetti = true;
                        p.spin = Rand01() * 360;
                        S.particles.push_back(p);
                    }
                }
            } else {
                S.round++;
                StartRound();
            }
        }
        break;
    }
    default: break;
    }
    return S.wantExit;
}

// =============================================================================
//  VẼ
// =============================================================================
// =============================================================================
//  SÂN KHẤU & ÁNH SÁNG (Bài 15)
// =============================================================================
static bool Neon() { return S.stage == 1; }

// Thiết lập "không khí" ánh sáng cho sân khấu hiện tại
static void SetupEnvironment() {
    gfx::Environment e;
    float mid = (S.phase == Phase::Select) ? 0.0f : (S.p1.pos.x + S.p2.pos.x) * 0.5f;
    e.shadowCenter = { mid, 0, 0 };
    e.shadowSize = 20;
    if (!Neon()) {
        e.sunDir = { 0.3f, -1.0f, -0.45f };
        e.sunColor = { 255, 238, 214, 255 };
        e.sunIntensity = 1.15f;
        e.sky = { 125, 140, 196, 255 };
        e.ground = { 76, 54, 66, 255 };
        e.ambient = 0.5f;
        e.fog = { 34, 24, 52, 255 };
        e.fogNear = 18; e.fogFar = 46;
        e.lights[0] = { { -7, 4, 2 }, pal::accent, 1.3f, 9 };
        e.lights[1] = { { 7, 4, 2 }, pal::accent2, 1.3f, 9 };
        e.lights[2] = { { 0, 6, -7.5f }, pal::purple, 1.2f, 9 };
        e.lightCount = 3;
        e.bloom = 0.45f; e.bloomThreshold = 0.8f;
    } else {
        e.sunDir = { -0.2f, -1.0f, -0.35f };
        e.sunColor = { 130, 150, 255, 255 };
        e.sunIntensity = 0.4f;
        e.sky = { 50, 46, 110, 255 };
        e.ground = { 36, 12, 46, 255 };
        e.ambient = 0.5f;
        e.fog = { 8, 6, 22, 255 };
        e.fogNear = 14; e.fogFar = 36;
        float sw = std::sin(S.time * 0.9f) * 6.0f;
        e.lights[0] = { { -5, 3, 1.5f }, Color{ 255, 60, 200, 255 }, 2.2f, 8 };
        e.lights[1] = { { 5, 3, 1.5f }, Color{ 40, 220, 255, 255 }, 2.2f, 8 };
        e.lights[2] = { { sw, 5, 0.5f }, Color{ 255, 170, 60, 255 }, 1.8f, 7 };
        e.lights[3] = { { 0, 5, -6 }, Color{ 150, 80, 255, 255 }, 1.8f, 11 };
        e.lightCount = 4;
        e.bloom = 1.0f; e.bloomThreshold = 0.6f; e.saturation = 1.2f;
    }
    // Ánh chớp tại điểm trúng đòn: đèn điểm sống 0.15 giây
    if (S.hitLightTime > 0 && e.lightCount < 8)
        e.lights[e.lightCount++] = { S.hitLightPos, Color{ 255, 200, 120, 255 }, 4.0f * (S.hitLightTime / 0.15f), 4 };
    gfx::SetEnvironment(e);
}

static Color CrowdColor(int i, int row) {
    return Color{ (unsigned char)(80 + (i * 37 + row * 53) % 150), (unsigned char)(70 + (i * 71 + row * 29) % 120),
                  (unsigned char)(90 + (i * 13 + row * 97) % 140), 255 };
}

static Vector3 CrowdPos(int i, int row) {
    float x = -11.0f + i * 0.85f + (row % 2) * 0.4f;
    float jump = std::fabs(std::sin(S.time * (3.0f + (i % 5)) + i * 1.7f + row)) * (S.phase == Phase::RoundOver ? 0.3f : 0.12f);
    return { x, row * 1.2f + 0.6f + jump, -4.5f - row * 1.2f };
}

// Vật thể đặc, được chiếu sáng
static void DrawStageSolid() {
    Color base = Neon() ? Color{ 24, 24, 36, 255 } : Color{ 52, 56, 76, 255 };
    gfx::Box({ 0, -0.3f, 0 }, { 17.4f, 0.6f, 5.4f }, base);
    for (int x = -8; x < 8; ++x)
        for (int z = -2; z < 2; ++z) {
            bool odd = (x + z) & 1;
            Color c = Neon() ? (odd ? Color{ 34, 32, 52, 255 } : Color{ 26, 26, 40, 255 })
                             : (odd ? Color{ 66, 72, 98, 255 } : Color{ 56, 61, 84, 255 });
            gfx::Box({ x + 0.5f, 0.0f, z + 0.5f }, { 0.96f, 0.02f, 0.96f }, c);
        }
    // Cột góc và dây đài phía sau
    Color metal = { 150, 156, 170, 255 };
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
            gfx::Cylinder({ sx * 8.5f, 0, sz * 2.55f }, { sx * 8.5f, 1.6f, sz * 2.55f }, 0.09f, 0.09f, metal, 10);
            gfx::Box({ sx * 8.5f, 1.65f, sz * 2.55f }, { 0.26f, 0.12f, 0.26f }, sx < 0 ? pal::bad : pal::accent2);
        }
    const Color ropes[3] = { pal::bad, Color{ 235, 235, 240, 255 }, pal::accent2 };
    for (int r = 0; r < 3; ++r) {
        float y = 0.55f + r * 0.42f;
        gfx::Cylinder({ -8.5f, y, -2.55f }, { 8.5f, y, -2.55f }, 0.035f, 0.035f, ropes[r], 8, false);
        for (int sx = -1; sx <= 1; sx += 2)
            gfx::Cylinder({ sx * 8.5f, y, -2.55f }, { sx * 8.5f, y, 2.55f }, 0.035f, 0.035f, ropes[r], 8, false);
    }
    // Cột trụ và loa
    for (int s = -1; s <= 1; s += 2) {
        gfx::Box({ s * 10.0f, 2.3f, -3.0f }, { 0.7f, 4.6f, 0.7f }, Color{ 36, 40, 56, 255 });
        gfx::Box({ s * 10.0f, 0.9f, -1.6f }, { 1.0f, 1.8f, 0.8f }, Color{ 24, 24, 30, 255 });
        gfx::Cylinder({ s * 10.0f, 1.25f, -1.19f }, { s * 10.0f, 1.25f, -1.17f }, 0.32f, 0.32f, Color{ 50, 50, 60, 255 }, 16);
        gfx::Cylinder({ s * 10.0f, 0.5f, -1.19f }, { s * 10.0f, 0.5f, -1.17f }, 0.2f, 0.2f, Color{ 50, 50, 60, 255 }, 16);
    }
    // Khán đài + khán giả
    for (int row = 0; row < 3; ++row) {
        gfx::Box({ 0, row * 0.6f + 0.3f, -4.5f - row * 1.2f }, { 24.0f, 0.6f + row * 1.2f, 1.2f }, Color{ 34, 36, 50, 255 });
        for (int i = 0; i < 28; ++i) {
            Vector3 b = CrowdPos(i, row);
            gfx::Box({ b.x, b.y + 0.3f, b.z }, { 0.42f, 0.6f, 0.32f }, CrowdColor(i, row));
            gfx::Sphere({ b.x, b.y + 0.78f, b.z }, 0.16f, Color{ 222, 186, 152, 255 }, 5, 7);
        }
    }
    // Giàn đèn phía trên
    gfx::Box({ 0, 6.6f, 0.2f }, { 20.0f, 0.25f, 0.25f }, Color{ 70, 74, 86, 255 });
    gfx::Box({ 0, 6.6f, -0.6f }, { 20.0f, 0.25f, 0.25f }, Color{ 70, 74, 86, 255 });
    for (int i = 0; i < 4; ++i) {
        float x = -5.4f + i * 3.6f;
        gfx::Cylinder({ x, 6.45f, -0.2f }, { x, 6.05f, -0.2f }, 0.18f, 0.24f, Color{ 30, 30, 36, 255 }, 12);
    }
    // Khung màn hình LED
    gfx::Box({ 0, 5.3f, -9.0f }, { 10.0f, 3.2f, 0.3f }, Color{ 22, 24, 32, 255 });

    // Pháo giấy
    for (const auto& p : S.particles) {
        if (!p.confetti) continue;
        rlPushMatrix();
        rlTranslatef(p.pos.x, p.pos.y, p.pos.z);
        rlRotatef(p.spin, 0.3f, 1, 0.5f);
        gfx::Box({ 0, 0, 0 }, { p.size, p.size * 0.6f, 0.01f }, p.color);
        rlPopMatrix();
    }
}

// Quad có texture (dùng cho màn hình LED)
static void TexturedQuad(Texture2D tex, Vector3 c, float w, float h) {
    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 255);
    rlNormal3f(0, 0, 1);
    rlTexCoord2f(0, 0); rlVertex3f(c.x - w / 2, c.y - h / 2, c.z);
    rlTexCoord2f(1, 0); rlVertex3f(c.x + w / 2, c.y - h / 2, c.z);
    rlTexCoord2f(1, 1); rlVertex3f(c.x + w / 2, c.y + h / 2, c.z);
    rlTexCoord2f(0, 1); rlVertex3f(c.x - w / 2, c.y + h / 2, c.z);
    rlEnd();
    rlSetTexture(0);
}

// Vật phát sáng: vẽ không qua shader ánh sáng, cộng màu (additive)
static void DrawStageGlow() {
    if (S.screenOk) TexturedQuad(S.screen.texture, { 0, 5.3f, -8.84f }, 9.4f, 2.7f);

    gfx::BeginGlow();
    Color stripA = Neon() ? Color{ 255, 60, 200, 255 } : pal::accent;
    Color stripB = Neon() ? Color{ 40, 220, 255, 255 } : pal::accent2;
    DrawCube({ 0, 0.03f, 2.72f }, 17.4f, 0.05f, 0.06f, stripA);
    DrawCube({ 0, -0.3f, 2.71f }, 17.4f, 0.04f, 0.02f, Fade(stripA, 0.6f));
    DrawCube({ 0, 0.03f, -2.72f }, 17.4f, 0.05f, 0.06f, stripB);
    for (int s = -1; s <= 1; s += 2)
        DrawCube({ s * 10.0f, 2.3f, -2.64f }, 0.14f, 4.4f, 0.02f, s < 0 ? stripA : stripB);

    // Đèn sân khấu + chùm sáng
    for (int i = 0; i < 4; ++i) {
        float x = -5.4f + i * 3.6f;
        float swing = Neon() ? std::sin(S.time * 0.8f + i * 1.3f) * 2.5f : 0.0f;
        Color c = (i % 2 == 0) ? stripA : stripB;
        if (!Neon()) c = Color{ 255, 236, 200, 255 };
        DrawSphere({ x, 6.02f, -0.2f }, 0.17f, c);
        DrawCylinderEx({ x, 6.0f, -0.2f }, { x + swing, 0.0f, 0.4f }, 0.15f, 1.5f, 16, Fade(c, Neon() ? 0.10f : 0.05f));
    }
    // Gậy phát sáng của khán giả
    for (int row = 0; row < 3; ++row)
        for (int i = 0; i < 28; i += 2) {
            Vector3 b = CrowdPos(i, row);
            float wave = std::sin(S.time * 4 + i * 0.6f + row) * 0.25f;
            Color c = ((i + row) % 3 == 0) ? stripA : ((i + row) % 3 == 1 ? stripB : pal::accent);
            DrawCube({ b.x + 0.22f + wave * 0.3f, b.y + 1.1f, b.z + 0.1f }, 0.05f, 0.32f, 0.05f, c);
        }
    // Tia lửa
    for (const auto& p : S.particles) {
        if (p.confetti) continue;
        float k = p.life / p.maxLife;
        float sz = p.size * k + 0.012f;
        DrawCube(p.pos, sz, sz, sz, Fade2(p.color, 0.5f + k * 0.5f));
    }
    // Sao trên trời đêm (chỉ sân Neon)
    gfx::EndGlow();
}

static void DrawCasters() {
    bool anim = Has(F_ANIM_STATES);
    {
        Fighter p1 = S.p1;                                   // màn chọn: xoay mặt về phía camera
        if (S.phase == Phase::Select) p1.showcaseYaw = 20.0f + std::sin(S.time * 0.8f) * 25.0f;
        DrawFighterModel(p1, anim, S.time);
    }
    if (S.phase != Phase::Select) DrawFighterModel(S.p2, anim, S.time);
    for (int s = -1; s <= 1; s += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            gfx::Cylinder({ s * 8.5f, 0, sz * 2.55f }, { s * 8.5f, 1.6f, sz * 2.55f }, 0.09f, 0.09f, WHITE, 8);
}

// Màn hình LED: vẽ chữ vào render texture
static void RenderLedScreen() {
    if (!S.screenOk) return;
    BeginTextureMode(S.screen);
    Color top = Neon() ? Color{ 40, 6, 60, 255 } : Color{ 30, 24, 70, 255 };
    Color bot = Neon() ? Color{ 6, 30, 60, 255 } : Color{ 70, 20, 60, 255 };
    DrawRectangleGradientV(0, 0, 640, 184, top, bot);
    for (int i = 0; i < 24; ++i) {                    // cột nhạc nhảy múa
        float h = 20 + std::fabs(std::sin(S.time * 3 + i * 0.7f)) * 50;
        DrawRectangle(i * 27 + 4, (int)(184 - h), 20, (int)h, Fade(i % 2 ? pal::accent2 : pal::purple, 0.35f));
    }
    std::string title = "CODE VÕ ĐÀI";
    ui::TextCentered(title, 320, 18, 64, pal::accent, FS_BOLD);
    std::string sub;
    if (S.phase == Phase::Select) sub = Neon() ? "SÂN KHẤU NEON" : "CHỌN VÕ SĨ";
    else sub = std::string(GetCharacterStats(S.p1.type).name) + "  VS  " + GetCharacterStats(S.p2.type).name;
    ui::TextCentered(sub, 320, 100, 34, WHITE, FS_BOLD);
    if (S.phase != Phase::Select && S.rules.roundsToWin > 1)
        ui::TextCentered(TextFormat("HIỆP %d", S.round), 320, 142, 24, pal::accent2, FS_BOLD);
    for (int y = 0; y < 184; y += 3) DrawRectangle(0, y, 640, 1, Color{ 0, 0, 0, 60 });   // vạch quét
    EndTextureMode();
}

void arena::Init() {
    S.screen = LoadRenderTexture(640, 184);
    S.screenOk = S.screen.id != 0;
    if (S.screenOk) SetTextureFilter(S.screen.texture, TEXTURE_FILTER_BILINEAR);
}

void arena::Shutdown() {
    if (S.screenOk) UnloadRenderTexture(S.screen);
}

void arena::RenderOffscreen() {
    SetupEnvironment();
    RenderLedScreen();
    gfx::RenderShadowMap(DrawCasters);
}

void arena::Draw3D() {
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    if (Neon()) {
        DrawRectangleGradientV(0, 0, (int)W, (int)H, Color{ 4, 4, 16, 255 }, Color{ 34, 8, 44, 255 });
        for (int i = 0; i < 90; ++i) {           // sao
            float x = std::fmod(i * 137.5f, W), y = std::fmod(i * 71.3f, H * 0.45f);
            float tw = 0.5f + 0.5f * std::sin(S.time * 2 + i);
            DrawCircle((int)x, (int)y, 1.2f + (i % 3) * 0.5f, Fade(WHITE, 0.3f + 0.5f * tw));
        }
    } else {
        DrawRectangleGradientV(0, 0, (int)W, (int)H, Color{ 22, 20, 46, 255 }, Color{ 72, 38, 82, 255 });
    }

    BeginMode3D(S.cam);
    gfx::BeginLit(S.cam);
    DrawStageSolid();
    bool anim = Has(F_ANIM_STATES);
    {
        Fighter p1 = S.p1;                                   // màn chọn: xoay mặt về phía camera
        if (S.phase == Phase::Select) p1.showcaseYaw = 20.0f + std::sin(S.time * 0.8f) * 25.0f;
        DrawFighterModel(p1, anim, S.time);
    }
    if (S.phase != Phase::Select) DrawFighterModel(S.p2, anim, S.time);
    gfx::EndLit();
    DrawStageGlow();
    if (S.showHitbox && S.phase != Phase::Select) {
        for (const Fighter* f : { &S.p1, &S.p2 }) {
            DrawBoundingBox(HurtBox(*f), GREEN);
            if (f->state == FState::Punch || f->state == FState::Kick)
                DrawBoundingBox(AttackBox(*f), AttackActive(*f) ? RED : Color{ 120, 40, 40, 255 });
        }
    }
    EndMode3D();

    gfx::SetFlash(S.flash);
    gfx::SetAberration(S.slowMo > 0 ? 0.8f : S.shake * 0.5f);
}
// ---- HUD --------------------------------------------------------------------
static void DrawHpBar(const Fighter& f, float trail, bool left, float s) {
    float W = (float)GetScreenWidth();
    float bw = W * 0.36f, bh = 22 * s, y = 22 * s;
    float x = left ? 24 * s : W - 24 * s - bw;
    float pct = Clamp01(pcode::TiLeMau((int)std::ceil(f.hp), (int)f.maxHp, f.hp / f.maxHp));   // Bài 1: code người chơi
    DrawRectangleRec({ x - 3, y - 3, bw + 6, bh + 6 }, Color{ 0, 0, 0, 160 });
    DrawRectangleRec({ x, y, bw, bh }, Color{ 60, 20, 24, 255 });
    float tw = bw * trail, pw = bw * pct;
    Color hc = pct > 0.5f ? pal::good : (pct > 0.25f ? pal::accent : pal::bad);
    if (left) {
        DrawRectangleRec({ x + bw - tw, y, tw, bh }, Color{ 255, 255, 255, 200 });
        DrawRectangleRec({ x + bw - pw, y, pw, bh }, hc);
    } else {
        DrawRectangleRec({ x, y, tw, bh }, Color{ 255, 255, 255, 200 });
        DrawRectangleRec({ x, y, pw, bh }, hc);
    }
    const char* name = GetCharacterStats(f.type).name;
    std::string label = f.isPlayer ? std::string("BẠN · ") + name : std::string(name);
    if (left) ui::TextShadow(label, x, y + bh + 6 * s, 20 * s, pal::text);
    else ui::TextShadow(label, x + bw - ui::Measure(label, 20 * s, FS_BOLD).x, y + bh + 6 * s, 20 * s, pal::text);
    ui::Text(TextFormat("%d/%d", (int)std::ceil(f.hp), (int)f.maxHp), left ? x + 6 * s : x + bw - 70 * s, y + 2 * s, 17 * s, Color{ 0, 0, 0, 200 }, FS_BOLD);

    if (S.rules.roundsToWin > 1) {
        for (int i = 0; i < S.rules.roundsToWin; ++i) {
            float cx = left ? x + bw - 12 * s - i * 26 * s : x + 12 * s + i * 26 * s;
            DrawCircle((int)cx, (int)(y + bh + 18 * s), 8 * s, Color{ 0, 0, 0, 160 });
            if (i < f.roundsWon) DrawCircle((int)cx, (int)(y + bh + 18 * s), 6 * s, pal::accent);
        }
    }
}

static void DrawBigText(const std::string& t, float size, Color c, float yFrac) {
    float s = ui::S();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    Vector2 m = ui::Measure(t, size * s, FS_BOLD);
    float x = (W - m.x) / 2, y = H * yFrac;
    for (int i = 1; i <= 4; ++i) ui::Text(t, x + i * s, y + i * s, size * s, Color{ 0, 0, 0, 90 }, FS_BOLD);
    ui::Text(t, x, y, size * s, c, FS_BOLD);
}

static void DrawSelectOverlay() {
    float s = ui::S();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    DrawRectangleGradientV(0, 0, (int)W, (int)(140 * s), Color{ 8, 8, 16, 230 }, Color{ 8, 8, 16, 0 });
    ui::TextCentered("CHỌN VÕ SĨ", W / 2, 26 * s, 40 * s, pal::accent, FS_BOLD);
    ui::TextCentered("A/D hoặc ← → để chọn,  Enter để vào trận", W / 2, 76 * s, 18 * s, pal::dim);

    int n = CharCount();
    float gap = 20 * s;
    float cw = std::min(250 * s, (W - 60 * s - gap * (n - 1)) / n), ch = 150 * s;
    float total = cw * n + gap * (n - 1);
    float x0 = (W - total) / 2, y = H - ch - (Has(F_ROUNDS) ? 120 : 64) * s;
    for (int i = 1; i <= n; ++i) {
        const CharStats& cs = GetCharacterStats(SelType(i));
        Rectangle r = { x0 + (i - 1) * (cw + gap), y, cw, ch };
        bool sel = S.selChar == i;
        ui::Panel(r, Fade(pal::panel, 0.92f), sel ? pal::accent : pal::line, 0.08f);
        if (ui::Hover(r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (S.selChar == i) StartMatch(); else S.selChar = i;
        }
        ui::Text(cs.name, r.x + 14 * s, r.y + 10 * s, 24 * s, sel ? pal::accent : pal::text, FS_BOLD);
        ui::Text(cs.desc, r.x + 14 * s, r.y + 40 * s, 15 * s, pal::dim);
        const char* labels[3] = { "Tốc độ", "Sức mạnh", "Máu" };
        float vals[3] = { cs.speed / 6.0f, (cs.punch + cs.kick) / 28.0f, cs.maxHp / 130.0f };
        for (int k = 0; k < 3; ++k) {
            float yy = r.y + 70 * s + k * 24 * s;
            ui::Text(labels[k], r.x + 14 * s, yy, 15 * s, pal::text);
            ui::Bar({ r.x + 100 * s, yy + 4 * s, cw - 120 * s, 10 * s }, vals[k], sel ? pal::accent : pal::accent2, pal::panel2);
        }
    }
    // Chọn sân khấu (Bài 15)
    {
        std::string st = Has(F_NEON_STAGE)
            ? std::string("Sân khấu: ") + (S.stage == 1 ? "◀ NEON BAN ĐÊM ▶" : "◀ SÂN ĐẤU ▶") + "   (Q/E để đổi)"
            : std::string("Học Bài 15 (Shader) để mở khóa sân khấu Neon");
        ui::TextCentered(st, W / 2, 104 * s, 18 * s, Has(F_NEON_STAGE) ? pal::accent2 : pal::dim, FS_BOLD);
    }
    if (Has(F_ROUNDS)) {
        const char* diffs[3] = { "Dễ", "Thường", "Khó" };
        float bw = 140 * s, bh = 40 * s;
        float bx = (W - (bw * 3 + gap * 2)) / 2, by = H - 90 * s;
        ui::TextCentered("Độ khó AI (W/S)", W / 2, by - 26 * s, 16 * s, pal::dim);
        for (int i = 0; i < 3; ++i) {
            Rectangle r = { bx + i * (bw + gap), by, bw, bh };
            bool sel = S.selDiff == i;
            if (ui::Button(r, diffs[i], true, sel ? pal::accent : pal::accent2)) S.selDiff = i;
            if (sel) DrawRectangleRoundedLinesEx(r, 0.25f, 6, 3, pal::accent);
        }
    }
}

static void DrawMatchOver() {
    float s = ui::S();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    DrawRectangle(0, 0, (int)W, (int)H, Color{ 0, 0, 0, 140 });
    bool win = S.p1.roundsWon > S.p2.roundsWon;
    bool draw = S.p1.roundsWon == S.p2.roundsWon;
    float pw = 560 * s, ph = (Has(F_COMBO) ? 426 : 220) * s;
    Rectangle r = { (W - pw) / 2, (H - ph) / 2, pw, ph };
    ui::Panel(r, pal::panel, win ? pal::accent : pal::line, 0.05f);
    std::string title = draw ? "HÒA!" : (win ? "BẠN THẮNG!" : "BẠN THUA RỒI");
    ui::TextCentered(title, W / 2, r.y + 20 * s, 44 * s, win ? pal::accent : pal::text, FS_BOLD);
    if (S.rules.roundsToWin > 1)
        ui::TextCentered(TextFormat("Tỉ số hiệp  %d - %d", S.p1.roundsWon, S.p2.roundsWon), W / 2, r.y + 74 * s, 20 * s, pal::dim);

    float y = r.y + 110 * s;
    if (Has(F_COMBO)) {
        ui::Text("THỐNG KÊ TRẬN (std::map)", r.x + 30 * s, y, 18 * s, pal::accent2, FS_BOLD);
        y += 32 * s;
        ui::Text("Bạn", r.x + 300 * s, y, 16 * s, pal::dim, FS_BOLD);
        ui::Text("Đối thủ", r.x + 420 * s, y, 16 * s, pal::dim, FS_BOLD);
        y += 26 * s;
        const char* keys[] = { "Số lần ra đòn", "Đấm trúng", "Đá trúng", "Đỡ đòn thành công", "Sát thương gây ra" };
        for (const char* k : keys) {
            ui::Text(k, r.x + 30 * s, y, 17 * s, pal::text);
            auto get = [&](const Fighter& f) { auto it = f.stats.find(k); return it == f.stats.end() ? 0 : it->second; };
            ui::Text(TextFormat("%d", get(S.p1)), r.x + 300 * s, y, 17 * s, pal::text, FS_MONO);
            ui::Text(TextFormat("%d", get(S.p2)), r.x + 420 * s, y, 17 * s, pal::text, FS_MONO);
            y += 26 * s;
        }
        // Bài 7: DonManhNhat() của người chơi
        auto strongest = [](const Fighter& f) {
            int m = 0;
            for (int d : f.hits) m = std::max(m, d);
            return pcode::DonManhNhat(f.hits, m);
        };
        ui::Text("Đòn mạnh nhất", r.x + 30 * s, y, 17 * s, pal::text);
        ui::Text(TextFormat("%d", strongest(S.p1)), r.x + 300 * s, y, 17 * s, pal::text, FS_MONO);
        ui::Text(TextFormat("%d", strongest(S.p2)), r.x + 420 * s, y, 17 * s, pal::text, FS_MONO);
        y += 26 * s;
        ui::Text("Combo cao nhất", r.x + 30 * s, y, 17 * s, pal::text);
        ui::Text(TextFormat("%d", S.p1.bestCombo), r.x + 300 * s, y, 17 * s, pal::accent, FS_MONO);
        ui::Text(TextFormat("%d", S.p2.bestCombo), r.x + 420 * s, y, 17 * s, pal::text, FS_MONO);
        y += 34 * s;
    } else {
        ui::TextCentered("Học Bài 7 (STL) để có bảng thống kê trận đấu.", W / 2, y, 17 * s, pal::dim);
        y += 34 * s;
    }
    float bw = 220 * s, bh = 44 * s;
    if (ui::Button({ W / 2 - bw - 10 * s, r.y + ph - bh - 18 * s, bw, bh }, "Đấu lại (Enter)")) arena::Enter();
    if (ui::Button({ W / 2 + 10 * s, r.y + ph - bh - 18 * s, bw, bh }, "Về văn phòng (Esc)", true, pal::accent2)) S.wantExit = true;
}

static std::string ControlsLine() {
    std::string c = "A/D: đi";
    if (Has(F_JUMP_FACING)) c += "   W: nhảy";
    if (Has(F_PUNCH)) c += "   J: đấm";
    if (Has(F_KICK_BLOCK)) c += "   K: đá   L: đỡ";
    if (Has(F_DASH)) c += "   Shift: lướt";
    if (Has(F_HITBOX)) c += "   H: hitbox";
    if (Has(F_ROUNDS)) c += "   P: dừng";
    c += "   Esc: thoát";
    return c;
}

void arena::DrawHUD() {
    float s = ui::S();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    if (!gfx::PostEnabled() && S.flash > 0) DrawRectangle(0, 0, (int)W, (int)H, Fade(WHITE, S.flash));

    // Số sát thương bay lên
    for (const auto& p : S.popups) {
        Vector2 sp = GetWorldToScreen(p.pos, S.cam);
        float a = Clamp01(p.life / 0.4f);
        Vector2 m = ui::Measure(p.text, p.size * s, FS_BOLD);
        ui::TextShadow(p.text, sp.x - m.x / 2, sp.y, p.size * s, Fade2(p.color, a));
    }

    if (S.phase == Phase::Select) { DrawSelectOverlay(); return; }

    // HUD
    if (Has(F_HPBAR)) {
        DrawHpBar(S.p1, S.trail1, true, s);
        DrawHpBar(S.p2, S.trail2, false, s);
        // Bài 2: SoDonDeKO() của người chơi: còn bao nhiêu cú đấm nữa?
        if (pcode::Active(F_ARENA) && Has(F_PUNCH) && S.p2.hp > 0 && S.phase == Phase::Fight) {
            int punch = (int)GetCharacterStats(S.p1.type).punch;
            int n = pcode::SoDonDeKO((int)std::ceil(S.p2.hp), punch, -1);
            if (n >= 0) {
                std::string t = TextFormat("SoDonDeKO() = %d cú đấm", n);
                ui::TextShadow(t, W - 24 * s - ui::Measure(t, 15 * s, FS_BOLD).x, 84 * s, 15 * s, pal::accent2);
            }
        }
    } else {
        ui::Text("(Chưa có thanh máu: học Bài 1)", 24 * s, 24 * s, 18 * s, pal::dim);
    }
    if (S.rules.roundTime > 0) {
        std::string tt = TextFormat("%d", (int)std::ceil(S.timer));
        ui::Panel({ W / 2 - 40 * s, 14 * s, 80 * s, 50 * s }, Fade(pal::bg, 0.85f), pal::line, 0.2f);
        ui::TextCentered(tt, W / 2, 20 * s, 34 * s, S.timer < 10 ? pal::bad : pal::text, FS_BOLD);
    }
    if (Has(F_COMBO)) {
        if (S.p1.combo >= 2) ui::TextShadow(TextFormat("%d HIT COMBO!", S.p1.combo), 24 * s, 110 * s, 30 * s, pal::accent);
        if (S.p2.combo >= 2) {
            std::string c = TextFormat("%d HIT COMBO!", S.p2.combo);
            ui::TextShadow(c, W - 24 * s - ui::Measure(c, 30 * s, FS_BOLD).x, 110 * s, 30 * s, pal::accent2);
        }
    }

    if (S.phase == Phase::Intro) {
        if (S.phaseTime < 0.9f) DrawBigText(S.rules.roundsToWin > 1 ? TextFormat("HIỆP %d", S.round) : "SẴN SÀNG", 64, pal::text, 0.32f);
        else DrawBigText("ĐẤU!", 92, pal::accent, 0.30f);
    }
    if (S.phase == Phase::RoundOver) {
        std::string t = S.timeUp ? "HẾT GIỜ!" : "K.O.!";
        if (S.p2.type == CharType::Dummy && S.roundWinner == 1) t = "BAO CÁT RÁCH!";
        DrawBigText(t, 84, pal::accent, 0.28f);
        if (S.phaseTime > 0.8f) {
            std::string w = S.roundWinner == 1 ? "Bạn thắng hiệp này" : (S.roundWinner == 2 ? "Đối thủ thắng hiệp này" : "Hòa");
            DrawBigText(w, 30, pal::text, 0.45f);
        }
    }

    // Huy hiệu: game đang chạy bao nhiêu hàm do người chơi viết
    if (pcode::ActiveCount() > 0) {
        std::string b = TextFormat("</> Game đang chạy %d hàm do BẠN viết", pcode::ActiveCount());
        Vector2 m = ui::Measure(b, 15 * s, FS_BOLD);
        Rectangle r = { 16 * s, H - 78 * s, m.x + 24 * s, 30 * s };
        DrawRectangleRounded(r, 0.4f, 6, Color{ 10, 40, 24, 210 });
        DrawRectangleRoundedLinesEx(r, 0.4f, 6, 1.5f, pal::good);
        ui::Text(b, r.x + 12 * s, r.y + 6 * s, 15 * s, pal::good, FS_BOLD);
    }

    // Thanh hướng dẫn dưới cùng
    DrawRectangle(0, (int)(H - 40 * s), (int)W, (int)(40 * s), Color{ 0, 0, 0, 150 });
    ui::Text(ControlsLine(), 18 * s, H - 30 * s, 17 * s, pal::text);
    if (g_progress.completed < F_COUNT) {
        const Lesson& next = GetLessons()[g_progress.completed];
        std::string n = TextFormat("Bài %d sẽ thêm: %s", g_progress.completed + 1, next.unlock.c_str());
        Vector2 m = ui::Measure(n, 15 * s);
        if (m.x < W * 0.45f) ui::Text(n, W - m.x - 18 * s, H - 29 * s, 15 * s, pal::accent);
    }
    if (!Has(F_AI) && S.phase == Phase::Fight)
        ui::TextCentered("Đối thủ chỉ là bao cát. Học Bài 12 (AI) để nó biết đánh trả!", W / 2, H - 70 * s, 16 * s, pal::dim);

    if (S.hintTime > 0 && !S.hint.empty()) {
        float a = Clamp01(S.hintTime / 0.4f);
        Vector2 m = ui::Measure(S.hint, 19 * s, FS_BOLD);
        Rectangle r = { (W - m.x) / 2 - 16 * s, H * 0.62f, m.x + 32 * s, 40 * s };
        DrawRectangleRounded(r, 0.3f, 6, Fade2(Color{ 60, 20, 24, 235 }, a));
        DrawRectangleRoundedLinesEx(r, 0.3f, 6, 1.5f, Fade2(pal::bad, a));
        ui::Text(S.hint, r.x + 16 * s, r.y + 9 * s, 19 * s, Fade2(pal::text, a), FS_BOLD);
    }

    if (S.paused) {
        DrawRectangle(0, 0, (int)W, (int)H, Color{ 0, 0, 0, 150 });
        DrawBigText("TẠM DỪNG", 64, pal::text, 0.38f);
        ui::TextCentered("Nhấn P để tiếp tục, Esc để về văn phòng", W / 2, H * 0.52f, 20 * s, pal::dim);
    }
    if (S.phase == Phase::MatchOver && S.phaseTime >= 0) DrawMatchOver();
}
