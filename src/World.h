// =============================================================================
//  World.h — THẾ GIỚI MỞ: "Đảo Lập Trình".
//  Một hòn đảo lục giác GẦN NHƯ TRỐNG: chỉ có đất, nước và Studio của Minh.
//  Mọi thứ còn lại do NGƯỜI CHƠI TỰ XÂY — bằng chuột (phím B) hoặc tự viết
//  code C++ gọi API Dat(q, r, VAT_THE)... (phím C). Game chỉ cung cấp kiến thức.
//  Nhà có dân đến ở (hỏi bài), Đấu trường & Thư viện xây xong thì đi vào được.
//
//  Kiến thức game trong file World.cpp:
//   - Lưới lục giác (tọa độ axial), đường đi tự nối (auto-tiling bằng bitmask)
//   - Sinh bản đồ thủ tục bằng value noise
//   - Nhúng trình thông dịch: game "cắm" hàm C++ cho code người chơi gọi
//   - Gộp lưới tĩnh (static batching) để giảm số lần vẽ
//   - Camera góc nhìn thứ ba xoay bằng chuột, va chạm với địa hình
//   - Chu kỳ ngày đêm: nội suy màu trời, hướng mặt trời, đèn đường
// =============================================================================
#pragma once
#include <string>

namespace world {
    enum class Action { None, EnterOffice, EnterArena, OpenLesson };

    bool Init();                 // false nếu thiếu file model
    void Shutdown();
    bool Ready();

    Action Update(float dt, bool titleMode);
    void RenderOffscreen();      // bóng đổ — gọi trước khi vẽ cảnh
    void Draw3D();
    void DrawHUD();

    void Say(const std::string& who, const std::string& msg, float seconds = 6.0f);
    void SpawnAtStudio();        // đặt nhân vật trước cửa Studio
    void SpawnAtArena();         // đặt nhân vật trước cổng Đấu trường
    bool WantsTextInput();       // đang gõ code trong Bản thiết kế
    void SetHour(float h);       // đặt giờ trong ngày (0..24)
}
