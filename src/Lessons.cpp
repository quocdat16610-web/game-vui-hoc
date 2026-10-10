// =============================================================================
//  Lessons.cpp — NỘI DUNG HỌC. Muốn thêm bài, chỉ cần thêm 1 phần tử vào L.
//  (Đây cũng là ví dụ về "data-driven": nội dung tách khỏi code xử lý.)
// =============================================================================
#include "Lessons.h"

static std::vector<Lesson> BuildLessons() {
    std::vector<Lesson> L;

    // -------------------------------------------------------------------------
    // BÀI 1 — BIẾN & KIỂU DỮ LIỆU
    // -------------------------------------------------------------------------
    L.push_back({
        "Cơ bản", "Biến & kiểu dữ liệu", "lessons/01_bien_va_kieu.cpp",
        "Ngày 1. Mình là Minh, dev indie. Mình quyết định làm một game đối kháng 3D tên là \"Code Võ Đài\" "
        "bằng C++. Võ sĩ nào cũng cần máu, sát thương, tốc độ... Trong C++, những con số đó được cất trong BIẾN. "
        "Bắt đầu từ đây nhé!",
        {
            { "Biến là gì?",
              "Biến là một chiếc hộp có TÊN, chứa một GIÁ TRỊ. Mỗi biến có một KIỂU, cho biết hộp đó chứa loại dữ liệu gì.\n"
              "# Các kiểu hay dùng trong game\n"
              "- int: số nguyên (máu, điểm, số hiệp)\n"
              "- float: số thực (tốc độ, vị trí, thời gian). Viết kèm chữ f: 4.5f\n"
              "- bool: đúng/sai (đang đỡ đòn? đang nhảy?)\n"
              "- char: một ký tự ('J')\n"
              "- std::string: chuỗi ký tự (tên võ sĩ), cần #include <string>\n"
              "> Đặt tên biến rõ nghĩa: mau, tocDo, dangDo tốt hơn a, b, c rất nhiều.",
              R"CODE(#include <iostream>
#include <string>

int main() {
    std::string ten = "Ninja";  // chuỗi ký tự
    int   mau     = 100;        // số nguyên
    float tocDo   = 4.5f;       // số thực (có chữ f)
    bool  dangDo  = false;      // đúng / sai
    char  phimDam = 'J';        // 1 ký tự

    std::cout << ten << " co " << mau << " HP\n";
    return 0;
})CODE" },
            { "Phép toán & hằng số",
              "Game liên tục tính toán: trừ máu, cộng điểm, tính phần trăm để vẽ thanh máu.\n"
              "- const: hằng số, gán một lần rồi không sửa được nữa\n"
              "- -=, +=, ++: cách viết gọn\n"
              "! Chia hai số nguyên sẽ bỏ phần thập phân: 77 / 100 = 0. Muốn ra 0.77 phải ép sang float trước.\n"
              "> Thanh máu trong võ đài được vẽ đúng bằng công thức tiLe = (float)mau / mauToiDa.",
              R"CODE(const int MAU_TOI_DA = 100;  // hằng số
int mau = MAU_TOI_DA;
int satThuong = 12;

mau = mau - satThuong;   // 88
mau -= satThuong;        // 76 (viết gọn)
mau++;                   // 77

float tiLe = (float)mau / MAU_TOI_DA;  // 0.77
// Vẽ thanh máu dài 77% chiều rộng!)CODE" },
        },
        {
            { "Kiểu nào hợp nhất để lưu tốc độ di chuyển 4.5?",
              { "int", "float", "bool", "char" }, 1,
              "float lưu được phần thập phân. int sẽ cắt 4.5 thành 4." },
            { "int a = 7; int b = 2; Khi đó a / b bằng bao nhiêu?",
              { "3.5", "3", "4", "Lỗi biên dịch" }, 1,
              "Hai số nguyên chia nhau cho kết quả nguyên, phần .5 bị bỏ đi. Muốn 3.5 thì viết (float)a / b." },
            { "Từ khóa nào làm một biến không thể bị thay đổi sau khi gán?",
              { "static", "const", "auto", "return" }, 1,
              "const = hằng. Trình biên dịch sẽ báo lỗi nếu bạn lỡ tay sửa nó." },
        },
        "Thanh máu (HP bar)",
        "Võ đài sẽ vẽ thanh máu theo tỉ lệ mau / mauToiDa (mở võ đài ở bài 2).",
        "Mở file ví dụ, thêm biến giap (giáp) kiểu int. Sát thương thực nhận = satThuong - giap, nhưng không được nhỏ hơn 1.",
        "src/Arena.cpp: hàm DrawHud() tính hp / maxHp để vẽ thanh máu."
    });

    // -------------------------------------------------------------------------
    // BÀI 2 — RẼ NHÁNH & VÒNG LẶP
    // -------------------------------------------------------------------------
    L.push_back({
        "Cơ bản", "Rẽ nhánh & vòng lặp (game loop)", "lessons/02_re_nhanh_vong_lap.cpp",
        "Có biến rồi, nhưng game phải biết RA QUYẾT ĐỊNH (hết máu thì K.O.) và phải CHẠY LIÊN TỤC. "
        "Bí mật của mọi game: một vòng lặp khổng lồ chạy 60 lần mỗi giây!",
        {
            { "if / else / switch",
              "if cho phép chương trình rẽ nhánh theo điều kiện.\n"
              "- So sánh: ==  !=  <  >  <=  >=\n"
              "- Logic: && (VÀ), || (HOẶC), ! (KHÔNG)\n"
              "- switch hợp với việc chọn theo 1 giá trị (phím bấm, trạng thái)\n"
              "! Đừng nhầm = (gán) với == (so sánh). if (mau = 0) là một lỗi kinh điển!",
              R"CODE(if (mau <= 0) {
    std::cout << "K.O.!\n";
} else if (mau < 30) {
    std::cout << "Sap thua roi!\n";
} else {
    std::cout << "Van khoe\n";
}

switch (phim) {
    case 'J': std::cout << "Dam!\n"; break;
    case 'K': std::cout << "Da!\n";  break;
    default: break;   // phím khác: bỏ qua
})CODE" },
            { "Vòng lặp & GAME LOOP",
              "Vòng lặp lặp lại một khối lệnh. Mọi game đều có một GAME LOOP gồm 3 bước, chạy mỗi frame:\n"
              "- 1. Input: đọc bàn phím, chuột, tay cầm\n"
              "- 2. Update: cập nhật vị trí, máu, va chạm...\n"
              "- 3. Render: vẽ mọi thứ ra màn hình\n"
              "for dùng khi biết trước số lần lặp (3 hiệp), while khi lặp tới lúc điều kiện sai.\n"
              "> Game bạn đang chơi (file src/main.cpp) cũng chỉ là một vòng while như thế này.",
              R"CODE(for (int hiep = 1; hiep <= 3; hiep++) {
    std::cout << "Hiep " << hiep << "\n";
}

bool dangChay = true;
while (dangChay) {           // GAME LOOP
    DocPhim();               // 1. Input
    CapNhat();               // 2. Update
    Ve();                    // 3. Render
    if (mauP1 <= 0 || mauP2 <= 0)
        dangChay = false;    // có người K.O.
})CODE" },
        },
        {
            { "Thứ tự đúng của ba bước trong game loop là gì?",
              { "Vẽ → Input → Update", "Input → Update → Vẽ", "Update → Vẽ → Input", "Chỉ cần Vẽ" }, 1,
              "Đọc input trước để Update dùng được, cuối cùng vẽ kết quả mới nhất." },
            { "for (int i = 0; i < 3; i++) lặp bao nhiêu lần?",
              { "2 lần", "3 lần", "4 lần", "Vô hạn" }, 1,
              "i nhận 0, 1, 2. Đến i = 3 thì điều kiện i < 3 sai nên dừng." },
            { "Điều kiện \"còn sống VÀ còn thời gian\" viết thế nào?",
              { "conSong && thoiGian > 0", "conSong || thoiGian > 0", "conSong = thoiGian", "!conSong && thoiGian" }, 0,
              "&& yêu cầu cả hai vế đều đúng. || chỉ cần một vế." },
        },
        "Game loop: võ đài chạy được",
        "Bước lên bục võ đài trong văn phòng và nhấn E. Đối thủ lúc này mới là bao cát đứng yên.",
        "Trong file ví dụ, viết vòng lặp đánh theo lượt: mỗi lượt hai bên trừ máu nhau, in ra máu, dừng khi có người K.O.",
        "src/main.cpp: vòng while (!WindowShouldClose()) chính là game loop."
    });

    // -------------------------------------------------------------------------
    // BÀI 3 — HÀM
    // -------------------------------------------------------------------------
    L.push_back({
        "Cơ bản", "Hàm (function)", "lessons/03_ham.cpp",
        "main() của mình đã dài 500 dòng, sửa chỗ này hỏng chỗ kia. Mình cần chia nhỏ code thành HÀM, "
        "mỗi hàm làm đúng một việc: tính sát thương, kiểm tra K.O., vẽ thanh máu...",
        {
            { "Khai báo & gọi hàm",
              "Hàm gồm: kiểu trả về, tên, danh sách tham số và thân hàm.\n"
              "- void: hàm không trả về gì\n"
              "- return: trả kết quả và thoát hàm ngay\n"
              "- Có thể đặt giá trị mặc định cho tham số: int Dam(int luc = 8)\n"
              "> Quy tắc: một hàm nên ngắn, có tên là động từ, và chỉ làm một việc.",
              R"CODE(// kiểu_trả_về  Tên(tham_số)
int TinhSatThuong(int lucDanh, bool biDo) {
    if (biDo) return lucDanh / 4;  // bị đỡ: giảm 75%
    return lucDanh;
}

bool DaKO(int mau) { return mau <= 0; }

int main() {
    int d1 = TinhSatThuong(20, false);  // 20
    int d2 = TinhSatThuong(20, true);   // 5
})CODE" },
            { "Tham trị & tham chiếu",
              "Mặc định C++ truyền BẢN SAO (tham trị): hàm sửa bản sao, biến gốc không đổi.\n"
              "Thêm dấu & để truyền THAM CHIẾU: hàm làm việc trực tiếp trên biến gốc.\n"
              "- const T& : truyền tham chiếu nhưng không cho sửa, dùng cho dữ liệu lớn (string, vector) để khỏi sao chép\n"
              "> Trong game, Fighter được truyền bằng tham chiếu Fighter& vào các hàm Update.",
              R"CODE(void NhanDon(int mau, int dmg) {      // bản SAO
    mau -= dmg;   // không ảnh hưởng bên ngoài!
}
void NhanDonDung(int& mau, int dmg) { // THAM CHIẾU
    mau -= dmg;   // sửa biến gốc
}

int hp = 100;
NhanDon(hp, 10);      // hp vẫn 100 :(
NhanDonDung(hp, 10);  // hp = 90   :))CODE" },
        },
        {
            { "Hàm không trả về giá trị nào thì có kiểu trả về là gì?",
              { "int", "void", "null", "empty" }, 1,
              "void nghĩa là \"không có gì\"." },
            { "Muốn hàm sửa được biến gốc của người gọi, tham số nên khai báo thế nào?",
              { "int x", "int& x", "const int x", "int x = 0" }, 1,
              "int& là tham chiếu: x chính là biến gốc, không phải bản sao." },
            { "Lợi ích chính của việc chia code thành nhiều hàm?",
              { "Chương trình chạy nhanh gấp đôi", "Tái sử dụng, dễ đọc, dễ sửa lỗi", "Không cần khai báo biến nữa", "C++ bắt buộc phải vậy" }, 1,
              "Hàm giúp tổ chức code. Tốc độ gần như không đổi." },
        },
        "Đòn đấm (phím J)",
        "Vào võ đài, đứng gần bao cát và nhấn J để đấm.",
        "Viết hàm bool ChiMang(int phanTram) trả về true theo xác suất phanTram (dùng rand() % 100). Đòn chí mạng gây gấp đôi sát thương.",
        "src/Arena.cpp: hàm ApplyHit() tính sát thương, có xét đỡ đòn và combo."
    });

    // -------------------------------------------------------------------------
    // BÀI 4 — STRUCT & CLASS
    // -------------------------------------------------------------------------
    L.push_back({
        "Cơ bản", "Struct & Class (đóng gói)", "lessons/04_class.cpp",
        "Mỗi võ sĩ có cả chục biến: tên, máu, vị trí, đang đỡ... Hai võ sĩ là hai chục biến lộn xộn. "
        "Mình sẽ gom chúng lại thành một kiểu dữ liệu mới: class Fighter.",
        {
            { "struct: gom dữ liệu",
              "struct (và class) cho phép tự tạo kiểu dữ liệu chứa nhiều biến thành viên.\n"
              "- Truy cập thành viên bằng dấu chấm: p1.mau\n"
              "- Có thể gán giá trị mặc định ngay khi khai báo\n"
              "- Mỗi biến kiểu Fighter là một ĐỐI TƯỢNG riêng, có dữ liệu riêng",
              R"CODE(struct Fighter {
    std::string ten;
    int   mau = 100;
    float x   = 0.0f;
};

Fighter p1;              // tạo đối tượng
p1.ten = "Minh";
p1.x = -3.0f;

Fighter p2{"Bot", 100, 3.0f};  // khởi tạo nhanh)CODE" },
            { "class: đóng gói (encapsulation)",
              "class thêm khả năng ẨN dữ liệu (private) và chỉ cho thao tác qua HÀM THÀNH VIÊN (method).\n"
              "- public: ai cũng gọi được\n"
              "- private: chỉ bên trong class dùng được\n"
              "- Constructor: hàm cùng tên class, tự chạy khi tạo đối tượng\n"
              "- const sau tên hàm: hàm hứa không sửa đối tượng\n"
              "> Nhờ đóng gói, không ai có thể viết mau_ = -999. Mọi thay đổi đi qua NhanDon(), nơi luôn kiểm tra hợp lệ.",
              R"CODE(class Fighter {
public:
    Fighter(std::string ten, int mau)   // constructor
        : ten_(ten), mau_(mau) {}

    void Do(bool bat) { dangDo_ = bat; }
    void NhanDon(int dmg) {
        if (dangDo_) dmg /= 4;
        mau_ -= dmg;
        if (mau_ < 0) mau_ = 0;  // không âm máu
    }
    int  Mau() const    { return mau_; }
    bool ConSong() const { return mau_ > 0; }

private:              // bên ngoài không sửa bừa được
    std::string ten_;
    int  mau_;
    bool dangDo_ = false;
};)CODE" },
        },
        {
            { "Khác biệt mặc định giữa struct và class?",
              { "struct mặc định public, class mặc định private", "struct mặc định private, class mặc định public", "Hoàn toàn giống nhau", "struct không có hàm thành viên" }, 0,
              "Đó là khác biệt duy nhất. Thường dùng struct cho dữ liệu đơn giản, class khi cần đóng gói." },
            { "Hàm tự động chạy khi một đối tượng được tạo ra gọi là gì?",
              { "Destructor", "Constructor", "Operator", "main" }, 1,
              "Constructor khởi tạo đối tượng. Destructor (~Fighter) chạy khi đối tượng bị hủy." },
            { "Vì sao nên để biến mau_ là private?",
              { "Để chạy nhanh hơn", "Để mọi thay đổi phải đi qua NhanDon() và luôn hợp lệ", "Vì C++ bắt buộc", "Để tiết kiệm RAM" }, 1,
              "Đóng gói bảo vệ \"bất biến\" của đối tượng, ví dụ máu không bao giờ âm." },
        },
        "Đá (K) & Đỡ đòn (L)",
        "Trong võ đài: K để đá (chậm hơn nhưng mạnh), giữ L để đỡ (giảm 75% sát thương).",
        "Thêm vào class Fighter hàm HoiMau(int luong) không cho máu vượt quá mauToiDa_. Thêm destructor in ra \"Fighter bi huy\".",
        "src/Arena.h: struct Fighter chứa toàn bộ dữ liệu một võ sĩ."
    });

    // -------------------------------------------------------------------------
    // BÀI 5 — KẾ THỪA & ĐA HÌNH
    // -------------------------------------------------------------------------
    L.push_back({
        "Cơ bản", "Kế thừa & đa hình", "lessons/05_ke_thua_da_hinh.cpp",
        "Mình muốn ba nhân vật: Ninja nhanh nhẹn, Samurai cân bằng, Robot trâu bò. Copy class ba lần rồi sửa? "
        "Không! Dùng KẾ THỪA: phần chung để ở lớp cha, phần riêng ở lớp con.",
        {
            { "Kế thừa (inheritance)",
              "Lớp con kế thừa mọi thứ của lớp cha và có thể GHI ĐÈ (override) hàm ảo.\n"
              "- virtual: cho phép lớp con ghi đè\n"
              "- override: báo cho trình biên dịch kiểm tra là mình đang ghi đè thật\n"
              "- = 0: hàm thuần ảo, lớp chứa nó là lớp TRỪU TƯỢNG (không tạo trực tiếp được)",
              R"CODE(class Character {                    // lớp cha
public:
    virtual ~Character() = default;
    virtual std::string Ten() const = 0; // thuần ảo
    virtual float TocDo() const { return 4.0f; }
    virtual int   LucDam() const { return 8; }
};

class Ninja : public Character {     // lớp con
public:
    std::string Ten() const override { return "Ninja"; }
    float TocDo() const override { return 6.0f; }
    int   LucDam() const override { return 6; }
};

class Robot : public Character {
public:
    std::string Ten() const override { return "Robot"; }
    float TocDo() const override { return 2.8f; }
    int   LucDam() const override { return 13; }
};)CODE" },
            { "Đa hình (polymorphism)",
              "Một tham chiếu (hoặc con trỏ) kiểu lớp cha có thể trỏ tới bất kỳ lớp con nào. Khi gọi hàm virtual, "
              "C++ tự chọn ĐÚNG phiên bản của lớp con lúc chạy.\n"
              "> Nhờ vậy code võ đài chỉ cần biết Character. Thêm nhân vật thứ 4 không phải sửa code võ đài.\n"
              "! Lớp cha phải có virtual destructor, nếu không delete qua con trỏ lớp cha sẽ không hủy đúng lớp con.",
              R"CODE(void GioiThieu(const Character& c) {
    // Gọi đúng hàm của lớp con dù kiểu là Character&
    std::cout << c.Ten() << " toc do "
              << c.TocDo() << "\n";
}

Ninja n;
Robot r;
GioiThieu(n);   // Ninja toc do 6
GioiThieu(r);   // Robot toc do 2.8)CODE" },
        },
        {
            { "Từ khóa nào cho phép lớp con ghi đè hàm và được gọi đúng qua tham chiếu lớp cha?",
              { "static", "virtual", "const", "inline" }, 1,
              "virtual bật cơ chế gọi hàm động (dynamic dispatch)." },
            { "Một class có hàm \"= 0\" thì sao?",
              { "Chạy nhanh hơn", "Là lớp trừu tượng, không tạo đối tượng trực tiếp được", "Bị lỗi biên dịch", "Tự động thành struct" }, 1,
              "Lớp trừu tượng đóng vai trò \"bản hợp đồng\" mà lớp con phải thực hiện." },
            { "Vì sao lớp cha cần virtual destructor?",
              { "Để delete qua con trỏ lớp cha gọi đúng destructor lớp con", "Cho code đẹp hơn", "Để chạy nhanh hơn", "Không cần thiết" }, 0,
              "Thiếu nó, phần dữ liệu của lớp con có thể không được giải phóng." },
        },
        "Chọn nhân vật (Ninja / Samurai / Robot)",
        "Mỗi lần vào võ đài sẽ có màn chọn nhân vật, mỗi nhân vật có chỉ số khác nhau.",
        "Tạo lớp con thứ tư: Samurai (tốc độ 4.2, lực đấm 9). Đưa cả 4 nhân vật vào một vòng for để giới thiệu.",
        "src/Arena.cpp: hàm GetCharacterStats() trả về chỉ số từng nhân vật."
    });

    // -------------------------------------------------------------------------
    // BÀI 6 — CON TRỎ & BỘ NHỚ
    // -------------------------------------------------------------------------
    L.push_back({
        "Trung cấp", "Con trỏ, bộ nhớ & smart pointer", "lessons/06_con_tro_bo_nho.cpp",
        "Mỗi cú đấm trúng bắn ra 30 tia lửa. Chúng sinh ra rồi biến mất liên tục. "
        "Muốn làm điều đó mà không làm game rò rỉ bộ nhớ, mình phải hiểu C++ quản lý bộ nhớ ra sao.",
        {
            { "Con trỏ, stack và heap",
              "- Stack: biến cục bộ, tự hủy khi ra khỏi hàm. Nhanh nhưng nhỏ\n"
              "- Heap: cấp phát bằng new, tồn tại tới khi delete\n"
              "- Con trỏ (T*) lưu ĐỊA CHỈ của một biến. &x lấy địa chỉ, *p truy cập giá trị\n"
              "- Dùng -> để gọi thành viên qua con trỏ\n"
              "! new mà quên delete = rò rỉ bộ nhớ (memory leak). delete hai lần = crash.",
              R"CODE(int mau = 100;
int* p = &mau;     // p giữ ĐỊA CHỈ của mau
*p = 50;           // sửa qua con trỏ -> mau == 50

Fighter* f = new Fighter("Bot", 100); // HEAP
f->NhanDon(10);    // dùng -> với con trỏ
delete f;          // QUÊN dòng này = rò rỉ!
f = nullptr;       // tránh dùng lại con trỏ cũ)CODE" },
            { "Smart pointer & RAII",
              "C++ hiện đại hầu như không viết new/delete trực tiếp. Thay vào đó dùng smart pointer:\n"
              "- std::unique_ptr: một chủ sở hữu duy nhất, tự delete khi ra khỏi phạm vi\n"
              "- std::shared_ptr: nhiều chủ, đếm tham chiếu (dùng khi thật sự cần)\n"
              "> RAII: tài nguyên (bộ nhớ, file, texture) gắn với vòng đời một đối tượng. Đối tượng hủy thì tài nguyên được giải phóng.",
              R"CODE(#include <memory>

std::unique_ptr<Character> p1 = std::make_unique<Ninja>();
auto p2 = std::make_unique<Robot>();

p1->TocDo();       // đa hình qua con trỏ
// không cần delete!

std::vector<std::unique_ptr<Character>> roster;
roster.push_back(std::make_unique<Ninja>());
roster.push_back(std::move(p2)); // chuyển quyền sở hữu)CODE" },
            { "Áp dụng: hệ thống hạt",
              "Tia lửa là đối tượng nhỏ, sống ngắn. Cách tốt nhất là lưu theo GIÁ TRỊ trong std::vector: "
              "dữ liệu nằm liền nhau trong bộ nhớ, không cần new/delete từng hạt.\n"
              "> erase + remove_if là cách chuẩn để xóa các phần tử thỏa điều kiện khỏi vector.",
              R"CODE(struct Particle { Vector3 pos, vel; float life; };
std::vector<Particle> sparks;   // lưu GIÁ TRỊ

void SpawnSparks(Vector3 at) {
    for (int i = 0; i < 30; i++)
        sparks.push_back({at, RandomVel(), 0.5f});
}
void UpdateSparks(float dt) {
    for (auto& s : sparks) {
        s.pos = Vector3Add(s.pos, Vector3Scale(s.vel, dt));
        s.life -= dt;
    }
    sparks.erase(std::remove_if(sparks.begin(), sparks.end(),
        [](const Particle& s) { return s.life <= 0; }),
        sparks.end());
})CODE" },
        },
        {
            { "Biểu thức &x cho ta cái gì?",
              { "Giá trị của x", "Địa chỉ của x trong bộ nhớ", "x nhân 2", "Một bản sao của x" }, 1,
              "& đứng trước biến là toán tử lấy địa chỉ. (Khác với & trong khai báo tham chiếu int& r.)" },
            { "Cấp phát bằng new mà quên delete sẽ gây ra gì?",
              { "Lỗi biên dịch", "Rò rỉ bộ nhớ (memory leak)", "Game chạy nhanh hơn", "Không sao cả" }, 1,
              "Bộ nhớ bị giữ mãi. Game chạy lâu sẽ ngốn RAM dần rồi có thể crash." },
            { "std::unique_ptr khác con trỏ thường ở điểm nào?",
              { "Tự giải phóng bộ nhớ khi ra khỏi phạm vi", "Chậm hơn 10 lần", "Không dùng được toán tử ->", "Chỉ dùng được cho int" }, 0,
              "unique_ptr gần như không tốn thêm chi phí mà loại bỏ hẳn lỗi quên delete." },
        },
        "Hiệu ứng tia lửa (particles)",
        "Đánh trúng đối thủ sẽ thấy tia lửa bắn ra.",
        "Trong file ví dụ, thêm trọng lực cho hạt (vel.y -= 9.8 * dt) và in số hạt còn sống mỗi frame.",
        "src/Arena.cpp: các hàm SpawnSparks() và UpdateParticles()."
    });

    // -------------------------------------------------------------------------
    // BÀI 7 — STL
    // -------------------------------------------------------------------------
    L.push_back({
        "Trung cấp", "Thư viện chuẩn STL", "lessons/07_stl.cpp",
        "Mình muốn có combo: đánh trúng liên tiếp thì sát thương tăng, kèm bảng thống kê cuối trận. "
        "Không cần tự viết cấu trúc dữ liệu: thư viện chuẩn C++ (STL) có sẵn cả rồi.",
        {
            { "std::vector: mảng động",
              "vector là container dùng nhiều nhất trong game: danh sách đạn, quái, hạt, combo...\n"
              "- push_back(x): thêm vào cuối\n"
              "- size(): số phần tử,  v[i]: truy cập phần tử i\n"
              "- clear(): xóa hết\n"
              "- for (auto& x : v): duyệt từng phần tử (range-for)",
              R"CODE(#include <vector>

std::vector<int> combo;      // mảng động
combo.push_back(8);          // trúng đòn 8 dmg
combo.push_back(12);
std::cout << combo.size();   // 2

int tong = 0;
for (int dmg : combo) tong += dmg;   // 20

combo.clear();   // bị ngắt combo -> reset)CODE" },
            { "std::map: tra cứu theo khóa",
              "map lưu cặp khóa → giá trị, tự sắp xếp theo khóa. Rất hợp để đếm thống kê.\n"
              "- m[key]: nếu chưa có key thì tự tạo với giá trị mặc định (0)\n"
              "- unordered_map: nhanh hơn khi không cần thứ tự\n"
              "> auto& [ten, soLan] là \"structured binding\" của C++17, tách cặp thành hai biến.",
              R"CODE(#include <map>

std::map<std::string, int> thongKe;
thongKe["Dam"]++;     // chưa có -> tạo 0 rồi ++
thongKe["Da"] += 1;
thongKe["Dam"]++;

for (const auto& [ten, soLan] : thongKe)
    std::cout << ten << ": " << soLan << "\n";
// Da: 1
// Dam: 2)CODE" },
            { "<algorithm> & lambda",
              "<algorithm> có sẵn sort, find, count_if, max_element... Kết hợp với LAMBDA (hàm vô danh viết tại chỗ) rất mạnh.\n"
              "- [] là phần \"bắt\" biến bên ngoài, ví dụ [&] bắt tất cả theo tham chiếu",
              R"CODE(#include <algorithm>

std::vector<int> diem = {120, 80, 300, 95};
std::sort(diem.begin(), diem.end(),
          [](int a, int b) { return a > b; }); // giảm dần
// 300 120 95 80

int maxDmg = *std::max_element(diem.begin(), diem.end());
int soDonManh = std::count_if(diem.begin(), diem.end(),
          [](int d) { return d >= 100; });     // 2)CODE" },
        },
        {
            { "Container nào hợp để tra \"tên đòn\" → \"số lần dùng\"?",
              { "std::vector<int>", "std::map<std::string, int>", "int[10]", "std::string" }, 1,
              "map ánh xạ khóa (tên đòn) sang giá trị (số lần)." },
            { "v.push_back(x) làm gì?",
              { "Xóa x khỏi v", "Thêm x vào cuối v", "Sắp xếp v", "Tìm x trong v" }, 1,
              "push_back thêm vào cuối, vector tự nới rộng khi cần." },
            { "[](int a, int b) { return a > b; } là gì?",
              { "Một lambda (hàm vô danh)", "Một mảng", "Một macro", "Lỗi cú pháp" }, 0,
              "Lambda thường được truyền vào các thuật toán như sort." },
        },
        "Combo + bảng thống kê cuối trận",
        "Đánh trúng liên tiếp (cách nhau dưới 1.2 giây) để tăng combo, mỗi hit cộng thêm 10% sát thương.",
        "Dùng std::map để đếm số lần mỗi đòn TRÚNG và TRƯỢT, rồi in tỉ lệ chính xác của từng đòn.",
        "src/Arena.cpp: Fighter::stats là std::map<std::string,int>, combo được tính trong ApplyHit()."
    });

    // -------------------------------------------------------------------------
    // BÀI 8 — DELTA TIME
    // -------------------------------------------------------------------------
    L.push_back({
        "Trung cấp", "Game loop chuẩn & delta time", "lessons/08_delta_time.cpp",
        "Bạn mình chơi thử trên màn 144Hz: nhân vật chạy nhanh gấp đôi! Đây là lỗi kinh điển của người mới: "
        "quên DELTA TIME. Hôm nay mình sửa nó, rồi làm luôn đòn lướt.",
        {
            { "Delta time là gì?",
              "dt (delta time) là thời gian trôi qua giữa hai frame, tính bằng giây.\n"
              "- 60 FPS → dt ≈ 0.0167s,   144 FPS → dt ≈ 0.0069s\n"
              "- Nhân mọi chuyển động với dt thì tốc độ tính theo GIÂY, không theo frame\n"
              "> raylib: float dt = GetFrameTime();",
              R"CODE(// SAI: tốc độ phụ thuộc FPS
x += 0.1f;   // 60 FPS: 6 m/s, 144 FPS: 14.4 m/s!

// ĐÚNG: nhân với thời gian giữa 2 frame
float dt = GetFrameTime();   // ví dụ 0.0167
x += tocDo * dt;             // luôn đúng tocDo m/s)CODE" },
            { "Fixed timestep",
              "Vật lý và va chạm thích bước thời gian CỐ ĐỊNH để kết quả ổn định. Kỹ thuật \"accumulator\": cộng dồn thời gian thật, "
              "rồi chạy vật lý theo từng bước 1/60 giây.\n"
              "! Nên giới hạn dt (ví dụ tối đa 0.1s), nếu không khi game bị khựng thì nhân vật có thể bay xuyên tường.",
              R"CODE(const float STEP = 1.0f / 60.0f;
float tichLuy = 0.0f;

while (!WindowShouldClose()) {
    tichLuy += GetFrameTime();
    while (tichLuy >= STEP) {   // chạy 0, 1 hoặc 2 lần
        CapNhatVatLy(STEP);     // dt cố định
        tichLuy -= STEP;
    }
    Ve();
})CODE" },
            { "Bộ đếm thời gian: đòn lướt",
              "Rất nhiều thứ trong game là BỘ ĐẾM: thời gian hồi chiêu, thời gian choáng, thời gian bất tử... "
              "Mẫu chung: gán giá trị, trừ dt mỗi frame, hết thì dừng.",
              R"CODE(float dashTimer = 0, dashCooldown = 0;

if (IsKeyPressed(KEY_LEFT_SHIFT) && dashCooldown <= 0) {
    dashTimer = 0.18f;      // lướt trong 0.18s
    dashCooldown = 0.8f;    // hồi chiêu 0.8s
}
if (dashTimer > 0) {
    x += huong * 18.0f * dt;
    dashTimer -= dt;
}
dashCooldown -= dt;)CODE" },
        },
        {
            { "Game chạy 144 FPS thì dt xấp xỉ bao nhiêu?",
              { "0.144 giây", "0.0069 giây", "1 giây", "144 giây" }, 1,
              "dt = 1 / 144 ≈ 0.0069 giây." },
            { "Viết x += tocDo * dt đảm bảo điều gì?",
              { "FPS cao hơn", "Tốc độ di chuyển không phụ thuộc FPS", "Tốn ít RAM hơn", "Không cần game loop" }, 1,
              "Máy nhanh hay chậm thì sau 1 giây nhân vật vẫn đi đúng tocDo đơn vị." },
            { "Fixed timestep hữu ích nhất cho việc gì?",
              { "Vật lý & va chạm ổn định", "Vẽ chữ", "Đọc file save", "Phát nhạc" }, 0,
              "Bước thời gian cố định cho kết quả vật lý giống nhau trên mọi máy." },
        },
        "Lướt (phím Shift)",
        "Trong võ đài nhấn Shift để lướt nhanh về phía đang di chuyển, hồi chiêu 0.8 giây.",
        "Thêm đòn \"đấm tích lực\": giữ J càng lâu (tối đa 1 giây) thì sát thương càng lớn. Dùng một biến đếm thời gian giữ phím.",
        "src/Arena.cpp: UpdateFighter() nhận dt, mọi chuyển động đều nhân dt."
    });

    // -------------------------------------------------------------------------
    // BÀI 9 — TOÁN VECTOR 3D
    // -------------------------------------------------------------------------
    L.push_back({
        "Trung cấp", "Toán vector 3D & vật lý cơ bản", "lessons/09_vector_3d.cpp",
        "Võ sĩ của mình cứ quay lưng về phía đối thủ khi đi qua người nó. Còn muốn nhảy thì cần trọng lực. "
        "Tất cả quy về một thứ: VECTOR.",
        {
            { "Vector & nạp chồng toán tử",
              "Vector 3D gồm (x, y, z): biểu diễn vị trí, vận tốc, hướng. C++ cho phép NẠP CHỒNG toán tử để viết a + b như toán học.\n"
              "- Length: độ dài vector (khoảng cách)\n"
              "- Normalize: vector cùng hướng, dài 1\n"
              "- Dot: tích vô hướng, > 0 khi hai hướng cùng phía\n"
              "> raylib có sẵn những hàm này trong raymath.h: Vector3Add, Vector3Length, Vector3DotProduct...",
              R"CODE(struct Vec3 { float x, y, z; };

Vec3 operator+(Vec3 a, Vec3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
Vec3 operator-(Vec3 a, Vec3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
Vec3 operator*(Vec3 a, float s) { return {a.x*s, a.y*s, a.z*s}; }

float Length(Vec3 v) {
    return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}
Vec3 Normalize(Vec3 v) { return v * (1.0f / Length(v)); }
float Dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; })CODE" },
            { "Quay mặt về đối thủ",
              "Lấy vị trí đối thủ trừ vị trí mình ra vector hướng. Dấu của thành phần x cho biết đối thủ ở bên trái hay phải.",
              R"CODE(Vec3 huong = Normalize(doiThu.pos - toi.pos);
toi.facing = (huong.x >= 0) ? 1.0f : -1.0f;

float khoangCach = Length(doiThu.pos - toi.pos);

// Đối thủ có ở PHÍA TRƯỚC mặt mình không?
Vec3 truoc = {toi.facing, 0, 0};
bool oTruoc = Dot(truoc, huong) > 0.0f;)CODE" },
            { "Trọng lực & nhảy (tích phân Euler)",
              "Gia tốc thay đổi vận tốc, vận tốc thay đổi vị trí. Mỗi frame:\n"
              "- vel += gravity * dt\n"
              "- pos += vel * dt\n"
              "> Muốn nhảy chỉ cần đặt vận tốc y dương khi đang chạm đất. Trọng lực lo phần còn lại.",
              R"CODE(const float TRONG_LUC = -30.0f;

if (IsKeyPressed(KEY_W) && dangChamDat) {
    vel.y = 11.0f;           // bật lên
    dangChamDat = false;
}
vel.y += TRONG_LUC * dt;     // gia tốc -> vận tốc
pos = pos + vel * dt;        // vận tốc -> vị trí
if (pos.y <= 0) {            // chạm đất
    pos.y = 0; vel.y = 0; dangChamDat = true;
})CODE" },
        },
        {
            { "Với hai vector đã chuẩn hóa, Dot(a, b) > 0 nghĩa là gì?",
              { "Chúng cùng phía (góc giữa < 90°)", "Chúng vuông góc", "Chúng ngược hướng hoàn toàn", "Chúng bằng nhau" }, 0,
              "Dot = cos(góc). Dương khi góc < 90°, bằng 0 khi vuông góc, âm khi > 90°." },
            { "Normalize(v) trả về gì?",
              { "Vector dài 1, cùng hướng với v", "Vector 0", "Độ dài của v", "v nhân 2" }, 0,
              "Vector đơn vị chỉ mang thông tin HƯỚNG." },
            { "Thứ tự cập nhật nào đúng cho trọng lực?",
              { "vel += gravity*dt; rồi pos += vel*dt", "pos = gravity", "vel = pos * dt", "pos += gravity (không cần dt)" }, 0,
              "Đây là tích phân Euler (bán ẩn), đủ tốt cho hầu hết game." },
        },
        "Nhảy (W / Space) + tự quay mặt",
        "Nhấn W hoặc Space để nhảy. Đi vòng qua đối thủ, võ sĩ sẽ tự quay mặt lại.",
        "Thêm \"nhảy hai lần\" (double jump): cho phép nhảy thêm 1 lần khi đang ở trên không.",
        "src/Arena.cpp: trong UpdateFighter() có trọng lực GRAVITY và đoạn tính facing."
    });

    // -------------------------------------------------------------------------
    // BÀI 10 — INPUT & MÁY TRẠNG THÁI
    // -------------------------------------------------------------------------
    L.push_back({
        "Trung cấp", "Input & máy trạng thái (FSM)", "lessons/10_may_trang_thai.cpp",
        "Võ sĩ của mình vừa đấm vừa đá vừa đỡ cùng một lúc, nhìn như bị nhập. "
        "Lời giải mà game đối kháng nào cũng dùng: MÁY TRẠNG THÁI HỮU HẠN (FSM).",
        {
            { "enum class & trạng thái",
              "FSM: tại mỗi thời điểm nhân vật ở đúng MỘT trạng thái. Mỗi trạng thái quyết định được làm gì và chuyển sang đâu.\n"
              "- enum class: liệt kê trạng thái, an toàn kiểu hơn enum thường\n"
              "- stateTime: đã ở trạng thái hiện tại bao lâu, dùng cho hoạt ảnh và thời gian đòn",
              R"CODE(enum class State { Idle, Walk, Punch, Kick, Block, Hurt, KO };

struct Input {          // tách INPUT khỏi logic
    int  moveX = 0;     // -1 trái, 0 đứng, 1 phải
    bool punch = false, kick = false, block = false;
};

struct Fighter {
    State state = State::Idle;
    float stateTime = 0;    // đã ở state này bao lâu
    void ChangeState(State s) { state = s; stateTime = 0; }
};)CODE" },
            { "Cập nhật FSM bằng switch",
              "Trong lúc đang đấm, nhân vật KHÔNG nhận lệnh mới. Đây là \"cam kết\" (commitment), thứ tạo nên chiều sâu cho game đối kháng.",
              R"CODE(void Update(Fighter& f, const Input& in, float dt) {
    f.stateTime += dt;
    switch (f.state) {
    case State::Idle:
    case State::Walk:
        if (in.punch)      f.ChangeState(State::Punch);
        else if (in.block) f.ChangeState(State::Block);
        else f.state = in.moveX ? State::Walk : State::Idle;
        break;
    case State::Punch:       // đang đấm: khóa input
        if (f.stateTime > 0.35f) f.ChangeState(State::Idle);
        break;
    case State::Hurt:        // choáng (hitstun)
        if (f.stateTime > 0.30f) f.ChangeState(State::Idle);
        break;
    default: break;
    }
})CODE" },
            { "Frame data & hoạt ảnh",
              "Một đòn đánh có 3 pha:\n"
              "- Startup: vung tay, chưa gây sát thương\n"
              "- Active: hitbox BẬT, có thể trúng\n"
              "- Recovery: thu tay về, dễ bị phản đòn\n"
              "> Hoạt ảnh đơn giản cũng tính từ stateTime: góc tay = sin(tiến độ × π).",
              R"CODE(// startup  0.00-0.10s | active 0.10-0.20s | recovery 0.20-0.35s
bool HitboxActive(const Fighter& f) {
    return f.state == State::Punch &&
           f.stateTime >= 0.10f && f.stateTime < 0.20f;
}

float tienDo = f.stateTime / 0.35f;          // 0 -> 1
float gocTay = sinf(tienDo * PI) * 90.0f;    // 0 -> 90 -> 0 độ)CODE" },
        },
        {
            { "Lợi ích chính của máy trạng thái (FSM)?",
              { "Mỗi lúc chỉ có một trạng thái hợp lệ, chuyển trạng thái rõ ràng", "Tăng FPS", "Giảm dung lượng game", "Tự vẽ model 3D" }, 0,
              "FSM loại bỏ các tổ hợp vô lý như vừa đấm vừa đỡ." },
            { "Trong pha \"recovery\" của một đòn đánh, điều gì đúng?",
              { "Hitbox đang bật", "Nhân vật đang thu đòn và dễ bị phản đòn", "Nhân vật bất tử", "Game tạm dừng" }, 1,
              "Đánh hụt thì recovery là lúc đối thủ trừng phạt bạn." },
            { "Vì sao tách struct Input ra khỏi Fighter?",
              { "Để người chơi và AI dùng chung toàn bộ logic Fighter", "Vì C++ bắt buộc", "Để vẽ nhanh hơn", "Để lưu game" }, 0,
              "Bàn phím tạo Input, AI cũng tạo Input. Fighter không cần biết ai đang điều khiển." },
        },
        "Hoạt ảnh tay chân + choáng khi trúng đòn",
        "Giờ võ sĩ vung tay, đá chân thật sự, bị trúng đòn sẽ lảo đảo (hitstun).",
        "Thêm trạng thái Taunt (khiêu khích, phím T) kéo dài 1 giây, trong lúc đó không làm gì được.",
        "src/Arena.h: enum class FState. src/Arena.cpp: UpdateFighter() là một FSM dùng switch."
    });

    // -------------------------------------------------------------------------
    // BÀI 11 — VA CHẠM AABB
    // -------------------------------------------------------------------------
    L.push_back({
        "Nâng cao", "Va chạm: hitbox & hurtbox (AABB)", "lessons/11_va_cham_aabb.cpp",
        "Lỗi buồn cười: mình đấm ra sau lưng mà bao cát vẫn mất máu, vì mình chỉ đo khoảng cách! "
        "Game đối kháng thật dùng HITBOX (vùng gây đòn) và HURTBOX (vùng nhận đòn).",
        {
            { "AABB: hộp thẳng trục",
              "AABB (Axis-Aligned Bounding Box) là hộp chữ nhật có cạnh song song các trục. Kiểm tra va chạm cực nhanh: "
              "hai hộp chồng nhau khi và chỉ khi chúng chồng nhau trên CẢ BA trục.\n"
              "> raylib có sẵn: BoundingBox và CheckCollisionBoxes().",
              R"CODE(struct AABB { Vec3 min, max; };

bool Overlap(const AABB& a, const AABB& b) {
    return a.min.x <= b.max.x && a.max.x >= b.min.x &&
           a.min.y <= b.max.y && a.max.y >= b.min.y &&
           a.min.z <= b.max.z && a.max.z >= b.min.z;
})CODE" },
            { "Hitbox vs hurtbox",
              "- Hurtbox: thân người, luôn tồn tại\n"
              "- Hitbox: nắm đấm/bàn chân, chỉ bật trong pha active\n"
              "- Cờ daTrung: một đòn chỉ được gây sát thương MỘT lần\n"
              "> Trong võ đài, nhấn H để xem hitbox (đỏ) và hurtbox (xanh) thật của game!",
              R"CODE(AABB Hurtbox(const Fighter& f) {       // thân người
    return {{f.pos.x - 0.4f, f.pos.y,        -0.4f},
            {f.pos.x + 0.4f, f.pos.y + 1.9f,  0.4f}};
}
AABB PunchBox(const Fighter& f) {      // nắm đấm
    float cx = f.pos.x + f.facing * 0.9f;
    return {{cx - 0.25f, f.pos.y + 1.2f, -0.3f},
            {cx + 0.25f, f.pos.y + 1.6f,  0.3f}};
}

if (HitboxActive(a) && !a.daTrung &&
    Overlap(PunchBox(a), Hurtbox(b))) {
    b.NhanDon(a.LucDam());
    a.daTrung = true;   // mỗi đòn chỉ trúng 1 lần
})CODE" },
            { "Phản hồi va chạm: đẩy nhau",
              "Phát hiện va chạm mới là một nửa. Nửa còn lại là XỬ LÝ: hai võ sĩ không được đi xuyên qua nhau, "
              "nên khi chồng lấn thì đẩy mỗi người ra một nửa phần chồng lấn.",
              R"CODE(float d = b.pos.x - a.pos.x;
const float MIN_DIST = 0.8f;
if (std::fabs(d) < MIN_DIST) {
    float day = (MIN_DIST - std::fabs(d)) / 2;
    float dau = (d >= 0) ? 1.0f : -1.0f;
    a.pos.x -= dau * day;
    b.pos.x += dau * day;
}
// không cho ra khỏi sàn đấu
a.pos.x = std::clamp(a.pos.x, -7.0f, 7.0f);)CODE" },
        },
        {
            { "Hai AABB va chạm khi nào?",
              { "Khi chồng nhau trên CẢ BA trục x, y, z", "Chỉ cần chồng nhau trên trục x", "Khi tâm trùng nhau", "Khi cùng kích thước" }, 0,
              "Chỉ cần tách nhau trên một trục là không va chạm (định lý trục phân tách)." },
            { "Cờ daTrung dùng để làm gì?",
              { "Đảm bảo một đòn chỉ gây sát thương một lần", "Đếm combo", "Vẽ hitbox", "Tăng tốc độ" }, 0,
              "Pha active kéo dài nhiều frame. Thiếu cờ này, một cú đấm trừ máu 6 lần liền!" },
            { "Hurtbox là gì?",
              { "Vùng có thể BỊ đánh trúng", "Vùng gây sát thương", "Một loại camera", "Hộp chứa âm thanh" }, 0,
              "Hitbox đánh, hurtbox chịu đòn." },
        },
        "Hitbox chính xác + đẩy nhau (phím H)",
        "Giờ chỉ đòn trúng thật mới gây sát thương. Nhấn H trong võ đài để bật/tắt hiển thị hitbox.",
        "Thêm đòn \"đấm thấp\" với hitbox ở độ cao 0.3 - 0.7 mét. Kiểm tra: nó có trúng đối thủ đang nhảy không?",
        "src/Arena.cpp: HurtBox(), AttackBox() và đoạn \"đẩy nhau\" trong UpdateArena()."
    });

    // -------------------------------------------------------------------------
    // BÀI 12 — AI ĐỐI THỦ
    // -------------------------------------------------------------------------
    L.push_back({
        "Nâng cao", "AI đối thủ", "lessons/12_ai_doi_thu.cpp",
        "Đánh bao cát mãi cũng chán. Đến lúc cho Bot một \"bộ não\": nó sẽ tiếp cận, tấn công, lùi lại và biết đỡ đòn. "
        "Điều thú vị là AI chỉ cần tạo ra... một struct Input!",
        {
            { "Bộ não: chọn chế độ",
              "AI đơn giản cũng là một FSM, nhưng chuyển trạng thái theo QUAN SÁT thế trận: khoảng cách, đối thủ đang làm gì.\n"
              "- Ra quyết định theo nhịp (0.25s) để giả lập thời gian phản xạ của con người\n"
              "- Thêm ngẫu nhiên để AI khó đoán",
              R"CODE(enum class AIMode { TiepCan, LuiLai, TanCong, PhongThu };

struct AI { AIMode mode = AIMode::TiepCan; float timer = 0; };

void Think(AI& ai, const Fighter& me, const Fighter& foe, float dt) {
    ai.timer -= dt;
    if (ai.timer > 0) return;        // chưa đến lúc nghĩ
    ai.timer = 0.25f;                // thời gian phản xạ
    float d = std::fabs(foe.pos.x - me.pos.x);
    if (foe.state == State::Punch && d < 2.0f)
        ai.mode = AIMode::PhongThu;
    else if (d > 1.8f)
        ai.mode = AIMode::TiepCan;
    else
        ai.mode = (Rand01() < 0.7f) ? AIMode::TanCong
                                    : AIMode::LuiLai;
})CODE" },
            { "Thực thi: AI tạo ra Input",
              "Vì Fighter chỉ hiểu Input, AI tạo Input y như bàn phím. Toàn bộ di chuyển, va chạm, hoạt ảnh được dùng lại 100%.\n"
              "> Độ khó = thời gian phản xạ + tỉ lệ đỡ đòn + độ hung hăng. AI không nên hoàn hảo: game phải VUI, không phải khó tuyệt đối.\n"
              "> Nâng cao hơn: Behavior Tree, Utility AI, GOAP... nhưng FSM vẫn là nền tảng.",
              R"CODE(Input Act(const AI& ai, const Fighter& me, const Fighter& foe) {
    Input in;
    int huong = (foe.pos.x > me.pos.x) ? 1 : -1;
    switch (ai.mode) {
    case AIMode::TiepCan:  in.moveX = huong;  break;
    case AIMode::LuiLai:   in.moveX = -huong; break;
    case AIMode::PhongThu: in.block = true;   break;
    case AIMode::TanCong:
        if (Rand01() < 0.5f) in.punch = true;
        else                 in.kick  = true;
        break;
    }
    return in;   // giống hệt Input từ bàn phím!
})CODE" },
        },
        {
            { "Vì sao AI ra quyết định mỗi 0.25s thay vì mỗi frame?",
              { "Giả lập thời gian phản xạ như người và tránh giật cục", "Vì CPU quá yếu", "Vì C++ bắt buộc", "Để AI luôn thua" }, 0,
              "AI phản ứng tức thì trong 1 frame sẽ đỡ được mọi đòn: không công bằng và không vui." },
            { "AI trả về struct Input thì có lợi gì?",
              { "Dùng lại toàn bộ logic Fighter giống người chơi", "AI chạy trên GPU", "Không cần FSM nữa", "Không cần kiểm tra va chạm" }, 0,
              "Một Fighter, hai \"bộ điều khiển\": bàn phím và AI." },
            { "Cách tăng độ khó AI hợp lý nhất?",
              { "Giảm thời gian phản xạ và tăng tỉ lệ đỡ đòn", "Cho AI gấp 10 lần máu", "Cho AI bất tử", "Cho AI đọc trước phím bạn bấm" }, 0,
              "Độ khó tốt đến từ hành vi thông minh hơn, không phải từ gian lận." },
        },
        "Đối thủ AI biết đánh trả",
        "Bao cát đã biến thành Bot thật sự: tiếp cận, tấn công, đỡ đòn và lùi lại.",
        "Thêm chế độ AIMode::PhanDon: khi người chơi đánh hụt (đang recovery) và ở gần, AI lập tức đấm.",
        "src/Arena.cpp: hàm AIThink() và AIAct()."
    });

    // -------------------------------------------------------------------------
    // BÀI 13 — CAMERA 3D & GAME FEEL VỚI RAYLIB
    // -------------------------------------------------------------------------
    L.push_back({
        "Nâng cao", "raylib: camera 3D & game feel", "lessons/13_camera_raylib.cpp",
        "Gameplay ổn rồi nhưng đánh chưa \"đã tay\". Bí quyết của các game đối kháng lớn là GAME FEEL: "
        "camera bám theo trận đấu, màn hình rung và khựng hình một chút mỗi khi trúng đòn.",
        {
            { "Khung chương trình raylib",
              "raylib là thư viện C/C++ đơn giản để làm game. Game này viết bằng raylib!\n"
              "- InitWindow mở cửa sổ, SetTargetFPS giới hạn FPS\n"
              "- BeginMode3D/EndMode3D: vẽ thế giới 3D qua Camera3D\n"
              "- Vẽ UI 2D SAU EndMode3D để nó nằm trên cùng",
              R"CODE(#include "raylib.h"

int main() {
    InitWindow(1280, 720, "Code Vo Dai");
    SetTargetFPS(60);
    Camera3D cam = {};
    cam.position = {0, 4, 12};  cam.target = {0, 1.5f, 0};
    cam.up = {0, 1, 0};  cam.fovy = 45;
    cam.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(SKYBLUE);
        BeginMode3D(cam);
            DrawCube({0, 0.9f, 0}, 0.8f, 1.8f, 0.5f, RED);
            DrawGrid(20, 1.0f);
        EndMode3D();
        DrawText("FIGHT!", 20, 20, 40, WHITE); // UI 2D
        EndDrawing();
    }
    CloseWindow();
})CODE" },
            { "Camera động & rung màn hình",
              "Lerp (nội suy tuyến tính) giúp camera đuổi theo mục tiêu một cách mượt mà thay vì giật cục.\n"
              "- Lerp(a, b, t) = a + (b - a) * t\n"
              "- Rung: cộng một độ lệch ngẫu nhiên, giảm dần về 0",
              R"CODE(// Nhìn vào điểm giữa 2 võ sĩ, lùi xa khi họ xa nhau
Vector3 giua = Vector3Lerp(p1.pos, p2.pos, 0.5f);
float xa = 9.0f + fabsf(p1.pos.x - p2.pos.x) * 0.6f;
Vector3 dich = {giua.x, 3.5f, xa};
cam.position = Vector3Lerp(cam.position, dich, 5.0f * dt);
cam.target = Vector3Lerp(cam.target, {giua.x, 1.4f, 0}, 5.0f * dt);

if (shake > 0) {          // rung màn hình
    cam.position.x += GetRandomValue(-100, 100) / 100.0f * shake;
    cam.position.y += GetRandomValue(-100, 100) / 100.0f * shake;
    shake -= dt * 2.0f;
})CODE" },
            { "Hit-stop",
              "Hit-stop: dừng logic game vài phần trăm giây khi trúng đòn. Não người cảm nhận đó là cú đánh \"nặng\". "
              "Trick nhỏ nhưng game đối kháng nào cũng dùng.",
              R"CODE(float hitStop = 0, shake = 0;
void OnHit(bool manh) {
    hitStop = manh ? 0.10f : 0.06f;
    shake   = manh ? 0.40f : 0.20f;
}

float dt = GetFrameTime();
if (hitStop > 0) { hitStop -= dt; dt = 0; } // logic đứng yên
UpdateGame(dt);
DrawGame();   // nhưng vẫn VẼ bình thường)CODE" },
        },
        {
            { "UI 2D (thanh máu, chữ) nên được vẽ khi nào?",
              { "Sau EndMode3D()", "Trong BeginMode3D()", "Trước BeginDrawing()", "Không thể vẽ 2D trong raylib" }, 0,
              "Vẽ sau cùng để UI đè lên cảnh 3D." },
            { "Lerp(a, b, 0.5) cho kết quả gì?",
              { "Điểm chính giữa a và b", "Đúng bằng a", "Đúng bằng b", "a nhân b" }, 0,
              "t = 0 ra a, t = 1 ra b, t = 0.5 ra điểm giữa." },
            { "Hit-stop là gì?",
              { "Dừng logic vài khung hình khi trúng đòn để tạo cảm giác lực", "Một lỗi lag", "Tắt âm thanh", "Đổi góc camera" }, 0,
              "Logic dừng nhưng vẫn vẽ, nên người chơi thấy khoảnh khắc va chạm rõ hơn." },
        },
        "Camera động + rung màn hình + hit-stop",
        "Camera giờ bám theo trận đấu, rung và khựng nhẹ mỗi khi có đòn trúng.",
        "Biên dịch và chạy lessons/13_camera_raylib.cpp. Thử đổi fovy và tốc độ Lerp để xem cảm giác camera thay đổi thế nào.",
        "src/Arena.cpp: UpdateCamera() và biến hitStop, shake."
    });

    // -------------------------------------------------------------------------
    // BÀI 14 — KIẾN TRÚC GAME
    // -------------------------------------------------------------------------
    L.push_back({
        "Nâng cao", "Kiến trúc game & tối ưu", "lessons/14_kien_truc.cpp",
        "Game lớn dần, main.cpp bắt đầu thành mì spaghetti. Bài cuối: cách các studio tổ chức code "
        "để game lớn mà vẫn dễ sửa: Scene, luật chơi dạng dữ liệu, ECS và tối ưu.",
        {
            { "Scene / Game State",
              "Mỗi \"màn\" (menu, chọn nhân vật, trận đấu, kết quả) là một Scene riêng. Game loop chỉ việc gọi Update/Draw của scene hiện tại. "
              "Đây chính là kế thừa + đa hình + unique_ptr bạn đã học!\n"
              "> Game này có enum class Screen { Title, Office, Lesson, Arena } trong src/Common.h, phiên bản đơn giản của ý tưởng đó.",
              R"CODE(class Scene {
public:
    virtual ~Scene() = default;
    virtual void Update(float dt) = 0;
    virtual void Draw() = 0;
};
class MenuScene  : public Scene { /* ... */ };
class FightScene : public Scene { /* ... */ };

std::unique_ptr<Scene> scene = std::make_unique<MenuScene>();
while (!WindowShouldClose()) {
    scene->Update(GetFrameTime());
    BeginDrawing(); scene->Draw(); EndDrawing();
    if (next) scene = std::move(next);   // đổi màn
})CODE" },
            { "Luật chơi là dữ liệu",
              "Đừng rải số 60, 3, 0.5 khắp code. Gom luật chơi vào một struct: dễ cân bằng game, dễ thêm chế độ chơi, thậm chí đọc từ file.\n"
              "- Phase của trận: Intro → Fight → RoundOver → MatchOver",
              R"CODE(struct MatchRules {          // dữ liệu tách khỏi logic
    int   hiepCanThang = 2;  // thắng 2 trong 3 hiệp
    float thoiGianHiep = 60;
    float doKhoAI      = 0.5f;
};
enum class Phase { Intro, Fight, RoundOver, MatchOver };

// Hết giờ: ai còn nhiều % máu hơn thì thắng hiệp
if (timer <= 0)
    winner = (p1.HpPct() > p2.HpPct()) ? 1 : 2;)CODE" },
            { "ECS & tối ưu hiệu năng",
              "ECS (Entity Component System) được dùng trong nhiều engine hiện đại:\n"
              "- Entity: chỉ là một ID\n"
              "- Component: dữ liệu thuần (Position, Health)\n"
              "- System: hàm xử lý mọi entity có component phù hợp\n"
              "Dữ liệu cùng loại nằm liền nhau → CPU cache hoạt động hiệu quả.\n"
              "> Quy tắc vàng: ĐO trước (profiler), tối ưu sau. Tránh new/delete trong game loop, dùng object pool.",
              R"CODE(struct Position { float x, y, z; };
struct Health   { int hp, maxHp; };

std::vector<Position> positions;  // liền nhau trong RAM
std::vector<Health>   healths;    // CPU cache rất thích

void HealthSystem(std::vector<Health>& hs) {
    for (auto& h : hs)
        if (h.hp < 0) h.hp = 0;
})CODE" },
        },
        {
            { "Chia game thành các Scene giúp ích gì?",
              { "Tách menu, trận đấu... thành lớp riêng, dễ thêm màn mới", "Tự động tăng FPS", "Thay thế hoàn toàn class", "Bỏ được game loop" }, 0,
              "Mỗi scene tự lo Update/Draw của mình, game loop gọn gàng." },
            { "Trong ECS, Component chứa gì?",
              { "Dữ liệu", "Toàn bộ logic game", "Cửa sổ game", "Shader" }, 0,
              "Logic nằm trong System. Component chỉ là dữ liệu." },
            { "Quy tắc vàng khi tối ưu hiệu năng?",
              { "Đo (profile) trước, rồi mới tối ưu chỗ chậm nhất", "Tối ưu mọi dòng code ngay từ đầu", "Luôn viết bằng assembly", "Bỏ hết class" }, 0,
              "Thường 90% thời gian chạy nằm ở 10% code. Đo để tìm đúng 10% đó." },
        },
        "Hiệp đấu 2/3, đồng hồ, độ khó, tạm dừng",
        "Trận đấu giờ có 3 hiệp, đồng hồ 60 giây, chọn độ khó AI và tạm dừng bằng P.",
        "Thêm Scene \"Luyện tập\": máu đối thủ tự hồi đầy sau 2 giây không bị đánh, hiển thị sát thương mỗi đòn.",
        "src/Arena.cpp: enum class Phase và struct MatchRules."
    });

    return L;
}

const std::vector<Lesson>& GetLessons() {
    static const std::vector<Lesson> lessons = BuildLessons();
    return lessons;
}
