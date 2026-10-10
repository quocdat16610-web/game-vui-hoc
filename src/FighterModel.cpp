// =============================================================================
//  FighterModel.cpp — dựng và tạo hoạt ảnh cho võ sĩ từ các khối hình cơ bản.
//
//  Mỗi tay/chân có 2 đoạn (bắp tay + cẳng tay, đùi + cẳng chân) nối bằng khớp.
//  Hoạt ảnh = tính GÓC của từng khớp theo trạng thái (FSM, Bài 10) và
//  stateTime, rồi dùng rlPushMatrix/rlRotatef để xoay (ma trận biến đổi).
// =============================================================================
#include "Arena.h"
#include "Common.h"
#include "Gfx.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>

namespace {
    const float PUNCH_TIME = 0.35f, KICK_TIME = 0.55f, HURT_TIME = 0.32f;

    // Góc các khớp (độ). Góc dương = xoay về phía trước mặt.
    struct Pose {
        float armF1 = 35, armF2 = 95;      // tay trước: vai, khuỷu
        float armB1 = 25, armB2 = 105;     // tay sau
        float legF1 = 12, legF2 = -14;     // chân trước: hông, đầu gối (âm = gập ra sau)
        float legB1 = -14, legB2 = -10;
        float lean = 0, bob = 0;           // nghiêng người, nhún
    };

    Color Shade(Color c, float k) {
        return Color{ (unsigned char)std::min(255.0f, c.r * k), (unsigned char)std::min(255.0f, c.g * k),
                      (unsigned char)std::min(255.0f, c.b * k), c.a };
    }

    // Tay hoặc chân 2 đoạn
    void Limb2(Vector3 pivot, float a1, float a2, float l1, float l2, float r1, float r2,
               Color c1, Color c2, bool arm, Color endCol) {
        rlPushMatrix();
        rlTranslatef(pivot.x, pivot.y, pivot.z);
        rlRotatef(a1, 0, 0, 1);
        gfx::Capsule({ 0, 0, 0 }, { 0, -l1, 0 }, r1, c1, 8);
        rlTranslatef(0, -l1, 0);
        rlRotatef(a2, 0, 0, 1);
        gfx::Capsule({ 0, 0, 0 }, { 0, -l2, 0 }, r2, c2, 8);
        if (arm) {
            gfx::Sphere({ 0, -l2 - r2 * 0.5f, 0 }, r2 * 1.45f, endCol, 6, 8);          // nắm đấm / găng
        } else {
            rlTranslatef(0, -l2, 0);
            rlRotatef(-a1 - a2, 0, 0, 1);                                               // bàn chân luôn gần song song mặt đất
            gfx::Box({ 0.07f, -0.04f, 0 }, { 0.3f, 0.1f, 0.17f }, endCol);
        }
        rlPopMatrix();
    }

