#include "PlayerCode.h"
#include "Audio.h"
#include "Challenges.h"
#include "Common.h"
#include "Gfx.h"
#include "MiniCpp.h"
#include "raylib.h"
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>

using namespace mcpp;

namespace {
    std::unique_ptr<Program> g_prog[F_COUNT];
    std::string g_error;

    Slot I(long long v) { return Program::MakeSlot(Value::Int(v)); }
    Slot F(double v) { return Program::MakeSlot(Value::Float(v)); }
    Slot B(bool v) { return Program::MakeSlot(Value::Bool(v)); }

    Program* P(int lesson) { return (lesson >= 0 && lesson < F_COUNT) ? g_prog[lesson].get() : nullptr; }

    // Code người chơi lỗi khi đang chạy trong game: tắt nó, báo lỗi, dùng code mẫu
    void Fail(int lesson, const Error& e) {
        char buf[512];
        std::snprintf(buf, sizeof(buf), "Code Bài %d lỗi khi chạy%s: %s. Game tạm dùng code mẫu, hãy sửa lại ở máy tính nhé!",
                      lesson + 1, e.line > 0 ? (" (dòng " + std::to_string(e.line) + ")").c_str() : "", e.msg.c_str());
        g_error = buf;
        g_prog[lesson].reset();
    }

    void ApplySideEffects(int lesson) {
        if (lesson == F_NEON_STAGE) {                              // Bài 15: shader
            Program* p = P(lesson);
            gfx::SetDiffuse(p ? pcode::LoadSource(lesson) : std::string());
        }
        if (lesson == F_MUSIC) {                                   // Bài 16: tiếng đấm
            std::vector<float> samples;
            if (pcode::MauAmThanh(samples, 22050, 0.22f)) sfx::SetCustomPunch(samples, 22050);
        }
    }
}

std::string pcode::LoadSource(int lesson) {
    std::ifstream in(FileNameForLesson(lesson), std::ios::binary);
    if (!in) return GetChallenge(lesson).starter;
    std::stringstream ss;
    ss << in.rdbuf();
    std::string s = ss.str();
    std::string out;
    for (char c : s) if (c != '\r') out += c;               // bỏ ký tự xuống dòng kiểu Windows
    return out;
}

bool pcode::HasSaved(int lesson) {
    std::ifstream in(FileNameForLesson(lesson));
    return (bool)in;
}

void pcode::SaveSource(int lesson, const std::string& src) {
    if (!DirectoryExists("code")) MakeDirectory("code");
    std::ofstream out(FileNameForLesson(lesson), std::ios::binary);
    out << src;
}

void pcode::Activate(int lesson, const std::string& src) {
    if (lesson < 0 || lesson >= F_COUNT) return;
    auto p = std::make_unique<Program>();
    std::vector<Error> errs;
    if (p->Compile(src, errs)) g_prog[lesson] = std::move(p);
    else g_prog[lesson].reset();
    ApplySideEffects(lesson);
}

void pcode::LoadAll() {
    for (int l = 0; l < F_COUNT; ++l) {
        g_prog[l].reset();
        if (!g_progress.Has((Feature)l)) continue;
        std::string src;
        if (HasSaved(l)) {
            src = LoadSource(l);
        } else {
            // Bài đã học từ phiên bản cũ (chưa có thử thách code): dùng lời giải mẫu
            src = "// Code mẫu (bạn đã học bài này trước khi game có thử thách code).\n"
                  "// Hãy mở máy tính, vào bài này để tự viết lại!\n" + GetChallenge(l).solution;
            SaveSource(l, src);
        }
        std::vector<Error> errs;
        std::vector<TestResult> res;
        std::string out;
        if (RunChallenge(l, src, errs, res, out)) {
            Activate(l, src);
        } else {
            g_error = "Code trong " + FileNameForLesson(l) + " chưa qua bài kiểm tra nên game tạm dùng code mẫu. Mở máy tính để sửa nhé!";
        }
    }
}

bool pcode::Active(int lesson) { return P(lesson) != nullptr; }

int pcode::ActiveCount() {
    int n = 0;
    for (auto& p : g_prog) if (p) n++;
    return n;
}

std::string pcode::TakeError() {
    std::string e = g_error;
    g_error.clear();
    return e;
}

// =============================================================================
//  Các hàm game gọi
// =============================================================================
#define TRY_CALL(lesson, expr, fallback)           \
    Program* p = P(lesson);                         \
    if (!p) return fallback;                        \
    try { return expr; }                            \
    catch (const Error& e) { Fail(lesson, e); return fallback; }

float pcode::TiLeMau(int mau, int mauToiDa, float fb) {
    TRY_CALL(F_HPBAR, (float)p->Call("TiLeMau", { I(mau), I(mauToiDa) }).AsFloat(), fb)
}

bool pcode::KetThucHiep(int mau1, int mau2, bool fb) {
    TRY_CALL(F_ARENA, p->Call("KetThucHiep", { I(mau1), I(mau2) }).AsBool(), fb)
}

int pcode::SoDonDeKO(int mau, int satThuong, int fb) {
    if (satThuong <= 0) return fb;
    TRY_CALL(F_ARENA, (int)p->Call("SoDonDeKO", { I(mau), I(satThuong) }).AsInt(), fb)
}

int pcode::TinhSatThuong(int lucDanh, bool biDo, int fb) {
    TRY_CALL(F_PUNCH, (int)p->Call("TinhSatThuong", { I(lucDanh), B(biDo) }).AsInt(), fb)
}

