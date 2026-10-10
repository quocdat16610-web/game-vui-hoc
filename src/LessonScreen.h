// =============================================================================
//  LessonScreen.h — màn hình máy tính của anh dev: đọc bài, xem code, làm quiz
// =============================================================================
#pragma once
#include <string>

namespace lessonscreen {
    void Open(int lessonIndex);
    bool Update(float dt);          // true = đóng màn hình, quay lại văn phòng
    void Draw();
    std::string TakeMessage();      // lời anh dev nói khi đóng máy (nếu có)
    bool WantsTextInput();          // đang gõ code (để tắt các phím tắt khác)
}
