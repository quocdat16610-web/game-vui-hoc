// =============================================================================
//  BÀI 7 — THƯ VIỆN CHUẨN STL: vector, map, algorithm, lambda
//  Chạy: g++ -std=c++17 07_stl.cpp -o bai07 && ./bai07
// =============================================================================
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <map>
#include <string>
#include <vector>

int main() {
    std::srand((unsigned)std::time(nullptr));

    // ---------- vector: chuỗi combo ----------
    std::vector<int> combo;
    std::map<std::string, int> thongKe;          // tên đòn -> số lần
    std::vector<int> moiDon;                     // sát thương từng đòn trúng

    const std::string DON[3] = { "Dam", "Da", "Dam manh" };
    const int SAT_THUONG[3] = { 8, 12, 18 };

    std::cout << "Mo phong 15 don danh:\n";
    int comboMax = 0;
    for (int i = 0; i < 15; i++) {
        int loai = std::rand() % 3;
        bool trung = std::rand() % 100 < 70;
        thongKe[DON[loai]]++;                    // map tự tạo key nếu chưa có
        if (trung) {
            // combo: mỗi đòn trúng liên tiếp +10% sát thương
            int dmg = (int)(SAT_THUONG[loai] * (1.0f + 0.1f * combo.size()));
            combo.push_back(dmg);
            moiDon.push_back(dmg);
            thongKe["Trung"]++;
            std::cout << "  " << DON[loai] << " trung " << dmg << " (combo " << combo.size() << ")\n";
        } else {
            thongKe["Truot"]++;
            std::cout << "  " << DON[loai] << " TRUOT -> mat combo\n";
            comboMax = std::max(comboMax, (int)combo.size());
            combo.clear();
        }
    }
    comboMax = std::max(comboMax, (int)combo.size());

    // ---------- map: in thống kê (tự sắp theo key) ----------
    std::cout << "\nThong ke:\n";
    for (const auto& [ten, soLan] : thongKe)
        std::cout << "  " << ten << ": " << soLan << "\n";
    std::cout << "  Combo dai nhat: " << comboMax << "\n";

    // ---------- algorithm + lambda ----------
    if (!moiDon.empty()) {
        std::sort(moiDon.begin(), moiDon.end(), [](int a, int b) { return a > b; });
        int tong = 0;
        for (int d : moiDon) tong += d;
        int soDonManh = (int)std::count_if(moiDon.begin(), moiDon.end(), [](int d) { return d >= 15; });
        std::cout << "  Don manh nhat: " << moiDon.front() << "\n";
        std::cout << "  Tong sat thuong: " << tong << "\n";
        std::cout << "  So don >= 15: " << soDonManh << "\n";
    }

    // BÀI TẬP: đếm số lần TRÚNG và TRƯỢT riêng cho từng loại đòn
    // (gợi ý: std::map<std::string, std::pair<int,int>>), in tỉ lệ chính xác.
    return 0;
}
