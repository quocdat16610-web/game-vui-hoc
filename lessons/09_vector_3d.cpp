// =============================================================================
//  BÀI 9 — TOÁN VECTOR 3D & VẬT LÝ CƠ BẢN
//  Chạy: g++ -std=c++17 09_vector_3d.cpp -o bai09 && ./bai09
// =============================================================================
#include <cmath>
#include <iostream>

struct Vec3 {
    float x = 0, y = 0, z = 0;
};

// Nạp chồng toán tử: viết a + b như toán học
Vec3 operator+(Vec3 a, Vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
Vec3 operator-(Vec3 a, Vec3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
Vec3 operator*(Vec3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }
std::ostream& operator<<(std::ostream& os, Vec3 v) {
    return os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
}

float Length(Vec3 v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
Vec3 Normalize(Vec3 v) {
    float l = Length(v);
    return (l > 0) ? v * (1.0f / l) : Vec3{};
}
float Dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

int main() {
    Vec3 toi = { -2, 0, 0 };
    Vec3 doiThu = { 3, 0, 1 };

    Vec3 huong = Normalize(doiThu - toi);
    std::cout << "Khoang cach: " << Length(doiThu - toi) << "\n";
    std::cout << "Huong toi doi thu: " << huong << "  (do dai = " << Length(huong) << ")\n";

    float facing = (huong.x >= 0) ? 1.0f : -1.0f;
    std::cout << "Quay mat ve: " << (facing > 0 ? "phai" : "trai") << "\n";

    // Dot product: đối thủ có ở phía trước không?
    Vec3 truoc = { facing, 0, 0 };
    std::cout << "Dot = " << Dot(truoc, huong) << " -> "
              << (Dot(truoc, huong) > 0 ? "o PHIA TRUOC" : "o PHIA SAU") << "\n";
    Vec3 nhinNguoc = { -1, 0, 0 };
    std::cout << "Neu quay lung: Dot = " << Dot(nhinNguoc, huong) << " -> phia sau\n\n";

    // ---------- Nhảy với trọng lực (tích phân Euler) ----------
    const float TRONG_LUC = -30.0f, DT = 1.0f / 60.0f;
    Vec3 pos = { 0, 0, 0 }, vel = { 2.0f, 10.0f, 0 };   // nhảy chéo về phía trước
    bool chamDat = false;
    float cao = 0;
    int frame = 0;
    std::cout << "Nhay:\n";
    while (!chamDat) {
        vel.y += TRONG_LUC * DT;          // gia tốc -> vận tốc
        pos = pos + vel * DT;             // vận tốc -> vị trí
        if (pos.y > cao) cao = pos.y;
        if (pos.y <= 0) { pos.y = 0; vel = {}; chamDat = true; }
        if (++frame % 6 == 0) {
            std::cout << "  t=" << frame * DT << "s  ";
            for (int i = 0; i < (int)(pos.y * 10); i++) std::cout << ' ';
            std::cout << "o\n";
        }
    }
    std::cout << "Do cao lon nhat: " << cao << " m, tiep dat tai x = " << pos.x << "\n";

    // BÀI TẬP: cho phép "nhảy hai lần" khi đang ở trên không.
    return 0;
}
