// =============================================================================
//  BÀI 3 — HÀM
//  Chạy: g++ -std=c++17 03_ham.cpp -o bai03 && ./bai03
//
//  Mục tiêu: tách logic chiến đấu thành các hàm nhỏ, hiểu tham trị/tham chiếu.
// =============================================================================
#include <iostream>
#include <string>

// Hàm trả về giá trị
int TinhSatThuong(int lucDanh, bool biDo) {
    if (biDo) return lucDanh / 4;     // bị đỡ: giảm 75%
    return lucDanh;
}

// Hàm trả về bool
bool DaKO(int mau) { return mau <= 0; }

// Tham số có giá trị mặc định
int Dam(int luc = 8) { return luc; }

// THAM TRỊ: hàm nhận bản sao -> không đổi biến gốc
void NhanDonSai(int mau, int dmg) {
    mau -= dmg;
    std::cout << "  (ben trong ham NhanDonSai: mau = " << mau << ")\n";
}

// THAM CHIẾU: hàm sửa trực tiếp biến gốc
void NhanDon(int& mau, int dmg) {
    mau -= dmg;
    if (mau < 0) mau = 0;
}

// const& : đọc dữ liệu lớn mà không sao chép, không cho sửa
void InThanhMau(const std::string& ten, int mau, int mauToiDa) {
    int o = mau * 20 / mauToiDa;
    std::cout << ten << " [";
    for (int i = 0; i < 20; i++) std::cout << (i < o ? '#' : '.');
    std::cout << "] " << mau << "\n";
}

// "Nạp chồng" (overload): cùng tên, khác tham số
int TongSatThuong(int a, int b) { return a + b; }
int TongSatThuong(int a, int b, int c) { return a + b + c; }

int main() {
    int hp = 100;

    std::cout << "Tham tri:\n";
    NhanDonSai(hp, 10);
    std::cout << "  hp sau khi goi = " << hp << "  (khong doi!)\n";

    std::cout << "Tham chieu:\n";
    NhanDon(hp, 10);
    std::cout << "  hp sau khi goi = " << hp << "\n\n";

    std::cout << "Dam thuong: " << TinhSatThuong(Dam(), false) << "\n";
    std::cout << "Dam manh bi do: " << TinhSatThuong(Dam(20), true) << "\n";
    std::cout << "Combo 3 don: " << TongSatThuong(8, 8, 12) << "\n\n";

    while (!DaKO(hp)) {
        NhanDon(hp, TinhSatThuong(Dam(17), false));
        InThanhMau("Bot", hp, 100);
    }
    std::cout << "K.O.!\n";

    // BÀI TẬP: viết hàm bool ChiMang(int phanTram) dùng rand() % 100.
    // Nếu chí mạng thì sát thương nhân đôi.
    return 0;
}