    Pose ComputePose(const Fighter& f, bool animate, float time) {
        Pose p;
        const float PI_F = 3.14159265f;
        float t = f.stateTime;
        if (f.casual && (f.state == FState::Idle || f.state == FState::Walk)) {
            float s = (f.state == FState::Walk) ? std::sin(f.walkPhase) : 0.0f;
            p.armF1 = -s * 28; p.armF2 = 12 + std::max(0.0f, s) * 20;
            p.armB1 = s * 28;  p.armB2 = 12 + std::max(0.0f, -s) * 20;
            p.legF1 = s * 28;  p.legF2 = -6 - 40 * std::max(0.0f, -s);
            p.legB1 = -s * 28; p.legB2 = -6 - 40 * std::max(0.0f, s);
            p.bob = (f.state == FState::Walk) ? std::fabs(std::cos(f.walkPhase)) * 0.04f : std::sin(time * 2) * 0.01f;
            return p;
        }
        if (!animate) {
            // Chưa học Bài 10: tư thế cứng, đòn đánh chỉ "giật" một cái
            if (f.state == FState::Punch) { p.armF1 = 90; p.armF2 = 0; }
            if (f.state == FState::Kick)  { p.legF1 = 90; p.legF2 = 0; }
            if (f.state == FState::Block) { p.armF1 = p.armB1 = 70; p.armF2 = p.armB2 = 120; }
            if (f.state == FState::KO) p.lean = 85;
            return p;
        }
        switch (f.state) {
        case FState::Idle: {
            float b = std::sin(time * 4.0f);
            p.bob = b * 0.02f - 0.02f;
            p.armF1 += b * 4; p.armB1 -= b * 4;
            p.legF2 -= 4 + b * 3; p.legB2 -= 4 + b * 3;
            break;
        }
        case FState::Walk: {
            float s = std::sin(f.walkPhase);
            p.legF1 = 6 + s * 30;  p.legF2 = -12 - 38 * std::max(0.0f, -s);
            p.legB1 = -6 - s * 30; p.legB2 = -12 - 38 * std::max(0.0f, s);
            p.armF1 = 35 - s * 8;  p.armB1 = 25 + s * 8;
            p.bob = std::fabs(std::cos(f.walkPhase)) * 0.05f - 0.03f;
            break;
        }
        case FState::Punch: {
            float k = std::sin(Clamp01(t / PUNCH_TIME) * PI_F);
            p.armF1 = 35 + k * 57; p.armF2 = 95 * (1 - k);        // duỗi thẳng tay
            p.armB1 = 20; p.armB2 = 115;
            p.lean = -k * 10;
            p.legF1 = 12 + k * 10; p.legB1 = -14 - k * 10;
            break;
        }
        case FState::Kick: {
            float u = Clamp01(t / KICK_TIME);
            float lift = std::sin(u * PI_F);                       // nhấc chân
            float ext = std::max(0.0f, std::sin((u - 0.15f) / 0.7f * PI_F));   // bật duỗi
            p.legF1 = 12 + lift * 88; p.legF2 = -95 * lift * (1 - ext) - 5;
            p.legB1 = -14 - lift * 8; p.legB2 = -10 - lift * 8;
            p.lean = lift * 18;
            p.armF1 = 45; p.armF2 = 100; p.armB1 = -10; p.armB2 = 60;
            break;
        }
        case FState::Block:
            p.armF1 = 75; p.armF2 = 125; p.armB1 = 65; p.armB2 = 128;
            p.legF2 = -28; p.legB2 = -26; p.bob = -0.06f; p.lean = 5;
            break;
        case FState::Hurt: {
            float k = 1 - Clamp01(t / HURT_TIME);
            p.lean = 24 * k; p.armF1 = 15; p.armF2 = 40; p.armB1 = 5; p.armB2 = 35;
            p.legF2 = -20 * k;
            break;
        }
        case FState::KO: {
            float k = std::min(1.0f, t * 3);
            p.lean = k * 84; p.armF1 = 150 * k; p.armF2 = 10; p.armB1 = 130 * k; p.armB2 = 20;
            p.legF1 = 20 * k; p.legF2 = -5; p.legB1 = 5; p.legB2 = -5;
            break;
        }
        case FState::Victory: {
            float j = std::fabs(std::sin(time * 6));
            p.armF1 = 170; p.armF2 = 15; p.armB1 = 30; p.armB2 = 100;
            p.bob = j * 0.14f;
            p.legF2 = -25 * (1 - j); p.legB2 = -25 * (1 - j);
            break;
        }
        }
        return p;
    }

    void DrawDummy(const Fighter& f, float time) {
        const CharStats& cs = GetCharacterStats(CharType::Dummy);
        Color bag = Lerp(cs.body, WHITE, f.flash * 0.7f);
        gfx::Cylinder({ 0, 0, 0 }, { 0, 0.1f, 0 }, 0.5f, 0.45f, Color{ 60, 62, 72, 255 }, 16);
        float wob = std::sin(time * 22.0f) * f.flash * 14.0f;
        rlPushMatrix();
        rlTranslatef(0, 0.1f, 0);
        if (f.state == FState::KO) rlRotatef(-std::min(1.0f, f.stateTime * 3) * 80.0f * (f.facing > 0 ? 1 : -1), 0, 0, 1);
        rlRotatef(wob, 0, 0, 1);
        gfx::Cylinder({ 0, 0, 0 }, { 0, 1.7f, 0 }, 0.44f, 0.4f, bag, 18);
        gfx::Sphere({ 0, 1.7f, 0 }, 0.4f, bag, 6, 18);
        gfx::Cylinder({ 0, 0.33f, 0 }, { 0, 0.43f, 0 }, 0.45f, 0.45f, cs.accent, 18);
        gfx::Cylinder({ 0, 1.28f, 0 }, { 0, 1.38f, 0 }, 0.42f, 0.42f, cs.accent, 18);
        gfx::Sphere({ -0.12f, 1.55f, 0.38f }, 0.05f, Color{ 20, 20, 24, 255 }, 5, 6);
        gfx::Sphere({ 0.12f, 1.55f, 0.38f }, 0.05f, Color{ 20, 20, 24, 255 }, 5, 6);
        rlPopMatrix();
    }
}

