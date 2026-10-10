// =============================================================================
//  BÀI 5 — KẾ THỪA & ĐA HÌNH
//  Chạy: g++ -std=c++17 05_ke_thua_da_hinh.cpp -o bai05 && ./bai05
// =============================================================================
#include <iostream>
#include <string>

// Lớp cha trừu tượng: "bản hợp đồng" mà mọi nhân vật phải tuân theo
class Character {
public:
    virtual ~Character() = default;              // BẮT BUỘC có virtual destructor
    virtual std::string Ten() const = 0;         // thuần ảo -> lớp con phải viết
    virtual float TocDo() const { return 4.0f; } // ảo, có mặc định
    virtual int   LucDam() const { return 8; }
    virtual int   MauToiDa() const { return 100; }

    // Hàm thường dùng chung, gọi các hàm ảo bên trong
    void GioiThieu() const {
        std::cout << Ten() << ": toc do " << TocDo() << ", luc dam " << LucDam()
                  << ", mau " << MauToiDa() << "\n";
    }
    // Đặc chiêu: mỗi lớp con làm khác nhau
    virtual void DacChieu() const { std::cout << "  (khong co dac chieu)\n"; }
};

class Ninja : public Character {
public:
    std::string Ten() const override { return "Ninja"; }
    float TocDo() const override { return 5.6f; }
    int   LucDam() const override { return 6; }
    int   MauToiDa() const override { return 90; }
    void  DacChieu() const override { std::cout << "  Phi tieu! Vut vut vut!\n"; }
};

class Robot : public Character {
public:
    std::string Ten() const override { return "Robot"; }
    float TocDo() const override { return 3.0f; }
    int   LucDam() const override { return 11; }
    int   MauToiDa() const override { return 130; }
    void  DacChieu() const override { std::cout << "  Ban laser! Bzzzt!\n"; }
};

// Hàm nhận THAM CHIẾU lớp cha -> làm việc được với MỌI lớp con
void ThuSuc(const Character& c) {
    c.GioiThieu();
    c.DacChieu();          // gọi đúng phiên bản của lớp con (đa hình)
}

int main() {
    Ninja n;
    Robot r;
    ThuSuc(n);
    ThuSuc(r);

    // Mảng con trỏ lớp cha trỏ tới các lớp con
    const Character* doiHinh[] = { &n, &r };
    int tongLuc = 0;
    for (const Character* c : doiHinh) tongLuc += c->LucDam();
    std::cout << "Tong luc dam ca doi: " << tongLuc << "\n";

    // Character c;   // LỖI: không tạo được lớp trừu tượng. Thử bỏ comment!

    // BÀI TẬP: thêm lớp Samurai (toc do 4.2, luc dam 8) với đặc chiêu riêng.
    return 0;
}
