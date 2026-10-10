// =============================================================================
//  UI.h — vẽ chữ tiếng Việt, khung, nút bấm, khối code tô màu cú pháp
// =============================================================================
#pragma once
#include "raylib.h"
#include <string>

// Bảng màu chung (theo phong cách một IDE tối)
namespace pal {
    const Color bg       = {  17,  20,  28, 255 };
    const Color panel    = {  27,  31,  42, 255 };
    const Color panel2   = {  36,  41,  55, 255 };
    const Color line     = {  58,  65,  84, 255 };
    const Color text     = { 222, 226, 236, 255 };
    const Color dim      = { 140, 148, 170, 255 };
    const Color accent   = { 255, 176,  46, 255 };   // vàng hổ phách
    const Color accent2  = {  64, 196, 255, 255 };   // xanh lam
    const Color good     = {  92, 214, 130, 255 };
    const Color bad      = { 255,  92,  92, 255 };
    const Color purple   = { 196, 140, 255, 255 };
}

enum FontStyle { FS_REGULAR = 0, FS_BOLD = 1, FS_MONO = 2 };

namespace ui {
    void  LoadFonts();
    void  UnloadFonts();
    float S();                                   // hệ số co giãn theo chiều cao cửa sổ (720p = 1)

    Vector2 Measure(const std::string& t, float size, int style = FS_REGULAR);
    void  Text(const std::string& t, float x, float y, float size, Color c, int style = FS_REGULAR);
    void  TextCentered(const std::string& t, float cx, float y, float size, Color c, int style = FS_REGULAR);
    void  TextShadow(const std::string& t, float x, float y, float size, Color c, int style = FS_BOLD);

    // Vẽ đoạn văn tự xuống dòng trong box. Hỗ trợ đầu dòng đặc biệt:
    //   "# "  tiêu đề,  "- " gạch đầu dòng,  "> " ô mẹo,  "! " cảnh báo
    // Trả về chiều cao đã dùng. draw=false để chỉ đo.
    float Wrapped(const std::string& t, Rectangle box, float size, Color c, bool draw = true);

    // Vẽ khối code C++ có tô màu cú pháp. Trả về chiều cao.
    float Code(const std::string& code, Rectangle box, float size, bool draw = true);
    void  CodeLine(const std::string& line, float x, float y, float size);   // 1 dòng code tô màu

    bool  Button(Rectangle r, const std::string& label, bool enabled = true, Color accent = pal::accent);
    bool  Hover(Rectangle r);
    void  Panel(Rectangle r, Color bg, Color border, float round = 0.04f);
    void  Bar(Rectangle r, float t, Color fg, Color bg);
    void  Star(Vector2 c, float r, Color col);   // ngôi sao 5 cánh
}
