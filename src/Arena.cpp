// =============================================================================
//  Arena.cpp — VÕ ĐÀI: game đối kháng 3D "Code Võ Đài".
//
//  Đọc file này sau khi học xong các bài: mọi khái niệm trong bài học đều
//  xuất hiện ở đây (FSM, delta time, AABB, AI, camera, particles...).
//  Các dòng if (g_progress.Has(F_...)) là "công tắc" mở khóa tính năng.
// =============================================================================
#include "Arena.h"
#include "Common.h"
#include "Lessons.h"
#include "UI.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
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

const CharStats& GetCharacterStats(CharType t) { return CHAR_STATS[(int)t]; }

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

    struct Particle { Vector3 pos, vel; float life, maxLife, size; Color color; };
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
static void ResetFighter(Fighter& f, CharType t, bool player) {
    int won = f.roundsWon;
    auto stats = f.stats;
    int best = f.bestCombo;
    f = Fighter{};
    f.type = t;
    f.isPlayer = player;
    f.maxHp = f.hp = GetCharacterStats(t).maxHp;
    f.pos = { player ? -2.2f : 2.2f, 0, 0 };
    f.facing = player ? 1.0f : -1.0f;
    f.roundsWon = won;
    f.stats = stats;
    f.bestCombo = best;
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
    CharType me = Has(F_CHARSELECT) ? (CharType)S.selChar : CharType::Dev;
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
        S.p1.type = (CharType)S.selChar;
        S.p1.isPlayer = true;
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
        if (foeAttacking && dist < 2.2f && Rand01() < BLOCK_P[d]) { ai.mode = AIMode::Defend; ai.blockHold = 0.45f; }
        else if (dist > 1.5f) ai.mode = (Rand01() < 0.85f) ? AIMode::Approach : AIMode::Retreat;
        else ai.mode = (Rand01() < AGGRO[d]) ? AIMode::Attack : AIMode::Retreat;
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
    if (f.state == FState::Punch) return f.stateTime >= PUNCH_ON && f.stateTime < PUNCH_OFF;
    if (f.state == FState::Kick)  return f.stateTime >= KICK_ON && f.stateTime < KICK_OFF;
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
        p.vel.y += GRAVITY * 0.5f * dt;
        p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
        p.life -= dt;
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
    float dmg = kick ? cs.kick : cs.punch;

    if (Has(F_COMBO)) {
        a.combo = (a.comboTimer > 0) ? a.combo + 1 : 1;
        a.comboTimer = 1.2f;
        dmg *= 1.0f + 0.1f * (a.combo - 1);
        a.bestCombo = std::max(a.bestCombo, a.combo);
    }

    bool blocked = (b.state == FState::Block);
    if (blocked) {
        dmg *= 0.25f;
        b.stats["Đỡ đòn thành công"]++;
    }

    b.hp = std::max(0.0f, b.hp - dmg);
    a.stats[kick ? "Đá trúng" : "Đấm trúng"]++;
    a.stats["Sát thương gây ra"] += (int)std::round(dmg);

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
                         blocked ? "ĐỠ!" : TextFormat("-%d", (int)std::round(dmg)),
                         0.9f, blocked ? pal::accent2 : (kick ? pal::accent : WHITE), blocked ? 26.0f : 30.0f });

    if (Has(F_PARTICLES))
        SpawnSparks(contact, blocked ? 12 : (kick ? 34 : 24), blocked ? pal::accent2 : pal::accent, dir);

    if (Has(F_CAMERA_FX)) {
        S.hitStop = ko ? 0.25f : (kick ? 0.10f : 0.06f);
        S.shake   = ko ? 0.60f : (blocked ? 0.10f : (kick ? 0.40f : 0.22f));
    }
}

