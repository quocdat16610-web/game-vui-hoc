// =============================================================================
//  Challenges.cpp — đề bài, code khởi đầu, lời giải và TEST của 16 thử thách.
//  Mỗi test gọi hàm người chơi viết qua trình thông dịch MiniCpp rồi so sánh
//  kết quả với đáp án — giống "unit test" trong các dự án thật.
// =============================================================================
#include "Challenges.h"
#include "Gfx.h"
#include <cmath>
#include <cstdio>

using namespace mcpp;

namespace {
    Slot I(long long v) { return Program::MakeSlot(Value::Int(v)); }
    Slot F(double v) { return Program::MakeSlot(Value::Float(v)); }
    Slot B(bool v) { return Program::MakeSlot(Value::Bool(v)); }

    std::string Fmt(double d) { return Value::Float(d).ToString(); }

    std::string ErrText(const Error& e) {
        return e.line > 0 ? "Lỗi khi chạy (dòng " + std::to_string(e.line) + "): " + e.msg : "Lỗi khi chạy: " + e.msg;
    }

    // Chạy một phép thử: so sánh kết quả với đáp án
    void Check(std::vector<TestResult>& out, const std::string& name, const std::function<Value()>& run,
               const std::string& want, const std::function<bool(const Value&)>& ok, const std::string& hint = "") {
        TestResult r;
        r.name = name;
        try {
            Value v = run();
            r.pass = ok(v);
            r.detail = r.pass ? "→ " + v.ToString() : "→ " + v.ToString() + ", nhưng cần " + want;
            if (!r.pass && !hint.empty()) r.detail += "\nGợi ý: " + hint;
        } catch (const Error& e) {
            r.pass = false;
            r.detail = ErrText(e);
            r.line = e.line;
        }
        out.push_back(r);
    }

    std::function<bool(const Value&)> Near(double want, double eps = 1e-3) {
        return [=](const Value& v) { return v.IsNumber() && std::fabs(v.AsFloat() - want) <= eps; };
    }
    std::function<bool(const Value&)> Eq(long long want) {
        return [=](const Value& v) { return v.IsNumber() && v.kind != Kind::Float && v.AsInt() == want; };
    }
    std::function<bool(const Value&)> IsBool(bool want) {
        return [=](const Value& v) { return v.IsNumber() && v.AsBool() == want; };
    }

    bool Require(Program& p, std::vector<TestResult>& out, const std::string& fn, const std::string& sig) {
        if (p.HasFunction(fn)) return true;
        out.push_back({ sig, false, "Không tìm thấy hàm " + fn + ". Đừng đổi tên hàm nhé!" });
        return false;
    }

