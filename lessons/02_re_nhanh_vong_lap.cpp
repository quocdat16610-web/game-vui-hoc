// =============================================================================
//  BÀI 2 — RẼ NHÁNH & VÒNG LẶP
//  Chạy: g++ -std=c++17 02_re_nhanh_vong_lap.cpp -o bai02 && ./bai02
//
//  Mục tiêu: một trận đấu theo lượt chạy trong vòng lặp "game loop" đơn giản.
//  Bạn nhập phím J (đấm), K (đá), L (đỡ), Q (thoát).
// =============================================================================
#include <cstdlib>
#include <ctime>
#include <iostream>

int main() {
    std::srand((unsigned)std::time(nullptr));
    int mauBan = 100, mauBot = 100;
    int luot = 1;
    bool dangChay = true;

    // ======== GAME LOOP: Input -> Update -> Render ========
    while (dangChay) {
        // 1. INPUT
        std::cout << "\n--- Luot " << luot << " ---  Ban: " << mauBan << " HP   Bot: " << mauBot << " HP\n";
        std::cout << "Chon don (J = dam, K = da, L = do, Q = thoat): ";
        char phim;
        if (!(std::cin >> phim)) break;     // hết dữ liệu nhập (Ctrl+D / Ctrl+Z) -> thoát

        // 2. UPDATE
        bool banDo = false;
        switch (phim) {
            case 'j': case 'J': mauBot -= 8;  std::cout << "Ban DAM trung! -8\n";  break;
            case 'k': case 'K':
                if (std::rand() % 100 < 60) { mauBot -= 14; std::cout << "Ban DA trung! -14\n"; }
                else std::cout << "Da truot!\n";
                break;
            case 'l': case 'L': banDo = true; std::cout << "Ban giu the DO.\n"; break;
            case 'q': case 'Q': dangChay = false; continue;
            default: std::cout << "Phim khong hop le, mat luot!\n"; break;
        }

        // Bot đánh trả (nếu còn sống)
        if (mauBot > 0) {
            int dmg = 6 + std::rand() % 6;
            if (banDo) dmg /= 4;           // đỡ đòn giảm 75%
            mauBan -= dmg;
            std::cout << "Bot danh tra: -" << dmg << (banDo ? " (da do)" : "") << "\n";
        }

        // 3. "RENDER" + kiểm tra kết thúc
        if (mauBan <= 0 && mauBot <= 0) { std::cout << "HOA! Ca hai cung K.O.\n"; dangChay = false; }
        else if (mauBot <= 0)            { std::cout << "K.O.! BAN THANG!\n";      dangChay = false; }
        else if (mauBan <= 0)            { std::cout << "K.O.! Ban thua...\n";      dangChay = false; }
        else if (mauBan < 30)            { std::cout << "Canh bao: mau thap!\n"; }
        luot++;
    }

    // Vòng for: đếm ngược kiểu game đối kháng
    for (int i = 3; i >= 1; i--) std::cout << i << "... ";
    std::cout << "Het tran!\n";
    return 0;
}
