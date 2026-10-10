// =============================================================================
//  Lessons.h — dữ liệu các bài học (lý thuyết + code mẫu + câu hỏi)
// =============================================================================
#pragma once
#include <string>
#include <vector>

struct Question {
    std::string q;
    std::vector<std::string> options;
    int correct;                 // chỉ số đáp án đúng trong options
    std::string explain;         // giải thích sau khi trả lời
};

struct LessonPage {
    std::string heading;
    std::string text;            // hỗ trợ "# ", "- ", "> ", "! " (xem ui::Wrapped)
    std::string code;            // có thể rỗng
};

struct Lesson {
    std::string level;           // "Cơ bản" / "Trung cấp" / "Nâng cao"
    std::string title;
    std::string file;            // file ví dụ chạy được trong thư mục lessons/
    std::string story;           // lời kể của anh dev
    std::vector<LessonPage> pages;
    std::vector<Question> quiz;
    std::string unlock;          // tên tính năng mở khóa
    std::string unlockHow;       // cách thử tính năng trong võ đài
    std::string exercise;        // bài tập tự làm
    std::string inGame;          // xem khái niệm này trong mã nguồn game ở đâu
};

const std::vector<Lesson>& GetLessons();