void DrawFighterModel(const Fighter& f, bool animate, float time) {
    if (!gfx::ShadowsOn()) {            // không có bóng đổ thật thì vẽ bóng tròn giả
        float sh = 0.55f - std::min(f.pos.y, 2.0f) * 0.12f;
        DrawCylinder({ f.pos.x, 0.012f, f.pos.z }, sh, sh, 0.01f, 16, Color{ 0, 0, 0, 90 });
    }

    rlPushMatrix();
    rlTranslatef(f.pos.x, f.pos.y, f.pos.z);
    if (f.type == CharType::Dummy) { DrawDummy(f, time); rlPopMatrix(); return; }
    rlRotatef(f.facing > 0 ? 0.0f : 180.0f, 0, 1, 0);

    const CharStats& cs = GetCharacterStats(f.type);
    float fl = f.flash * 0.55f;
    Color body  = Lerp(cs.body, WHITE, fl);
    Color acc   = Lerp(cs.accent, WHITE, fl);
    Color skin  = Lerp(cs.skin, WHITE, fl);
    Color pants = Lerp(Shade(cs.body, 0.6f), WHITE, fl);
    Color glove = (f.type == CharType::Robot || f.type == CharType::Dev) ? skin : acc;
    if (f.type == CharType::Samurai) glove = Shade(acc, 0.8f);
    Color shoe  = Lerp(Color{ 34, 34, 40, 255 }, WHITE, fl);
    if (f.type == CharType::Dev) shoe = Lerp(Color{ 236, 236, 240, 255 }, WHITE, fl);

    Pose p = ComputePose(f, animate, time);
    rlTranslatef(0, p.bob, 0);
    rlRotatef(p.lean, 0, 0, 1);

    // Chân (vẽ chân sau trước)
    Limb2({ 0, 0.92f, -0.13f }, p.legB1, p.legB2, 0.44f, 0.42f, 0.1f, 0.085f, pants, pants, false, shoe);
    Limb2({ 0, 0.92f, 0.13f },  p.legF1, p.legF2, 0.44f, 0.42f, 0.1f, 0.085f, pants, pants, false, shoe);

    // Thân: hông, bụng, ngực, vai
    gfx::Box({ 0, 0.94f, 0 }, { 0.32f, 0.2f, 0.48f }, pants);
    gfx::Box({ 0, 1.06f, 0 }, { 0.34f, 0.09f, 0.5f }, acc);                // đai lưng
    gfx::Box({ 0, 1.2f, 0 }, { 0.3f, 0.22f, 0.44f }, body);
    gfx::Box({ 0.01f, 1.45f, 0 }, { 0.36f, 0.32f, 0.58f }, body);           // ngực
    gfx::Sphere({ 0, 1.55f, 0.33f }, 0.12f, body, 6, 10);                   // vai
    gfx::Sphere({ 0, 1.55f, -0.33f }, 0.12f, body, 6, 10);
    gfx::Cylinder({ 0, 1.6f, 0 }, { 0, 1.72f, 0 }, 0.075f, 0.07f, skin, 10);   // cổ

    // Đầu
    if (f.type == CharType::Robot) {
        gfx::Box({ 0, 1.86f, 0 }, { 0.36f, 0.34f, 0.36f }, skin);
        gfx::Box({ 0.17f, 1.88f, 0 }, { 0.04f, 0.09f, 0.28f }, acc);         // kính chắn
        gfx::Cylinder({ 0, 2.03f, 0 }, { 0, 2.2f, 0 }, 0.015f, 0.015f, Color{ 80, 84, 96, 255 }, 6);
        gfx::Sphere({ 0, 2.22f, 0 }, 0.05f, acc, 5, 6);
        gfx::Box({ 0, 1.86f, 0.19f }, { 0.12f, 0.12f, 0.04f }, Shade(skin, 0.8f));
        gfx::Box({ 0, 1.86f, -0.19f }, { 0.12f, 0.12f, 0.04f }, Shade(skin, 0.8f));
    } else {
        gfx::Sphere({ 0.01f, 1.88f, 0 }, 0.2f, skin, 9, 14);
        gfx::Sphere({ 0.2f, 1.86f, 0 }, 0.035f, skin, 4, 6);                   // mũi
        if (f.type == CharType::Dev) {
            Color hair = { 52, 36, 28, 255 };
            gfx::Sphere({ -0.03f, 1.95f, 0 }, 0.19f, hair, 7, 12);              // tóc
            gfx::Box({ 0.07f, 2.05f, 0 }, { 0.22f, 0.07f, 0.34f }, hair);
            Color fr = { 22, 22, 26, 255 };
            gfx::Box({ 0.2f, 1.9f, 0.075f }, { 0.03f, 0.07f, 0.1f }, fr);     // kính cận
            gfx::Box({ 0.2f, 1.9f, -0.075f }, { 0.03f, 0.07f, 0.1f }, fr);
            gfx::Box({ 0.2f, 1.91f, 0 }, { 0.02f, 0.02f, 0.06f }, fr);
            gfx::Box({ -0.16f, 1.6f, 0 }, { 0.12f, 0.16f, 0.46f }, Shade(body, 0.8f));   // mũ hoodie
        } else {
            gfx::Sphere({ 0.18f, 1.91f, 0.07f }, 0.026f, Color{ 20, 20, 24, 255 }, 4, 6);
            gfx::Sphere({ 0.18f, 1.91f, -0.07f }, 0.026f, Color{ 20, 20, 24, 255 }, 4, 6);
        }
        if (f.type == CharType::Custom) {                                                 // võ sĩ của bạn: mũ lưỡi trai "coder"
            gfx::Sphere({ -0.01f, 1.95f, 0 }, 0.195f, acc, 7, 12);
            gfx::Box({ 0.17f, 1.99f, 0 }, { 0.2f, 0.035f, 0.3f }, acc);
            gfx::Box({ -0.06f, 2.12f, 0 }, { 0.08f, 0.03f, 0.08f }, body);
        }
        if (f.type == CharType::Ninja) {
            gfx::Box({ 0.03f, 1.8f, 0 }, { 0.36f, 0.14f, 0.42f }, body);                // khăn che mặt
            gfx::Sphere({ -0.03f, 1.96f, 0 }, 0.185f, body, 7, 12);                      // mũ trùm
            gfx::Box({ 0, 1.97f, 0 }, { 0.42f, 0.06f, 0.44f }, acc);                    // băng đô
            float sway = std::sin(time * 6) * 8;
            rlPushMatrix();
            rlTranslatef(-0.2f, 1.96f, 0.05f);
            rlRotatef(-20 + sway, 0, 0, 1);
            gfx::Box({ -0.14f, 0, 0 }, { 0.28f, 0.05f, 0.05f }, acc);                   // đuôi khăn bay
            rlPopMatrix();
        }
        if (f.type == CharType::Samurai) {
            gfx::Cylinder({ 0, 1.97f, 0 }, { 0, 2.06f, 0 }, 0.27f, 0.2f, acc, 14);      // mũ giáp
            gfx::Sphere({ 0, 2.06f, 0 }, 0.2f, Shade(acc, 0.85f), 6, 12);
            gfx::Box({ 0.12f, 2.2f, 0 }, { 0.04f, 0.18f, 0.32f }, acc);                 // sừng trang trí
            gfx::Box({ 0, 1.62f, 0.36f }, { 0.34f, 0.08f, 0.24f }, acc);                // giáp vai
            gfx::Box({ 0, 1.62f, -0.36f }, { 0.34f, 0.08f, 0.24f }, acc);
            gfx::Box({ 0.02f, 0.86f, 0 }, { 0.36f, 0.24f, 0.52f }, Shade(acc, 0.7f));  // váy giáp
        }
    }

    // Tay (tay sau trước, tay trước sau để đè lên thân)
    Color sleeve = (f.type == CharType::Dev) ? body : body;
    Color forearm = (f.type == CharType::Ninja || f.type == CharType::Samurai) ? body : (f.type == CharType::Robot ? Shade(body, 1.15f) : skin);
    if (f.type == CharType::Dev) forearm = body;
    Limb2({ 0, 1.55f, -0.36f }, p.armB1, p.armB2, 0.32f, 0.3f, 0.075f, 0.065f, sleeve, forearm, true, glove);
    Limb2({ 0, 1.55f, 0.36f },  p.armF1, p.armF2, 0.32f, 0.3f, 0.075f, 0.065f, sleeve, forearm, true, glove);

    rlPopMatrix();
}
