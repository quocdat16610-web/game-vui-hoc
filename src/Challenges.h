// =============================================================================
//  Challenges.h — THỬ THÁCH CODE của từng bài học.
//  Người chơi tự viết hàm; game chấm bằng test tự động; viết đúng thì
//  GAME DÙNG CHÍNH HÀM ĐÓ (xem PlayerCode.h).
// =============================================================================
#pragma once
#include "MiniCpp.h"
#include <functional>
#include <string>
#include <vector>

struct TestResult {
    std::string name;      // ví dụ: TiLeMau(77, 100)
    bool pass = false;
    std::string detail;    // kết quả / lỗi / gợi ý
    int line = 0;          // dòng gây lỗi khi chạy (nếu có)
};

struct Challenge {
    std::string title;     // tên hàm cần viết
    std::string task;      // đề bài
    std::string starter;   // code ban đầu
    std::string solution;  // lời giải mẫu
    std::vector<std::string> hints;
    std::string usedIn;    // game dùng hàm này để làm gì
    bool glsl = false;     // bài 15: code GLSL chạy trên GPU
    std::function<void(mcpp::Program&, std::vector<TestResult>&)> tests;
};

const Challenge& GetChallenge(int lesson);
std::string FileNameForLesson(int lesson);          // code/bai_01.cpp ...

// Biên dịch + chạy toàn bộ test. Trả về true nếu qua hết.
bool RunChallenge(int lesson, const std::string& source, std::vector<mcpp::Error>& compileErrors,
                  std::vector<TestResult>& results, std::string& consoleOut);