bool pcode::NhanDon(int& mau, bool& conSong, int satThuong) {
    Program* p = P(F_KICK_BLOCK);
    if (!p) return false;
    try {
        Slot obj = p->NewObject("Fighter");
        Program::Field(obj, "mau")->i = mau;
        Program::Field(obj, "conSong")->i = conSong ? 1 : 0;
        p->CallMethod(obj, "NhanDon", { I(satThuong) });
        mau = (int)Program::Field(obj, "mau")->AsInt();
        conSong = Program::Field(obj, "conSong")->AsBool();
        return true;
    } catch (const Error& e) {
        Fail(F_KICK_BLOCK, e);
        return false;
    }
}

bool pcode::VoSiCuaBan(float& tocDo, int& lucDam, int& lucDa, int& mau) {
    Program* p = P(F_CHARSELECT);
    if (!p) return false;
    try {
        Slot o = p->NewObject("VoSiCuaBan");
        tocDo = (float)p->CallMethod(o, "TocDo", {}).AsFloat();
        lucDam = (int)p->CallMethod(o, "LucDam", {}).AsInt();
        lucDa = (int)p->CallMethod(o, "LucDa", {}).AsInt();
        mau = (int)p->CallMethod(o, "MauToiDa", {}).AsInt();
        return true;
    } catch (const Error& e) {
        Fail(F_CHARSELECT, e);
        return false;
    }
}

bool pcode::CapNhatHat(float& x, float& y, float& vx, float& vy, float& life, float dt) {
    Program* p = P(F_PARTICLES);
    if (!p) return false;
    try {
        Slot obj = p->NewObject("Particle");
        Slot fx = Program::Field(obj, "x"), fy = Program::Field(obj, "y"), fvx = Program::Field(obj, "vx"),
             fvy = Program::Field(obj, "vy"), fl = Program::Field(obj, "life");
        fx->f = x; fy->f = y; fvx->f = vx; fvy->f = vy; fl->f = life;
        p->Call("CapNhatHat", { obj, F(dt) });
        x = (float)fx->f; y = (float)fy->f; vx = (float)fvx->f; vy = (float)fvy->f; life = (float)fl->f;
        return true;
    } catch (const Error& e) {
        Fail(F_PARTICLES, e);
        return false;
    }
}

int pcode::DonManhNhat(const std::vector<int>& cacDon, int fb) {
    Program* p = P(F_COMBO);
    if (!p) return fb;
    try {
        Slot v = Program::NewVector(Kind::Int);
        for (int d : cacDon) v->items.push_back(I(d));
        return (int)p->Call("DonManhNhat", { v }).AsInt();
    } catch (const Error& e) {
        Fail(F_COMBO, e);
        return fb;
    }
}

float pcode::DiChuyen(float x, int huong, float tocDo, float dt, float fb) {
    TRY_CALL(F_DASH, (float)p->Call("DiChuyen", { F(x), I(huong), F(tocDo), F(dt) }).AsFloat(), fb)
}

bool pcode::TrongLuc(float& y, float& vy, float dt) {
    Program* p = P(F_JUMP_FACING);
    if (!p) return false;
    try {
        Slot sy = F(y), svy = F(vy);
        p->Call("TrongLuc", { sy, svy, F(dt) });
        y = (float)sy->AsFloat();
        vy = (float)svy->AsFloat();
        return true;
    } catch (const Error& e) {
        Fail(F_JUMP_FACING, e);
        return false;
    }
}

bool pcode::DangGayDon(int loaiDon, float t, bool fb) {
    TRY_CALL(F_ANIM_STATES, p->Call("DangGayDon", { I(loaiDon), F(t) }).AsBool(), fb)
}

bool pcode::VaCham(float aMinX, float aMinY, float aMaxX, float aMaxY, float bMinX, float bMinY, float bMaxX, float bMaxY, bool fb) {
    Program* p = P(F_HITBOX);
    if (!p) return fb;
    try {
        Slot a = p->NewObject("Hop"), b = p->NewObject("Hop");
        Program::Field(a, "minX")->f = aMinX; Program::Field(a, "minY")->f = aMinY;
        Program::Field(a, "maxX")->f = aMaxX; Program::Field(a, "maxY")->f = aMaxY;
        Program::Field(b, "minX")->f = bMinX; Program::Field(b, "minY")->f = bMinY;
        Program::Field(b, "maxX")->f = bMaxX; Program::Field(b, "maxY")->f = bMaxY;
        return p->Call("VaCham", { a, b }).AsBool();
    } catch (const Error& e) {
        Fail(F_HITBOX, e);
        return fb;
    }
}

int pcode::ChonHanhDong(float khoangCach, bool doiThuDangDanh, float ngauNhien, int fb) {
    TRY_CALL(F_AI, (int)p->Call("ChonHanhDong", { F(khoangCach), B(doiThuDangDanh), F(ngauNhien) }).AsInt(), fb)
}

float pcode::Lerp(float a, float b, float t, float fb) {
    TRY_CALL(F_CAMERA_FX, (float)p->Call("Lerp", { F(a), F(b), F(t) }).AsFloat(), fb)
}

int pcode::NguoiThang(float tiLe1, float tiLe2, int fb) {
    TRY_CALL(F_ROUNDS, (int)p->Call("NguoiThang", { F(tiLe1), F(tiLe2) }).AsInt(), fb)
}

bool pcode::MauAmThanh(std::vector<float>& out, int sampleRate, float seconds) {
    Program* p = P(F_MUSIC);
    if (!p) return false;
    try {
        out.clear();
        int n = (int)(seconds * sampleRate);
        for (int i = 0; i < n; ++i) out.push_back((float)p->Call("MauAmThanh", { F((double)i / sampleRate) }).AsFloat());
        return true;
    } catch (const Error& e) {
        Fail(F_MUSIC, e);
        return false;
    }
}
