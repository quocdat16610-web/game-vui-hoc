// =============================================================================
//  Common.h — những thứ dùng chung cho toàn bộ game
//  (màn hình hiện tại, tiến độ học, các "tính năng" của game đối kháng)
// =============================================================================
#pragma once
#include "raylib.h"
#include <string>

// Mỗi bài học hoàn thành sẽ mở khóa đúng 1 tính năng cho game đối kháng
// mà nhân vật dev đang viết. Thứ tự enum == thứ tự bài học.
enum Feature {
    F_HPBAR = 0,     // Bài 1:  biến & kiểu dữ liệu   -> thanh máu
    F_ARENA,         // Bài 2:  if / vòng lặp         -> game loop, sàn đấu chạy được
    F_PUNCH,         // Bài 3:  hàm                   -> đòn đấm
    F_KICK_BLOCK,    // Bài 4:  class                 -> đá + đỡ đòn
    F_CHARSELECT,    // Bài 5:  kế thừa & đa hình     -> chọn nhân vật
    F_PARTICLES,     // Bài 6:  con trỏ & bộ nhớ      -> hiệu ứng hạt
    F_COMBO,         // Bài 7:  STL                   -> combo + thống kê
    F_DASH,          // Bài 8:  delta time            -> lướt (dash)
    F_JUMP_FACING,   // Bài 9:  toán vector 3D        -> nhảy + tự quay mặt
    F_ANIM_STATES,   // Bài 10: máy trạng thái        -> hoạt ảnh + choáng
    F_HITBOX,        // Bài 11: va chạm AABB          -> hitbox chính xác, đẩy nhau
    F_AI,            // Bài 12: AI                    -> đối thủ biết đánh
    F_CAMERA_FX,     // Bài 13: camera & render       -> camera động, rung, hit-stop
    F_ROUNDS,        // Bài 14: kiến trúc game        -> hiệp đấu, đồng hồ, độ khó
    F_NEON_STAGE,    // Bài 15: shader & ánh sáng     -> sân khấu Neon ban đêm
    F_MUSIC,         // Bài 16: âm thanh              -> nhạc nền chiptune
    F_COUNT
};

enum class Screen { Title, World, Office, Lesson, Arena };

// Tiến độ người chơi, lưu ra file save.txt
struct Progress {
    int completed = 0;            // số bài đã xong (học tuần tự)
    int stars[F_COUNT] = {};      // số sao mỗi bài (0..3)
    int wins = 0;                 // số trận thắng ở võ đài
    bool introSeen = false;
    int gfxQuality = 2;           // 0 thấp, 1 trung bình, 2 cao

    bool Has(Feature f) const { return completed > (int)f; }
    void Load();
    void Save() const;
    int TotalStars() const;
};

extern Progress g_progress;

// Tiện ích nhỏ
float Clamp01(float v);
float Approach(float cur, float target, float step);   // tiến dần tới target
Color Fade2(Color c, float a);                          // nhân alpha
Color Lerp(Color a, Color b, float t);
