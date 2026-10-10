// =============================================================================
//  BÀI 4 — STRUCT & CLASS (ĐÓNG GÓI)
//  Chạy: g++ -std=c++17 04_class.cpp -o bai04 && ./bai04
// =============================================================================
#include <iostream>
#include <string>

// struct: gom dữ liệu đơn giản (mặc định public)
struct Vec2 {
    float x = 0, y = 0;
};

// class: dữ liệu private + hàm thành viên public
class Fighter {
public:
    // Constructor: chạy khi tạo đối tượng. ": ten_(ten) ..." là danh sách khởi tạo
    Fighter(std::string ten, int mauToiDa, int lucDam)
        : ten_(ten), mau_(mauToiDa), mauToiDa_(mauToiDa), lucDam_(lucDam) {
        std::cout << "[+] Tao vo si " << ten_ << "\n";
    }
    // Destructor: chạy khi đối tượng bị hủy
    ~Fighter() { std::cout << "[-] Huy vo si " << ten_ << "\n"; }

    void Do(bool bat) { dangDo_ = bat; }

    void NhanDon(int dmg) {
        if (dangDo_) dmg /= 4;
        mau_ -= dmg;
        if (mau_ < 0) mau_ = 0;          // đóng gói: máu không bao giờ âm
    }

    void Danh(Fighter& doiThu) const {
        std::cout << ten_ << " dam " << doiThu.ten_ << "!\n";
        doiThu.NhanDon(lucDam_);
    }

    // Hàm "const": hứa không sửa đối tượng
    int  Mau() const { return mau_; }
    bool ConSong() const { return mau_ > 0; }
    const std::string& Ten() const { return ten_; }

    Vec2 viTri;                          // thành viên public kiểu struct

private:
    std::string ten_;
    int  mau_;
    int  mauToiDa_;
    int  lucDam_;
    bool dangDo_ = false;
};

int main() {
    Fighter minh("Minh", 100, 9);
    Fighter bot("Bot", 80, 7);
    minh.viTri = { -3.0f, 0.0f };
    bot.viTri  = {  3.0f, 0.0f };

    // minh.mau_ = -999;   // LỖI biên dịch: mau_ là private. Thử bỏ comment xem!

    int luot = 0;
    while (minh.ConSong() && bot.ConSong()) {
        Fighter& tanCong = (luot % 2 == 0) ? minh : bot;
        Fighter& phongThu = (luot % 2 == 0) ? bot : minh;
        phongThu.Do(luot % 3 == 0);          // thỉnh thoảng đỡ đòn
        tanCong.Danh(phongThu);
        std::cout << "   " << minh.Ten() << ": " << minh.Mau()
                  << "  |  " << bot.Ten() << ": " << bot.Mau() << "\n";
        luot++;
    }
    std::cout << (minh.ConSong() ? minh.Ten() : bot.Ten()) << " THANG!\n";

    // BÀI TẬP: thêm hàm HoiMau(int luong) không cho vượt mauToiDa_.
    return 0;   // khi main kết thúc, destructor của minh và bot tự chạy
}