    std::vector<Challenge> Build() {
        std::vector<Challenge> C(16);

        // ---------------------------------------------------------------- BÀI 1
        C[0].title = "float TiLeMau(int mau, int mauToiDa)";
        C[0].task =
            "Viết hàm TiLeMau trả về tỉ lệ máu còn lại, từ 0.0 (hết máu) đến 1.0 (đầy máu).\n"
            "Ví dụ: mau = 77, mauToiDa = 100 thì trả về 0.77.\n"
            "> Game dùng CHÍNH HÀM NÀY để vẽ độ dài thanh máu trong võ đài.";
        C[0].starter =
            "// Thanh máu dài bao nhiêu phần trăm?\n"
            "// mau: máu hiện tại, mauToiDa: máu tối đa\n"
            "float TiLeMau(int mau, int mauToiDa) {\n"
            "    // TODO: tính và trả về tỉ lệ máu (0.0 -> 1.0)\n"
            "    return 0;\n"
            "}\n";
        C[0].solution =
            "float TiLeMau(int mau, int mauToiDa) {\n"
            "    return (float)mau / mauToiDa;   // ép kiểu để không bị chia nguyên\n"
            "}\n";
        C[0].hints = { "Tỉ lệ = mau chia cho mauToiDa.",
                       "Nếu kết quả luôn bằng 0: hai số int chia nhau sẽ mất phần thập phân (77 / 100 = 0). Hãy ép kiểu: (float)mau / mauToiDa" };
        C[0].usedIn = "Vẽ thanh máu";
        C[0].tests = [](Program& p, std::vector<TestResult>& o) {
            if (!Require(p, o, "TiLeMau", "TiLeMau(...)")) return;
            const char* hint = "77 / 100 trong C++ bằng 0 vì cả hai là int. Ép kiểu: (float)mau / mauToiDa";
            Check(o, "TiLeMau(77, 100)", [&] { return p.Call("TiLeMau", { I(77), I(100) }); }, "0.77", Near(0.77), hint);
            Check(o, "TiLeMau(100, 100)", [&] { return p.Call("TiLeMau", { I(100), I(100) }); }, "1.0", Near(1.0));
            Check(o, "TiLeMau(0, 100)", [&] { return p.Call("TiLeMau", { I(0), I(100) }); }, "0.0", Near(0.0));
            Check(o, "TiLeMau(45, 90)", [&] { return p.Call("TiLeMau", { I(45), I(90) }); }, "0.5", Near(0.5), hint);
        };

        // ---------------------------------------------------------------- BÀI 2
        C[1].title = "KetThucHiep() và SoDonDeKO()";
        C[1].task =
            "Viết 2 hàm:\n"
            "- bool KetThucHiep(int mau1, int mau2): trả về true khi MỘT TRONG HAI võ sĩ hết máu (máu <= 0).\n"
            "- int SoDonDeKO(int mau, int satThuong): dùng vòng lặp while, đếm cần bao nhiêu đòn để máu về 0 hoặc thấp hơn.\n"
            "Ví dụ: SoDonDeKO(100, 12) = 9 (sau 8 đòn còn 4 máu, đòn thứ 9 mới K.O.).\n"
            "> Game gọi KetThucHiep mỗi frame trong game loop để biết khi nào kết thúc hiệp, "
            "và hiện SoDonDeKO dưới thanh máu đối thủ.";
        C[1].starter =
            "// true nếu một trong hai võ sĩ hết máu\n"
            "bool KetThucHiep(int mau1, int mau2) {\n"
            "    // TODO: dùng if hoặc toán tử ||\n"
            "    return false;\n"
            "}\n"
            "\n"
            "// Cần bao nhiêu đòn để máu về 0?\n"
            "int SoDonDeKO(int mau, int satThuong) {\n"
            "    int soDon = 0;\n"
            "    // TODO: dùng vòng lặp while\n"
            "    return soDon;\n"
            "}\n";
        C[1].solution =
            "bool KetThucHiep(int mau1, int mau2) {\n"
            "    return mau1 <= 0 || mau2 <= 0;\n"
            "}\n"
            "\n"
            "int SoDonDeKO(int mau, int satThuong) {\n"
            "    int soDon = 0;\n"
            "    while (mau > 0) {\n"
            "        mau -= satThuong;\n"
            "        soDon++;\n"
            "    }\n"
            "    return soDon;\n"
            "}\n";
        C[1].hints = { "KetThucHiep: return mau1 <= 0 || mau2 <= 0;",
                       "SoDonDeKO: while (mau > 0) { trừ máu; tăng soDon; }. Nhớ thay đổi mau trong vòng lặp, nếu không sẽ lặp vô hạn!" };
        C[1].usedIn = "Kết thúc hiệp + số đòn còn lại";
        C[1].tests = [](Program& p, std::vector<TestResult>& o) {
            if (Require(p, o, "KetThucHiep", "KetThucHiep(...)")) {
                Check(o, "KetThucHiep(100, 50)", [&] { return p.Call("KetThucHiep", { I(100), I(50) }); }, "false", IsBool(false));
                Check(o, "KetThucHiep(0, 50)", [&] { return p.Call("KetThucHiep", { I(0), I(50) }); }, "true", IsBool(true));
                Check(o, "KetThucHiep(40, -5)", [&] { return p.Call("KetThucHiep", { I(40), I(-5) }); }, "true", IsBool(true),
                      "máu âm cũng là hết máu: dùng <= 0");
            }
            if (Require(p, o, "SoDonDeKO", "SoDonDeKO(...)")) {
                Check(o, "SoDonDeKO(100, 12)", [&] { return p.Call("SoDonDeKO", { I(100), I(12) }); }, "9", Eq(9));
                Check(o, "SoDonDeKO(24, 8)", [&] { return p.Call("SoDonDeKO", { I(24), I(8) }); }, "3", Eq(3),
                      "điều kiện lặp là mau > 0 (máu đúng bằng 0 là đã K.O.)");
                Check(o, "SoDonDeKO(0, 10)", [&] { return p.Call("SoDonDeKO", { I(0), I(10) }); }, "0", Eq(0));
            }
        };

        // ---------------------------------------------------------------- BÀI 3
        C[2].title = "int TinhSatThuong(int lucDanh, bool biDo)";
        C[2].task =
            "Viết hàm TinhSatThuong:\n"
            "- Nếu đối thủ ĐANG ĐỠ (biDo == true): sát thương chỉ còn 1/4 (chia 4, lấy phần nguyên).\n"
            "- Ngược lại: sát thương bằng nguyên lucDanh.\n"
            "> Game gọi hàm này mỗi khi có đòn đánh trúng.";
        C[2].starter =
            "int TinhSatThuong(int lucDanh, bool biDo) {\n"
            "    // TODO: bị đỡ thì giảm còn 1/4\n"
            "    return lucDanh;\n"
            "}\n";
        C[2].solution =
            "int TinhSatThuong(int lucDanh, bool biDo) {\n"
            "    if (biDo) return lucDanh / 4;\n"
            "    return lucDanh;\n"
            "}\n";
        C[2].hints = { "Dùng if (biDo) { ... } để rẽ nhánh.", "13 / 4 với số nguyên bằng 3 — đúng như đề yêu cầu." };
        C[2].usedIn = "Tính sát thương mỗi đòn";
        C[2].tests = [](Program& p, std::vector<TestResult>& o) {
            if (!Require(p, o, "TinhSatThuong", "TinhSatThuong(...)")) return;
            Check(o, "TinhSatThuong(20, false)", [&] { return p.Call("TinhSatThuong", { I(20), B(false) }); }, "20", Eq(20));
            Check(o, "TinhSatThuong(20, true)", [&] { return p.Call("TinhSatThuong", { I(20), B(true) }); }, "5", Eq(5));
            Check(o, "TinhSatThuong(13, true)", [&] { return p.Call("TinhSatThuong", { I(13), B(true) }); }, "3", Eq(3));
            Check(o, "TinhSatThuong(8, false)", [&] { return p.Call("TinhSatThuong", { I(8), B(false) }); }, "8", Eq(8));
        };

        // ---------------------------------------------------------------- BÀI 4
        C[3].title = "class Fighter: hàm NhanDon()";
        C[3].task =
            "Hoàn thành hàm thành viên NhanDon của class Fighter:\n"
            "- Trừ satThuong vào mau.\n"
            "- Máu không bao giờ được âm: nếu mau <= 0 thì đặt mau = 0 và conSong = false.\n"
            "> Mỗi võ sĩ trong võ đài sẽ nhận đòn qua CHÍNH hàm này. conSong = false là K.O.!";
        C[3].starter =
            "class Fighter {\n"
            "public:\n"
            "    int mau = 100;\n"
            "    bool conSong = true;\n"
            "\n"
            "    // Gọi khi võ sĩ trúng đòn\n"
            "    void NhanDon(int satThuong) {\n"
            "        // TODO: trừ máu, không để máu âm, hết máu thì conSong = false\n"
            "    }\n"
            "};\n";
        C[3].solution =
            "class Fighter {\n"
            "public:\n"
            "    int mau = 100;\n"
            "    bool conSong = true;\n"
            "\n"
            "    void NhanDon(int satThuong) {\n"
            "        mau -= satThuong;\n"
            "        if (mau <= 0) {\n"
            "            mau = 0;\n"
            "            conSong = false;\n"
            "        }\n"
            "    }\n"
            "};\n";
        C[3].hints = { "Bên trong hàm thành viên, bạn dùng thẳng tên biến mau, conSong (hoặc this->mau).",
                       "mau -= satThuong; rồi if (mau <= 0) { ... }" };
        C[3].usedIn = "Trừ máu + phát hiện K.O.";
        C[3].tests = [](Program& p, std::vector<TestResult>& o) {
            const StructDef* sd = p.FindStruct("Fighter");
            if (!sd) { o.push_back({ "class Fighter", false, "Không tìm thấy class Fighter" }); return; }
            if (!sd->methods.count("NhanDon")) { o.push_back({ "Fighter::NhanDon", false, "class Fighter chưa có hàm NhanDon" }); return; }
            if (sd->FieldIndex("mau") < 0 || sd->FieldIndex("conSong") < 0) {
                o.push_back({ "Biến thành viên", false, "Cần giữ biến thành viên mau và conSong" });
                return;
            }
            Slot f = p.NewObject("Fighter");
            Check(o, "f.NhanDon(30)  → mau", [&] { p.CallMethod(f, "NhanDon", { I(30) }); return *Program::Field(f, "mau"); }, "70", Eq(70));
            Check(o, "               → conSong", [&] { return *Program::Field(f, "conSong"); }, "true", IsBool(true));
            Check(o, "f.NhanDon(80)  → mau", [&] { p.CallMethod(f, "NhanDon", { I(80) }); return *Program::Field(f, "mau"); }, "0", Eq(0),
                  "máu không được âm: nếu mau <= 0 thì mau = 0");
            Check(o, "               → conSong", [&] { return *Program::Field(f, "conSong"); }, "false", IsBool(false));
            Slot g = p.NewObject("Fighter");
            Check(o, "g.NhanDon(100) → conSong", [&] { p.CallMethod(g, "NhanDon", { I(100) }); return *Program::Field(g, "conSong"); },
                  "false", IsBool(false), "máu đúng bằng 0 cũng là K.O. (dùng <= 0)");
        };

        // ---------------------------------------------------------------- BÀI 5
        C[4].title = "class VoSiCuaBan : public Character";
        C[4].task =
            "Thiết kế VÕ SĨ CỦA RIÊNG BẠN bằng cách kế thừa Character và ghi đè (override) 4 hàm ảo:\n"
            "- float TocDo(): 2.5 đến 6.5 (mét/giây)\n"
            "- int LucDam(): 4 đến 14,  int LucDa(): 6 đến 18\n"
            "- int MauToiDa(): 70 đến 140\n"
            "Luật cân bằng game: TocDo*10 + LucDam*3 + LucDa*2 + MauToiDa/2 <= 160.\n"
            "> Võ sĩ của bạn sẽ xuất hiện ở màn chọn võ sĩ!";
        C[4].starter =
            "// Lớp cha Character đã có sẵn trong game.\n"
            "class VoSiCuaBan : public Character {\n"
            "public:\n"
            "    float TocDo() const override { return 4.0f; }\n"
            "    int LucDam() const override { return 8; }\n"
            "    // TODO: ghi đè thêm int LucDa() và int MauToiDa()\n"
            "};\n";
        C[4].solution =
            "class VoSiCuaBan : public Character {\n"
            "public:\n"
            "    float TocDo() const override { return 4.5f; }\n"
            "    int LucDam() const override { return 9; }\n"
            "    int LucDa() const override { return 13; }\n"
            "    int MauToiDa() const override { return 105; }\n"
            "};\n";
        C[4].hints = { "Viết giống hai hàm có sẵn: int LucDa() const override { return 12; }",
                       "Bị vượt điểm cân bằng? Giảm bớt một chỉ số: nhanh thì yếu, trâu thì chậm." };
        C[4].usedIn = "Võ sĩ \"Của bạn\" ở màn chọn võ sĩ";
        C[4].tests = [](Program& p, std::vector<TestResult>& o) {
            const StructDef* sd = p.FindStruct("VoSiCuaBan");
            if (!sd) { o.push_back({ "class VoSiCuaBan", false, "Không tìm thấy class VoSiCuaBan" }); return; }
            o.push_back({ "Kế thừa Character", sd->base == "Character", sd->base == "Character" ? "✓" : "Cần viết: class VoSiCuaBan : public Character" });
            const char* names[4] = { "TocDo", "LucDam", "LucDa", "MauToiDa" };
            double lo[4] = { 2.5, 4, 6, 70 }, hi[4] = { 6.5, 14, 18, 140 };
            double val[4] = {};
            bool all = true;
            Slot obj = p.NewObject("VoSiCuaBan");
            for (int k = 0; k < 4; ++k) {
                if (!sd->methods.count(names[k])) {
                    o.push_back({ std::string(names[k]) + "()", false, "Chưa ghi đè hàm " + std::string(names[k]) + "()" });
                    all = false;
                    continue;
                }
                std::string want = Fmt(lo[k]) + " đến " + Fmt(hi[k]);
                Check(o, std::string(names[k]) + "()", [&] { Value v = p.CallMethod(obj, names[k], {}); val[k] = v.AsFloat(); return v; },
                      want, [&](const Value& v) { return v.IsNumber() && v.AsFloat() >= lo[k] && v.AsFloat() <= hi[k]; });
                if (!o.back().pass) all = false;
            }
            if (all) {
                double score = val[0] * 10 + val[1] * 3 + val[2] * 2 + val[3] / 2;
                TestResult r;
                r.name = "Luật cân bằng";
                r.pass = score <= 160.0001;
                char buf[160];
                std::snprintf(buf, sizeof(buf), "điểm = %.1f (tối đa 160)", score);
                r.detail = buf;
                if (!r.pass) r.detail += "\nGợi ý: võ sĩ quá mạnh, hãy giảm bớt một chỉ số";
                o.push_back(r);
            }
        };

        // ---------------------------------------------------------------- BÀI 6
        C[5].title = "void CapNhatHat(Particle& p, float dt)";
        C[5].task =
            "Cập nhật một hạt tia lửa sau dt giây, theo đúng thứ tự:\n"
            "- p.vy -= 15 * dt   (trọng lực kéo xuống)\n"
            "- p.x += p.vx * dt   và   p.y += p.vy * dt\n"
            "- p.life -= dt   (giảm thời gian sống)\n"
            "! Code khởi đầu có một lỗi về tham số: hàm có sửa được hạt GỐC không?\n"
            "> Mọi tia lửa khi đánh trúng đều bay theo hàm này.";
        C[5].starter =
            "struct Particle {\n"
            "    float x, y;      // vị trí\n"
            "    float vx, vy;    // vận tốc\n"
            "    float life;      // thời gian sống còn lại\n"
            "};\n"
            "\n"
            "void CapNhatHat(Particle p, float dt) {\n"
            "    // TODO: trọng lực, di chuyển, giảm life\n"
            "}\n";
        C[5].solution =
            "struct Particle {\n"
            "    float x, y;\n"
            "    float vx, vy;\n"
            "    float life;\n"
            "};\n"
            "\n"
            "void CapNhatHat(Particle& p, float dt) {   // & : sửa hạt gốc\n"
            "    p.vy -= 15 * dt;\n"
            "    p.x += p.vx * dt;\n"
            "    p.y += p.vy * dt;\n"
            "    p.life -= dt;\n"
            "}\n";
        C[5].hints = { "Tham số Particle p là BẢN SAO: sửa bao nhiêu thì hạt gốc cũng không đổi. Hãy dùng tham chiếu Particle& p.",
                       "Nhớ cập nhật p.vy TRƯỚC rồi mới cộng vào p.y." };
        C[5].usedIn = "Chuyển động của tia lửa";
        C[5].tests = [](Program& p, std::vector<TestResult>& o) {
            if (!p.FindStruct("Particle")) { o.push_back({ "struct Particle", false, "Đừng xóa struct Particle" }); return; }
            if (!Require(p, o, "CapNhatHat", "CapNhatHat(...)")) return;
            Slot h = p.NewObject("Particle");
            auto set = [&](const char* f, double v) { Slot s = Program::Field(h, f); if (s) s->f = v; };
            set("x", 0); set("y", 1); set("vx", 2); set("vy", 3); set("life", 0.5);
            bool ran = true;
            std::string err;
            try { p.Call("CapNhatHat", { h, F(0.1) }); } catch (const Error& e) { ran = false; err = ErrText(e); }
            if (!ran) { o.push_back({ "CapNhatHat(hat, 0.1)", false, err }); return; }
            const char* hint = "hạt gốc không thay đổi? Tham số phải là tham chiếu: Particle& p";
            auto field = [&](const char* f) { Slot s = Program::Field(h, f); return s ? *s : Value(); };
            Check(o, "sau 0.1s: vy (3 → 1.5)", [&] { return field("vy"); }, "1.5", Near(1.5), hint);
            Check(o, "          x  (0 → 0.2)", [&] { return field("x"); }, "0.2", Near(0.2), hint);
            Check(o, "          y  (1 → 1.15)", [&] { return field("y"); }, "1.15", Near(1.15), "cập nhật vy trước, rồi y += vy * dt");
            Check(o, "          life (0.5 → 0.4)", [&] { return field("life"); }, "0.4", Near(0.4), hint);
        };

        // ---------------------------------------------------------------- BÀI 7
        C[6].title = "int DonManhNhat(const std::vector<int>& cacDon)";
        C[6].task =
            "cacDon chứa sát thương của từng đòn đánh trúng trong trận. Trả về đòn MẠNH NHẤT. "
            "Nếu vector rỗng (chưa trúng đòn nào) thì trả về 0.\n"
            "Dùng vòng for (int d : cacDon) để duyệt.\n"
            "> Bảng thống kê cuối trận dùng hàm này cho dòng \"Đòn mạnh nhất\".";
        C[6].starter =
            "#include <vector>\n"
            "\n"
            "int DonManhNhat(const std::vector<int>& cacDon) {\n"
            "    int manhNhat = 0;\n"
            "    // TODO: duyệt vector, cập nhật manhNhat\n"
            "    return manhNhat;\n"
            "}\n";
        C[6].solution =
            "#include <vector>\n"
            "\n"
            "int DonManhNhat(const std::vector<int>& cacDon) {\n"
            "    int manhNhat = 0;\n"
            "    for (int d : cacDon) {\n"
            "        if (d > manhNhat) manhNhat = d;\n"
            "    }\n"
            "    return manhNhat;\n"
            "}\n";
        C[6].hints = { "for (int d : cacDon) { if (d > manhNhat) manhNhat = d; }",
                       "Cũng có thể dùng vòng for có chỉ số: for (int i = 0; i < cacDon.size(); i++)" };
        C[6].usedIn = "Thống kê \"Đòn mạnh nhất\"";
        C[6].tests = [](Program& p, std::vector<TestResult>& o) {
            if (!Require(p, o, "DonManhNhat", "DonManhNhat(...)")) return;
            auto vec = [](std::vector<int> xs) {
                Slot v = Program::NewVector(Kind::Int);
                for (int x : xs) v->items.push_back(I(x));
                return v;
            };
            Check(o, "DonManhNhat({8, 12, 6})", [&] { return p.Call("DonManhNhat", { vec({ 8, 12, 6 }) }); }, "12", Eq(12));
            Check(o, "DonManhNhat({3, 3, 20, 1})", [&] { return p.Call("DonManhNhat", { vec({ 3, 3, 20, 1 }) }); }, "20", Eq(20));
            Check(o, "DonManhNhat({5})", [&] { return p.Call("DonManhNhat", { vec({ 5 }) }); }, "5", Eq(5));
            Check(o, "DonManhNhat({})", [&] { return p.Call("DonManhNhat", { vec({}) }); }, "0", Eq(0));
        };

        // ---------------------------------------------------------------- BÀI 8
        C[7].title = "float DiChuyen(float x, int huong, float tocDo, float dt)";
        C[7].task =
            "Code khởi đầu đang mắc lỗi kinh điển: cộng một lượng cố định mỗi frame, nên máy càng nhiều FPS "
            "nhân vật chạy càng nhanh. Hãy sửa để vị trí mới = x + huong * tocDo * dt.\n"
            "huong là -1 (trái), 0 (đứng yên) hoặc 1 (phải).\n"
            "> Mọi bước chân của võ sĩ trong võ đài đi qua hàm này.";
        C[7].starter =
            "float DiChuyen(float x, int huong, float tocDo, float dt) {\n"
            "    // SAI: không dùng tocDo và dt -> tốc độ phụ thuộc FPS!\n"
            "    return x + huong * 0.1f;\n"
            "}\n";
        C[7].solution =
            "float DiChuyen(float x, int huong, float tocDo, float dt) {\n"
            "    return x + huong * tocDo * dt;\n"
            "}\n";
        C[7].hints = { "Quãng đường = vận tốc × thời gian. Vận tốc = huong * tocDo." };
        C[7].usedIn = "Di chuyển của võ sĩ";
        C[7].tests = [](Program& p, std::vector<TestResult>& o) {
            if (!Require(p, o, "DiChuyen", "DiChuyen(...)")) return;
            Check(o, "DiChuyen(0, 1, 4, 0.5)", [&] { return p.Call("DiChuyen", { F(0), I(1), F(4), F(0.5) }); }, "2.0", Near(2.0));
            Check(o, "DiChuyen(2, -1, 4, 0.25)", [&] { return p.Call("DiChuyen", { F(2), I(-1), F(4), F(0.25) }); }, "1.0", Near(1.0));
            Check(o, "DiChuyen(1, 0, 5, 1)", [&] { return p.Call("DiChuyen", { F(1), I(0), F(5), F(1) }); }, "1.0", Near(1.0));
            Check(o, "60 FPS: DiChuyen(0, 1, 6, 1/60)", [&] { return p.Call("DiChuyen", { F(0), I(1), F(6), F(1.0 / 60) }); }, "0.1", Near(0.1));
            Check(o, "144 FPS: DiChuyen(0, 1, 6, 1/144)", [&] { return p.Call("DiChuyen", { F(0), I(1), F(6), F(1.0 / 144) }); }, "0.04167",
                  Near(6.0 / 144, 1e-4), "máy 144 FPS phải đi ít hơn mỗi frame: nhân với dt");
        };

        // ---------------------------------------------------------------- BÀI 9
        C[8].title = "void TrongLuc(float& y, float& vy, float dt)";
        C[8].task =
            "Mô phỏng trọng lực khi võ sĩ nhảy (trọng lực = -30), theo thứ tự:\n"
            "- vy += -30 * dt   (gia tốc đổi vận tốc)\n"
            "- y += vy * dt   (vận tốc đổi vị trí)\n"
            "- nếu y <= 0 (chạm đất): đặt y = 0 và vy = 0\n"
            "Hai tham số là tham chiếu (&) nên hàm sửa trực tiếp vị trí và vận tốc của võ sĩ.\n"
            "> Mọi cú nhảy trong võ đài dùng hàm này.";
        C[8].starter =
            "const float TRONG_LUC = -30.0f;\n"
            "\n"
            "void TrongLuc(float& y, float& vy, float dt) {\n"
            "    // TODO: cập nhật vy, rồi y, rồi xử lý chạm đất\n"
            "}\n";
        C[8].solution =
            "const float TRONG_LUC = -30.0f;\n"
            "\n"
            "void TrongLuc(float& y, float& vy, float dt) {\n"
            "    vy += TRONG_LUC * dt;\n"
            "    y += vy * dt;\n"
            "    if (y <= 0) {\n"
            "        y = 0;\n"
            "        vy = 0;\n"
            "    }\n"
            "}\n";
        C[8].hints = { "Thứ tự rất quan trọng: vy trước, y sau (tích phân Euler bán ẩn)." };
        C[8].usedIn = "Vật lý cú nhảy";
        C[8].tests = [](Program& p, std::vector<TestResult>& o) {
            if (!Require(p, o, "TrongLuc", "TrongLuc(...)")) return;
            Slot y = F(0), vy = F(10);
            Check(o, "y=0, vy=10, dt=0.1 → vy", [&] { p.Call("TrongLuc", { y, vy, F(0.1) }); return *vy; }, "7.0", Near(7.0));
            Check(o, "                    → y", [&] { return *y; }, "0.7", Near(0.7), "y += vy * dt, dùng vy MỚI");
            Slot y2 = F(0.1), vy2 = F(-5);
            Check(o, "y=0.1, vy=-5 (rơi xuống) → y", [&] { p.Call("TrongLuc", { y2, vy2, F(0.1) }); return *y2; }, "0.0", Near(0.0),
                  "chạm đất: if (y <= 0) { y = 0; vy = 0; }");
            Check(o, "                          → vy", [&] { return *vy2; }, "0.0", Near(0.0));
        };

        // ---------------------------------------------------------------- BÀI 10
        C[9].title = "bool DangGayDon(int loaiDon, float t)";
        C[9].task =
            "Đòn đánh chỉ gây sát thương trong pha ACTIVE. Dùng switch để trả về true khi:\n"
            "- loaiDon == DAM và 0.10 <= t < 0.20\n"
            "- loaiDon == DA  và 0.22 <= t < 0.34\n"
            "- các trường hợp khác: false\n"
            "t là thời gian (giây) kể từ khi bắt đầu ra đòn.\n"
            "> Võ đài hỏi hàm này mỗi frame: hitbox có đang bật không?";
        C[9].starter =
            "const int DAM = 1;\n"
            "const int DA = 2;\n"
            "\n"
            "bool DangGayDon(int loaiDon, float t) {\n"
            "    switch (loaiDon) {\n"
            "        case DAM:\n"
            "            // TODO\n"
            "            return false;\n"
            "        // TODO: case DA\n"
            "        default:\n"
            "            return false;\n"
            "    }\n"
            "}\n";
        C[9].solution =
            "const int DAM = 1;\n"
            "const int DA = 2;\n"
            "\n"
            "bool DangGayDon(int loaiDon, float t) {\n"
            "    switch (loaiDon) {\n"
            "        case DAM: return t >= 0.10f && t < 0.20f;\n"
            "        case DA:  return t >= 0.22f && t < 0.34f;\n"
            "        default:  return false;\n"
            "    }\n"
            "}\n";
        C[9].hints = { "case DAM: return t >= 0.10f && t < 0.20f;" };
        C[9].usedIn = "Frame data: khi nào hitbox bật";
        C[9].tests = [](Program& p, std::vector<TestResult>& o) {
            if (!Require(p, o, "DangGayDon", "DangGayDon(...)")) return;
            struct T { int loai; double t; bool want; } cases[] = {
                { 1, 0.05, false }, { 1, 0.15, true }, { 1, 0.20, false }, { 2, 0.15, false }, { 2, 0.25, true }, { 2, 0.40, false }, { 7, 0.15, false } };
            for (auto& c : cases) {
                std::string name = "DangGayDon(" + std::string(c.loai == 1 ? "DAM" : c.loai == 2 ? "DA" : "7") + ", " + Fmt(c.t) + ")";
                Check(o, name, [&] { return p.Call("DangGayDon", { I(c.loai), F(c.t) }); }, c.want ? "true" : "false", IsBool(c.want));
            }
        };

        // ---------------------------------------------------------------- BÀI 11
        C[10].title = "bool VaCham(Hop a, Hop b)";
        C[10].task =
            "Hai hộp chữ nhật (AABB) chạm nhau khi chúng chồng lên nhau trên CẢ trục x LẪN trục y:\n"
            "- a.minX <= b.maxX và a.maxX >= b.minX\n"
            "- a.minY <= b.maxY và a.maxY >= b.minY\n"
            "(Chạm mép cũng tính là va chạm.)\n"
            "> Mọi cú đấm, cú đá: game kiểm tra hitbox với hurtbox bằng hàm này.";
        C[10].starter =
            "struct Hop {\n"
            "    float minX, minY;   // góc dưới trái\n"
            "    float maxX, maxY;   // góc trên phải\n"
            "};\n"
            "\n"
            "bool VaCham(Hop a, Hop b) {\n"
            "    // TODO\n"
            "    return false;\n"
            "}\n";
        C[10].solution =
            "struct Hop {\n"
            "    float minX, minY;\n"
            "    float maxX, maxY;\n"
            "};\n"
            "\n"
            "bool VaCham(Hop a, Hop b) {\n"
            "    return a.minX <= b.maxX && a.maxX >= b.minX &&\n"
            "           a.minY <= b.maxY && a.maxY >= b.minY;\n"
            "}\n";
        C[10].hints = { "Chỉ cần 1 trục tách rời là KHÔNG va chạm. Ghép 4 điều kiện bằng &&." };
        C[10].usedIn = "Hitbox / hurtbox";
        C[10].tests = [](Program& p, std::vector<TestResult>& o) {
            if (!p.FindStruct("Hop")) { o.push_back({ "struct Hop", false, "Đừng xóa struct Hop" }); return; }
            if (!Require(p, o, "VaCham", "VaCham(...)")) return;
            auto hop = [&](double a, double b, double c, double d) {
                Slot h = p.NewObject("Hop");
                const char* f[4] = { "minX", "minY", "maxX", "maxY" };
                double v[4] = { a, b, c, d };
                for (int k = 0; k < 4; ++k) if (Slot s = Program::Field(h, f[k])) s->f = v[k];
                return h;
            };
            Check(o, "chồng nhau", [&] { return p.Call("VaCham", { hop(0, 0, 2, 2), hop(1, 1, 3, 3) }); }, "true", IsBool(true));
            Check(o, "tách nhau theo trục x", [&] { return p.Call("VaCham", { hop(0, 0, 1, 1), hop(2, 0, 3, 1) }); }, "false", IsBool(false));
            Check(o, "tách nhau theo trục y", [&] { return p.Call("VaCham", { hop(0, 0, 1, 1), hop(0, 2, 1, 3) }); }, "false", IsBool(false),
                  "phải kiểm tra cả trục y (đối thủ đang nhảy cao)");
            Check(o, "chạm mép", [&] { return p.Call("VaCham", { hop(0, 0, 1, 1), hop(1, 0, 2, 1) }); }, "true", IsBool(true), "chạm mép dùng <= và >=");
            Check(o, "hộp nhỏ nằm trong hộp lớn", [&] { return p.Call("VaCham", { hop(0, 0, 5, 5), hop(2, 2, 3, 3) }); }, "true", IsBool(true));
        };

        // ---------------------------------------------------------------- BÀI 12
        C[11].title = "int ChonHanhDong(float khoangCach, bool doiThuDangDanh, float ngauNhien)";
        C[11].task =
            "Viết \"bộ não\" cho AI. ngauNhien là số ngẫu nhiên từ 0 đến 1. Quy tắc theo thứ tự ưu tiên:\n"
            "- 1. Nếu đối thủ đang đánh, khoảng cách < 2 và ngauNhien < 0.5: PHONG_THU\n"
            "- 2. Nếu khoảng cách > 1.5: TIEP_CAN\n"
            "- 3. Còn lại: ngauNhien < 0.7 thì TAN_CONG, ngược lại LUI_LAI\n"
            "> Đối thủ AI trong võ đài ra quyết định bằng chính hàm này!";
        C[11].starter =
            "const int TIEP_CAN = 0;\n"
            "const int LUI_LAI = 1;\n"
            "const int TAN_CONG = 2;\n"
            "const int PHONG_THU = 3;\n"
            "\n"
            "int ChonHanhDong(float khoangCach, bool doiThuDangDanh, float ngauNhien) {\n"
            "    // TODO: theo đúng thứ tự ưu tiên trong đề\n"
            "    return TIEP_CAN;\n"
            "}\n";
        C[11].solution =
            "const int TIEP_CAN = 0;\n"
            "const int LUI_LAI = 1;\n"
            "const int TAN_CONG = 2;\n"
            "const int PHONG_THU = 3;\n"
            "\n"
            "int ChonHanhDong(float khoangCach, bool doiThuDangDanh, float ngauNhien) {\n"
            "    if (doiThuDangDanh && khoangCach < 2.0f && ngauNhien < 0.5f) return PHONG_THU;\n"
            "    if (khoangCach > 1.5f) return TIEP_CAN;\n"
            "    if (ngauNhien < 0.7f) return TAN_CONG;\n"
            "    return LUI_LAI;\n"
            "}\n";
        C[11].hints = { "Viết các if theo đúng thứ tự, mỗi if có return riêng: điều kiện trước được ưu tiên." };
        C[11].usedIn = "Bộ não AI đối thủ";
        C[11].tests = [](Program& p, std::vector<TestResult>& o) {
            if (!Require(p, o, "ChonHanhDong", "ChonHanhDong(...)")) return;
            const char* names[4] = { "TIEP_CAN", "LUI_LAI", "TAN_CONG", "PHONG_THU" };
            struct T { double d; bool att; double r; int want; } cases[] = {
                { 3.0, false, 0.5, 0 }, { 1.0, true, 0.2, 3 }, { 1.0, true, 0.6, 2 }, { 1.0, false, 0.3, 2 }, { 1.0, false, 0.9, 1 }, { 3.0, true, 0.1, 0 } };
            for (auto& c : cases) {
                std::string name = "ChonHanhDong(" + Fmt(c.d) + ", " + (c.att ? "true" : "false") + ", " + Fmt(c.r) + ")";
                Check(o, name, [&] { return p.Call("ChonHanhDong", { F(c.d), B(c.att), F(c.r) }); }, names[c.want], Eq(c.want));
            }
        };

        // ---------------------------------------------------------------- BÀI 13
        C[12].title = "float Lerp(float a, float b, float t)";
        C[12].task =
            "Nội suy tuyến tính: Lerp(a, b, t) = a + (b - a) * t.\n"
            "t = 0 cho ra a, t = 1 cho ra b, t = 0.5 cho ra điểm chính giữa.\n"
            "> Camera động trong võ đài đuổi theo trận đấu một cách mượt mà nhờ hàm này.";
        C[12].starter =
            "float Lerp(float a, float b, float t) {\n"
            "    // TODO\n"
            "    return a;\n"
            "}\n";
        C[12].solution =
            "float Lerp(float a, float b, float t) {\n"
            "    return a + (b - a) * t;\n"
            "}\n";
        C[12].hints = { "Đi từ a, tiến thêm một phần t của quãng đường (b - a)." };
        C[12].usedIn = "Camera bám theo trận đấu";
        C[12].tests = [](Program& p, std::vector<TestResult>& o) {
            if (!Require(p, o, "Lerp", "Lerp(...)")) return;
            Check(o, "Lerp(0, 10, 0.5)", [&] { return p.Call("Lerp", { F(0), F(10), F(0.5) }); }, "5.0", Near(5));
            Check(o, "Lerp(2, 4, 0)", [&] { return p.Call("Lerp", { F(2), F(4), F(0) }); }, "2.0", Near(2));
            Check(o, "Lerp(2, 4, 1)", [&] { return p.Call("Lerp", { F(2), F(4), F(1) }); }, "4.0", Near(4));
            Check(o, "Lerp(-1, 1, 0.25)", [&] { return p.Call("Lerp", { F(-1), F(1), F(0.25) }); }, "-0.5", Near(-0.5));
        };

        // ---------------------------------------------------------------- BÀI 14
        C[13].title = "int NguoiThang(float tiLe1, float tiLe2)";
        C[13].task =
            "Khi HẾT GIỜ, võ sĩ còn nhiều phần trăm máu hơn sẽ thắng hiệp.\n"
            "Trả về 1 nếu tiLe1 > tiLe2, trả về 2 nếu tiLe2 > tiLe1, trả về 0 nếu bằng nhau (hòa).\n"
            "> Luật hết giờ của trận đấu dùng hàm này.";
        C[13].starter =
            "int NguoiThang(float tiLe1, float tiLe2) {\n"
            "    // TODO\n"
            "    return 0;\n"
            "}\n";
        C[13].solution =
            "int NguoiThang(float tiLe1, float tiLe2) {\n"
            "    if (tiLe1 > tiLe2) return 1;\n"
            "    if (tiLe2 > tiLe1) return 2;\n"
            "    return 0;\n"
            "}\n";
        C[13].hints = { "Hai câu if và một return 0 ở cuối." };
        C[13].usedIn = "Luật hết giờ";
        C[13].tests = [](Program& p, std::vector<TestResult>& o) {
            if (!Require(p, o, "NguoiThang", "NguoiThang(...)")) return;
            Check(o, "NguoiThang(0.8, 0.3)", [&] { return p.Call("NguoiThang", { F(0.8), F(0.3) }); }, "1", Eq(1));
            Check(o, "NguoiThang(0.2, 0.9)", [&] { return p.Call("NguoiThang", { F(0.2), F(0.9) }); }, "2", Eq(2));
            Check(o, "NguoiThang(0.5, 0.5)", [&] { return p.Call("NguoiThang", { F(0.5), F(0.5) }); }, "0", Eq(0));
        };

        // ---------------------------------------------------------------- BÀI 15
        C[14].title = "GLSL: float AnhSang(vec3 N, vec3 L)";
        C[14].glsl = true;
        C[14].task =
            "Đây là code GLSL: chạy trên CARD ĐỒ HỌA cho từng điểm ảnh! Cú pháp gần như C++.\n"
            "N là pháp tuyến bề mặt, L là hướng tới nguồn sáng (cả hai dài 1). "
            "Trả về độ sáng khuếch tán Lambert: max(dot(N, L), 0.0).\n"
            "> Khi qua thử thách, toàn bộ ánh sáng mặt trời của game (văn phòng + võ đài) được tính bằng hàm của bạn. "
            "Viết sai thì cả thế giới tối thui!";
        C[14].starter =
            "// N: pháp tuyến bề mặt, L: hướng tới nguồn sáng\n"
            "// Trả về 0.0 (tối) đến 1.0 (sáng nhất)\n"
            "float AnhSang(vec3 N, vec3 L) {\n"
            "    // TODO: dùng dot() và max()\n"
            "    return 1.0;\n"
            "}\n";
        C[14].solution =
            "float AnhSang(vec3 N, vec3 L) {\n"
            "    return max(dot(N, L), 0.0);\n"
            "}\n";
        C[14].hints = { "dot(N, L) là cos của góc giữa bề mặt và tia sáng.", "Mặt quay lưng với ánh sáng có dot âm: dùng max(..., 0.0) để không bị âm." };
        C[14].usedIn = "Ánh sáng mặt trời (shader GPU)";
        C[14].tests = [](Program& p, std::vector<TestResult>& o) {
            if (!Require(p, o, "AnhSang", "AnhSang(...)")) return;
            auto v3 = [&](double x, double y, double z) {
                Slot s = p.NewObject("vec3");
                s->items[0]->f = x; s->items[1]->f = y; s->items[2]->f = z;
                return s;
            };
            Check(o, "chiếu thẳng: N=(0,1,0), L=(0,1,0)", [&] { return p.Call("AnhSang", { v3(0, 1, 0), v3(0, 1, 0) }); }, "1.0", Near(1));
            Check(o, "chiếu ngang: L=(1,0,0)", [&] { return p.Call("AnhSang", { v3(0, 1, 0), v3(1, 0, 0) }); }, "0.0", Near(0));
            Check(o, "chiếu xiên 45°", [&] { return p.Call("AnhSang", { v3(0, 1, 0), v3(0.7071, 0.7071, 0) }); }, "0.7071", Near(0.7071));
            Check(o, "mặt quay lưng: L=(0,-1,0)", [&] { return p.Call("AnhSang", { v3(0, 1, 0), v3(0, -1, 0) }); }, "0.0", Near(0),
                  "kết quả âm sẽ làm màu sai: dùng max(dot(N, L), 0.0)");
        };

        // ---------------------------------------------------------------- BÀI 16
        C[15].title = "float MauAmThanh(float t)";
        C[15].task =
            "Tự thiết kế TIẾNG ĐẤM! Hàm trả về giá trị mẫu âm thanh tại thời điểm t (giây), "
            "game gọi hàm 4851 lần cho 0.22 giây âm thanh.\n"
            "Yêu cầu: giá trị trong khoảng [-1.5, 1.5]; đầu tiếng phải đủ to; cuối tiếng (sau 0.18s) phải gần im lặng.\n"
            "Có sẵn: sin, exp, PI, nhieu() (nhiễu trắng -1..1).\n"
            "> Qua thử thách: mỗi cú đấm trong võ đài phát ra đúng âm thanh bạn tạo.";
        C[15].starter =
            "float MauAmThanh(float t) {\n"
            "    // Hiện tại: tiếng \"bíp\" 440Hz kéo dài mãi, không tắt dần.\n"
            "    // TODO: tần số thấp hơn (100-200Hz), nhân với exp(-t * k) để tắt dần,\n"
            "    //       cộng thêm một chút nhiễu nhieu() cho tiếng \"bốp\".\n"
            "    return sin(2 * PI * 440 * t);\n"
            "}\n";
        C[15].solution =
            "float MauAmThanh(float t) {\n"
            "    float tanSo = 70 + 110 * exp(-t * 18);           // trượt từ cao xuống thấp\n"
            "    float thump = sin(2 * PI * tanSo * t) * exp(-t * 22);\n"
            "    float bop = nhieu() * exp(-t * 40) * 0.6f;       // tiếng va chạm\n"
            "    return thump + bop;\n"
            "}\n";
        C[15].hints = { "Đường bao tắt dần: nhân với exp(-t * 20).", "Tiếng đấm \"bụp\": sin(2 * PI * 120 * t) * exp(-t * 20) + nhieu() * exp(-t * 40) * 0.5" };
        C[15].usedIn = "Âm thanh cú đấm";
        C[15].tests = [](Program& p, std::vector<TestResult>& o) {
            if (!Require(p, o, "MauAmThanh", "MauAmThanh(...)")) return;
            const int RATE = 22050;
            double maxAll = 0, maxStart = 0, maxEnd = 0;
            bool finite = true;
            try {
                for (int i = 0; i < (int)(0.22 * RATE); ++i) {
                    double t = (double)i / RATE;
                    double v = p.Call("MauAmThanh", { F(t) }).AsFloat();
                    if (!std::isfinite(v)) finite = false;
                    double a = std::fabs(v);
                    maxAll = std::fmax(maxAll, a);
                    if (t < 0.03) maxStart = std::fmax(maxStart, a);
                    if (t > 0.18) maxEnd = std::fmax(maxEnd, a);
                }
            } catch (const Error& e) {
                o.push_back({ "MauAmThanh(t)", false, ErrText(e) });
                return;
            }
            o.push_back({ "Giá trị hợp lệ (không NaN/vô cực)", finite, finite ? "✓" : "có giá trị không hợp lệ (chia cho 0?)" });
            o.push_back({ "Không quá to (|mẫu| <= 1.5)", maxAll <= 1.5, "lớn nhất = " + Fmt(maxAll) });
            o.push_back({ "Đầu tiếng đủ to (>= 0.2)", maxStart >= 0.2, "lớn nhất 0.03s đầu = " + Fmt(maxStart) });
            o.push_back({ "Tắt dần (sau 0.18s < 0.15)", maxEnd < 0.15,
                          "lớn nhất sau 0.18s = " + Fmt(maxEnd) + (maxEnd < 0.15 ? "" : "\nGợi ý: nhân với exp(-t * 20) để tiếng tắt dần") });
        };
        return C;
    }
}

const Challenge& GetChallenge(int lesson) {
    static const std::vector<Challenge> C = Build();
    if (lesson < 0) lesson = 0;
    if (lesson >= (int)C.size()) lesson = (int)C.size() - 1;
    return C[lesson];
}

std::string FileNameForLesson(int lesson) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "code/bai_%02d.%s", lesson + 1, GetChallenge(lesson).glsl ? "glsl" : "cpp");
    return buf;
}

bool RunChallenge(int lesson, const std::string& source, std::vector<Error>& compileErrors,
                  std::vector<TestResult>& results, std::string& consoleOut) {
    results.clear();
    consoleOut.clear();
    const Challenge& ch = GetChallenge(lesson);
    Program p;
    p.SetStepLimit(2000000);
    if (!p.Compile(source, compileErrors)) return false;
    ch.tests(p, results);
    if (ch.glsl) {                                    // biên dịch thật trên GPU
        std::string err;
        bool ok = gfx::TestDiffuse(source, err);
        results.push_back({ "Biên dịch shader trên card đồ họa", ok, ok ? "✓ GPU chấp nhận code của bạn" : err });
    }
    consoleOut = p.Output();
    if (results.empty()) return false;
    for (const TestResult& r : results) if (!r.pass) return false;
    return true;
}
