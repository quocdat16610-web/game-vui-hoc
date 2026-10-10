// =============================================================================
//  BÀI 1 — BIẾN & KIỂU DỮ LIỆU
//  Chạy: g++ -std=c++17 01_bien_va_kieu.cpp -o bai01 && ./bai01
//
//  Mục tiêu: khai báo các chỉ số của một võ sĩ, tính toán máu và tỉ lệ máu.
//  (Chuỗi in ra dùng chữ không dấu để hiển thị đúng trên mọi cửa sổ console.)
// =============================================================================
#include <iostream>
#include <string>

int main() {
    // --- Khai báo biến: kiểu  tên = giá_trị; ---
    std::string ten      = "Ninja";  // chuỗi ký tự
    const int   MAU_TOI_DA = 100;    // hằng số: không đổi được
    int         mau      = MAU_TOI_DA;
    float       tocDo    = 4.5f;     // số thực, nhớ chữ f
    bool        dangDo   = false;    // đúng / sai
    char        phimDam  = 'J';      // một ký tự

    std::cout << "=== THONG TIN VO SI ===\n";
    std::cout << "Ten:      " << ten << "\n";
    std::cout << "Mau:      " << mau << "/" << MAU_TOI_DA << "\n";
    std::cout << "Toc do:   " << tocDo << " m/s\n";
    std::cout << "Dang do:  " << (dangDo ? "co" : "khong") << "\n";
    std::cout << "Phim dam: " << phimDam << "\n\n";

    // --- Phép toán ---
    int satThuong = 12;
    mau = mau - satThuong;      // 88
    std::cout << "Trung don! Mau con " << mau << "\n";
    mau -= satThuong;           // 76
    std::cout << "Trung don! Mau con " << mau << "\n";
    mau++;                      // 77 (hồi 1 máu)
    std::cout << "Hoi 1 mau: " << mau << "\n\n";

    // --- Cạm bẫy chia số nguyên ---
    int saiLam = mau / MAU_TOI_DA;              // 0 !!!
    float dung = (float)mau / MAU_TOI_DA;       // 0.77
    std::cout << "Chia so nguyen: " << saiLam << "   (sai!)\n";
    std::cout << "Ep sang float:  " << dung << "  (dung)\n\n";

    // --- Vẽ thanh máu bằng ký tự: 20 ô ---
    int soO = (int)(dung * 20);
    std::cout << "HP [";
    for (int i = 0; i < 20; i++) std::cout << (i < soO ? '#' : '.');
    std::cout << "] " << (int)(dung * 100) << "%\n";

    // BÀI TẬP: thêm biến int giap = 5;
    // Sát thương thực = satThuong - giap, nhưng tối thiểu là 1.
    return 0;
}
