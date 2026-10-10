// =============================================================================
//  BÀI 10 — INPUT & MÁY TRẠNG THÁI (FSM)
//  Chạy: g++ -std=c++17 10_may_trang_thai.cpp -o bai10 && ./bai10
//
//  Mô phỏng một võ sĩ nhận một chuỗi lệnh theo thời gian. Quan sát: khi đang
//  đấm thì lệnh mới bị bỏ qua (commitment), và frame data startup/active/recovery.
// =============================================================================
#include <iostream>
#include <string>

enum class State { Idle, Walk, Punch, Block, Hurt };

const char* TenState(State s) {
    switch (s) {
        case State::Idle:  return "Idle";
        case State::Walk:  return "Walk";
        case State::Punch: return "Punch";
        case State::Block: return "Block";
        case State::Hurt:  return "Hurt";
    }
    return "?";
}

// Input tách riêng: bàn phím hay AI đều tạo ra struct này
struct Input {
    int  moveX = 0;
    bool punch = false, block = false;
};

const float PUNCH_TIME = 0.35f, PUNCH_ON = 0.10f, PUNCH_OFF = 0.20f, HURT_TIME = 0.30f;

struct Fighter {
    State state = State::Idle;
    float stateTime = 0;
    float x = 0;
    void ChangeState(State s) {
        std::cout << "      -> " << TenState(state) << " => " << TenState(s) << "\n";
        state = s;
        stateTime = 0;
    }
};

bool HitboxActive(const Fighter& f) {
    return f.state == State::Punch && f.stateTime >= PUNCH_ON && f.stateTime < PUNCH_OFF;
}

void Update(Fighter& f, const Input& in, float dt) {
    f.stateTime += dt;
    switch (f.state) {
    case State::Idle:
    case State::Walk:
        if (in.punch)      f.ChangeState(State::Punch);
        else if (in.block) f.ChangeState(State::Block);
        else {
            State next = in.moveX ? State::Walk : State::Idle;
            if (next != f.state) f.ChangeState(next);
            f.x += in.moveX * 4.0f * dt;
        }
        break;
    case State::Block:
        if (!in.block) f.ChangeState(State::Idle);
        break;
    case State::Punch:                       // đang đấm: KHÓA input
        if (f.stateTime >= PUNCH_TIME) f.ChangeState(State::Idle);
        break;
    case State::Hurt:
        if (f.stateTime >= HURT_TIME) f.ChangeState(State::Idle);
        break;
    }
}

int main() {
    Fighter f;
    const float DT = 0.05f;                  // mỗi bước 0.05 giây cho dễ đọc
    for (int step = 0; step < 30; step++) {
        float t = step * DT;
        Input in;
        // "Kịch bản" bấm phím theo thời gian
        if (t < 0.3f) in.moveX = 1;          // đi sang phải
        if (step == 7) in.punch = true;      // đấm
        if (step == 9) in.punch = true;      // bấm đấm tiếp khi đang đấm -> bị bỏ qua!
        if (t >= 1.0f && t < 1.3f) in.block = true;

        std::cout << "t=" << t << "s  " << TenState(f.state)
                  << (HitboxActive(f) ? "  [HITBOX BAT]" : "") << "\n";
        Update(f, in, DT);
        if (step == 22) f.ChangeState(State::Hurt);   // giả sử bị trúng đòn
    }
    std::cout << "Vi tri cuoi: x = " << f.x << "\n";

    // BÀI TẬP: thêm State::Taunt (khiêu khích) kéo dài 1 giây.
    return 0;
}