static void CheckAttack(Fighter& a, Fighter& b) {
    if (a.hitDone || b.state == FState::KO || !AttackActive(a)) return;
    bool kick = (a.state == FState::Kick);
    bool hit;
    if (Has(F_HITBOX)) {
        hit = CheckCollisionBoxes(AttackBox(a), HurtBox(b));
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
        } else if (in.kick) {
            f.ChangeState(FState::Kick);
            f.stats["Số lần ra đòn"]++;
        } else {
            FState next = (in.moveX != 0) ? FState::Walk : FState::Idle;
            if (next != f.state) f.ChangeState(next);
            if (in.jump && f.onGround) { f.vel.y = JUMP_VEL; f.onGround = false; }
            if (in.dash && f.dashCooldown <= 0 && in.moveX != 0) {
                f.dashTimer = 0.18f;
                f.dashCooldown = 0.8f;
                f.dashDir = (float)in.moveX;
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
    float mx = 0;
    if (f.state == FState::Walk) mx = in.moveX * cs.speed;
    if (!f.onGround && (f.state == FState::Punch || f.state == FState::Kick)) mx = in.moveX * cs.speed * 0.6f;
    if (f.dashTimer > 0) {
        mx = f.dashDir * 18.0f;
        f.dashTimer -= dt;
    }
    f.pos.x += (mx + f.vel.x) * dt;
    f.vel.x = Approach(f.vel.x, 0, (f.onGround ? 14.0f : 4.0f) * dt);   // knockback giảm dần

    // Trọng lực (Bài 9)
    if (!f.onGround || f.vel.y > 0) {
        f.vel.y += GRAVITY * dt;
        f.pos.y += f.vel.y * dt;
        if (f.pos.y <= 0) { f.pos.y = 0; f.vel.y = 0; f.onGround = true; }
    }
    f.pos.x = Clamp(f.pos.x, -ARENA_HALF, ARENA_HALF);

    if (f.state == FState::Walk) f.walkPhase += cs.speed * dt * 2.2f;
}

// -----------------------------------------------------------------------------
// Camera (Bài 13)
// -----------------------------------------------------------------------------
static void UpdateCam(float dt) {
    if (Has(F_CAMERA_FX) && S.phase != Phase::Select) {
        float mid = (S.p1.pos.x + S.p2.pos.x) * 0.5f;
        float dist = std::fabs(S.p1.pos.x - S.p2.pos.x);
        Vector3 wantPos = { mid * 0.85f, 3.0f + dist * 0.08f, 8.0f + dist * 0.55f };
        Vector3 wantTarget = { mid * 0.9f, 1.3f, 0 };
        float k = std::min(1.0f, 4.0f * dt);
        S.camPos = Vector3Lerp(S.camPos, wantPos, k);
        S.camTarget = Vector3Lerp(S.camTarget, wantTarget, k);
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
    if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))  S.selChar = (S.selChar == 1) ? 3 : S.selChar - 1;
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) S.selChar = (S.selChar == 3) ? 1 : S.selChar + 1;
    if (Has(F_ROUNDS)) {
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))   S.selDiff = std::max(0, S.selDiff - 1);
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) S.selDiff = std::min(2, S.selDiff + 1);
    }
    S.p1.type = (CharType)S.selChar;
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
        UpdateParticles(dt);
        UpdateCam(dt);
        if (IsKeyPressed(KEY_ENTER)) arena::Enter();
        return S.wantExit;
    }

    // Hit-stop: logic dừng lại vài khung hình (Bài 13)
    float gdt = dt;
    if (S.hitStop > 0) { S.hitStop -= dt; gdt = 0; }

    S.phaseTime += gdt;

    Input in1, in2;
    if (S.phase == Phase::Fight) {
        in1 = ReadPlayerInput();
        if (Has(F_AI) && S.p2.type != CharType::Dummy) in2 = AIUpdate(S.ai, S.p2, S.p1, gdt);
    }
    UpdateFighter(S.p1, in1, S.p2, gdt);
    UpdateFighter(S.p2, in2, S.p1, gdt);

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
                S.roundWinner = (a > b) ? 1 : (b > a ? 2 : 0);
                S.phase = Phase::RoundOver; S.phaseTime = 0;
            }
        }
        if (ko1 || ko2) {
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
                if (S.p1.roundsWon > S.p2.roundsWon) { g_progress.wins++; g_progress.Save(); }
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
static Color Darken(Color c, float k = 0.55f) {
    return Color{ (unsigned char)(c.r * k), (unsigned char)(c.g * k), (unsigned char)(c.b * k), c.a };
}

static void Part(Vector3 c, Vector3 s, Color col) {
    DrawCubeV(c, s, col);
    DrawCubeWiresV(c, s, Darken(col));
}

// Tay/chân: một khối hộp xoay quanh khớp (vai/hông)
static void Limb(Vector3 pivot, float angleDeg, float len, float w, Color c, Color endCol, bool arm) {
    rlPushMatrix();
    rlTranslatef(pivot.x, pivot.y, pivot.z);
    rlRotatef(angleDeg, 0, 0, 1);
    Part({ 0, -len / 2, 0 }, { w, len, w }, c);
    if (arm) DrawSphereEx({ 0, -len, 0 }, w * 0.72f, 8, 8, endCol);
    else     Part({ 0.07f, -len + 0.04f, 0 }, { w * 1.5f, 0.1f, w * 1.1f }, endCol);
    rlPopMatrix();
}

static void DrawDummy(const Fighter& f, float time) {
    const CharStats& cs = GetCharacterStats(CharType::Dummy);
    Color bag = Lerp(cs.body, WHITE, f.flash * 0.7f);
    Part({ 0, 0.06f, 0 }, { 0.9f, 0.12f, 0.9f }, Color{ 60, 60, 70, 255 });
    float wob = std::sin(time * 22.0f) * f.flash * 14.0f;
    rlPushMatrix();
    rlTranslatef(0, 0.12f, 0);
    if (f.state == FState::KO) rlRotatef(-std::min(1.0f, f.stateTime * 3) * 80.0f * (f.facing > 0 ? 1 : -1), 0, 0, 1);
    rlRotatef(wob, 0, 0, 1);
    DrawCylinder({ 0, 0, 0 }, 0.42f, 0.46f, 1.7f, 14, bag);
    DrawCylinderWires({ 0, 0, 0 }, 0.42f, 0.46f, 1.7f, 14, Darken(bag));
    DrawCylinder({ 0, 0.35f, 0 }, 0.45f, 0.45f, 0.08f, 14, cs.accent);
    DrawCylinder({ 0, 1.3f, 0 }, 0.43f, 0.43f, 0.08f, 14, cs.accent);
    DrawSphereEx({ -0.12f, 1.45f, 0.38f }, 0.05f, 6, 6, BLACK);
    DrawSphereEx({ 0.12f, 1.45f, 0.38f }, 0.05f, 6, 6, BLACK);
    rlPopMatrix();
}

void DrawFighterModel(const Fighter& f, bool animate, float time) {
    // Bóng đổ giả dưới chân
    float sh = 0.55f - std::min(f.pos.y, 2.0f) * 0.12f;
    DrawCylinder({ f.pos.x, 0.012f, f.pos.z }, sh, sh, 0.01f, 16, Color{ 0, 0, 0, 90 });

    rlPushMatrix();
    rlTranslatef(f.pos.x, f.pos.y, f.pos.z);
    if (f.type == CharType::Dummy) { DrawDummy(f, time); rlPopMatrix(); return; }
    rlRotatef(f.facing > 0 ? 0.0f : 180.0f, 0, 1, 0);

    const CharStats& cs = GetCharacterStats(f.type);
    float fl = f.flash * 0.75f;
    Color body = Lerp(cs.body, WHITE, fl), acc = Lerp(cs.accent, WHITE, fl), skin = Lerp(cs.skin, WHITE, fl);
    Color pants = Lerp(Darken(cs.body, 0.6f), WHITE, fl);

    // Góc khớp (độ). Góc dương = vung về phía trước.
    float armF = 40, armB = 40, legF = 8, legB = -8, lean = 0, bob = 0;
    float t = f.stateTime;
    const float PI_F = 3.14159265f;
    if (animate) {
        switch (f.state) {
        case FState::Idle:
            bob = std::sin(time * 4.0f) * 0.025f;
            armF = 45 + std::sin(time * 4.0f) * 5; armB = 35 - std::sin(time * 4.0f) * 5;
            break;
        case FState::Walk: {
            float s = std::sin(f.walkPhase);
            legF = s * 32; legB = -s * 32;
            armF = 42 - s * 12; armB = 42 + s * 12;
            bob = std::fabs(s) * 0.05f;
            break;
        }
        case FState::Punch: {
            float k = std::sin(Clamp01(t / PUNCH_TIME) * PI_F);
            armF = 40 + k * 55; armB = 50; lean = -k * 8;
            break;
        }
        case FState::Kick: {
            float k = std::sin(Clamp01(t / KICK_TIME) * PI_F);
            legF = 8 + k * 92; legB = -8 - k * 10; lean = k * 14; armF = 30; armB = 55;
            break;
        }
        case FState::Block: armF = 115; armB = 105; lean = 6; break;
        case FState::Hurt:  lean = 22 * (1 - Clamp01(t / HURT_TIME)); armF = 10; armB = 15; break;
        case FState::KO:    lean = std::min(1.0f, t * 3) * 82; armF = 160; armB = 150; legF = 20; break;
        case FState::Victory:
            armF = 165; armB = 170; bob = std::fabs(std::sin(time * 6)) * 0.12f;
            break;
        }
    } else {
        // Chưa có hoạt ảnh (trước Bài 10): tư thế cứng đơ, chỉ "giật" tay/chân
        if (f.state == FState::Punch) armF = 90;
        if (f.state == FState::Kick) legF = 90;
        if (f.state == FState::Block) armF = armB = 110;
        if (f.state == FState::KO) lean = 85;
    }

    if (f.casual && (f.state == FState::Idle || f.state == FState::Walk)) {
        float s = (f.state == FState::Walk) ? std::sin(f.walkPhase) : 0.0f;
        armF = 4 - s * 26; armB = 4 + s * 26;
    }

    rlTranslatef(0, bob, 0);
    rlRotatef(lean, 0, 0, 1);

    // Chân
    Limb({ 0, 0.9f, 0.14f },  legF, 0.86f, 0.22f, pants, Darken(acc, 0.8f), false);
    Limb({ 0, 0.9f, -0.14f }, legB, 0.86f, 0.22f, pants, Darken(acc, 0.8f), false);
    // Thân + đai
    Part({ 0, 1.27f, 0 }, { 0.40f, 0.74f, 0.60f }, body);
    Part({ 0, 0.94f, 0 }, { 0.42f, 0.09f, 0.62f }, acc);
    // Đầu
    if (f.type == CharType::Robot) {
        Part({ 0, 1.84f, 0 }, { 0.38f, 0.36f, 0.38f }, skin);
        Part({ 0.19f, 1.86f, 0 }, { 0.03f, 0.08f, 0.30f }, acc);
        DrawCylinder({ 0, 2.02f, 0 }, 0.015f, 0.015f, 0.18f, 6, DARKGRAY);
        DrawSphereEx({ 0, 2.22f, 0 }, 0.05f, 6, 6, acc);
    } else {
        DrawSphereEx({ 0, 1.84f, 0 }, 0.21f, 10, 12, skin);
        if (f.type == CharType::Dev) {
            Part({ -0.02f, 2.0f, 0 }, { 0.38f, 0.1f, 0.42f }, Color{ 50, 34, 26, 255 });        // tóc
            Part({ 0.2f, 1.87f, 0.08f }, { 0.03f, 0.08f, 0.11f }, Color{ 20, 20, 24, 255 });    // kính
            Part({ 0.2f, 1.87f, -0.08f }, { 0.03f, 0.08f, 0.11f }, Color{ 20, 20, 24, 255 });
            Part({ -0.17f, 1.62f, 0 }, { 0.10f, 0.16f, 0.50f }, Darken(body, 0.8f));            // mũ áo hoodie
        } else {
            DrawSphereEx({ 0.19f, 1.88f, 0.07f }, 0.025f, 5, 5, BLACK);
            DrawSphereEx({ 0.19f, 1.88f, -0.07f }, 0.025f, 5, 5, BLACK);
        }
        if (f.type == CharType::Ninja) {
            Part({ 0.02f, 1.76f, 0 }, { 0.40f, 0.13f, 0.43f }, body);                          // khăn che mặt
            Part({ 0, 1.95f, 0 }, { 0.43f, 0.07f, 0.45f }, acc);                                // băng đô
            Part({ -0.32f, 1.93f, 0.05f }, { 0.26f, 0.05f, 0.06f }, acc);
        }
        if (f.type == CharType::Samurai) {
            Part({ 0, 2.0f, 0 }, { 0.46f, 0.13f, 0.48f }, acc);                                 // mũ giáp
            Part({ 0.05f, 2.14f, 0 }, { 0.05f, 0.18f, 0.32f }, acc);
            Part({ 0, 1.63f, 0.36f }, { 0.32f, 0.08f, 0.24f }, acc);                            // giáp vai
            Part({ 0, 1.63f, -0.36f }, { 0.32f, 0.08f, 0.24f }, acc);
        }
    }
    // Tay
    Limb({ 0, 1.56f, 0.40f },  armF, 0.70f, 0.17f, body, f.type == CharType::Robot ? acc : skin, true);
    Limb({ 0, 1.56f, -0.40f }, armB, 0.70f, 0.17f, body, f.type == CharType::Robot ? acc : skin, true);

    rlPopMatrix();
}

static void DrawStage() {
    // Sàn đấu
    Part({ 0, -0.3f, 0 }, { 17.0f, 0.6f, 5.0f }, Color{ 38, 42, 58, 255 });
    for (int x = -8; x < 8; ++x)
        for (int z = -2; z < 2; ++z) {
            Color c = ((x + z) & 1) ? Color{ 52, 58, 80, 255 } : Color{ 46, 51, 70, 255 };
            DrawCube({ x + 0.5f, 0.0f, z + 0.5f }, 0.98f, 0.02f, 0.98f, c);
        }
    // Viền đèn neon
    DrawCube({ 0, 0.02f, 2.45f }, 17.0f, 0.04f, 0.08f, pal::accent);
    DrawCube({ 0, 0.02f, -2.45f }, 17.0f, 0.04f, 0.08f, pal::accent2);
    // Cột hai bên
    for (int s = -1; s <= 1; s += 2) {
        Part({ s * 9.0f, 2.0f, -2.0f }, { 0.6f, 4.6f, 0.6f }, Color{ 30, 34, 48, 255 });
        DrawCube({ s * 9.0f, 2.0f, -1.68f }, 0.12f, 4.4f, 0.04f, s < 0 ? pal::accent : pal::accent2);
    }
    // Khán giả (khối hộp nhún nhảy)
    for (int row = 0; row < 3; ++row) {
        Part({ 0, row * 0.6f + 0.3f, -4.5f - row * 1.2f }, { 22.0f, 0.6f + row * 1.2f, 1.2f }, Color{ 28, 30, 42, 255 });
        for (int i = 0; i < 26; ++i) {
            float x = -11.0f + i * 0.85f + (row % 2) * 0.4f;
            float jump = std::fabs(std::sin(S.time * (3.0f + (i % 5)) + i * 1.7f + row)) * 0.12f;
            unsigned char r = (unsigned char)(80 + (i * 37 + row * 53) % 150);
            unsigned char g = (unsigned char)(70 + (i * 71 + row * 29) % 120);
            unsigned char b = (unsigned char)(90 + (i * 13 + row * 97) % 140);
            Vector3 base = { x, row * 1.2f + 0.6f + jump, -4.5f - row * 1.2f };
            DrawCube({ base.x, base.y + 0.3f, base.z }, 0.4f, 0.6f, 0.3f, Color{ r, g, b, 255 });
            DrawSphereEx({ base.x, base.y + 0.78f, base.z }, 0.16f, 5, 6, Color{ 220, 185, 150, 255 });
        }
    }
    // Màn hình lớn phía sau
    Part({ 0, 6.4f, -9.0f }, { 9.0f, 2.6f, 0.3f }, Color{ 20, 22, 32, 255 });
    DrawCube({ 0, 6.4f, -8.82f }, 8.6f, 2.2f, 0.02f, Color{ 40, 26, 60, 255 });
}

// ---- HUD --------------------------------------------------------------------
static void DrawHpBar(const Fighter& f, float trail, bool left, float s) {
    float W = (float)GetScreenWidth();
    float bw = W * 0.36f, bh = 22 * s, y = 22 * s;
    float x = left ? 24 * s : W - 24 * s - bw;
    float pct = f.hp / f.maxHp;
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
    ui::TextCentered("CHỌN VÕ SĨ", W / 2, 26 * s, 40 * s, pal::accent, FS_BOLD);
    ui::TextCentered("A/D hoặc ← → để chọn,  Enter để vào trận", W / 2, 76 * s, 18 * s, pal::dim);

    float cw = 250 * s, ch = 150 * s, gap = 20 * s;
    float total = cw * 3 + gap * 2;
    float x0 = (W - total) / 2, y = H - ch - (Has(F_ROUNDS) ? 120 : 64) * s;
    for (int i = 1; i <= 3; ++i) {
        const CharStats& cs = GetCharacterStats((CharType)i);
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
    float pw = 560 * s, ph = (Has(F_COMBO) ? 400 : 220) * s;
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

void arena::Draw() {
    float s = ui::S();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    DrawRectangleGradientV(0, 0, (int)W, (int)H, Color{ 16, 16, 34, 255 }, Color{ 58, 30, 66, 255 });

    BeginMode3D(S.cam);
    DrawStage();
    bool anim = Has(F_ANIM_STATES);
    DrawFighterModel(S.p1, anim, S.time);
    if (S.phase != Phase::Select) DrawFighterModel(S.p2, anim, S.time);
    for (const auto& p : S.particles) {
        float k = p.life / p.maxLife;
        DrawCube(p.pos, p.size * k + 0.01f, p.size * k + 0.01f, p.size * k + 0.01f, Fade2(p.color, 0.4f + k * 0.6f));
    }
    if (S.showHitbox && S.phase != Phase::Select) {
        for (const Fighter* f : { &S.p1, &S.p2 }) {
            DrawBoundingBox(HurtBox(*f), GREEN);
            if (f->state == FState::Punch || f->state == FState::Kick)
                DrawBoundingBox(AttackBox(*f), AttackActive(*f) ? RED : Color{ 120, 40, 40, 255 });
        }
    }
    EndMode3D();

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
