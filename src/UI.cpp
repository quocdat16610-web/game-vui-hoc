#include "UI.h"
#include <cctype>
#include <cmath>
#include <vector>
#include <unordered_set>

static Font s_fonts[3];
static bool s_loaded[3] = {};

// Font mặc định của raylib chỉ có chữ ASCII -> phải tự nạp font TTF với đủ
// các "codepoint" (mã ký tự Unicode) của tiếng Việt.
static std::vector<int> VietnameseCodepoints() {
    std::vector<int> cps;
    for (int c = 32;     c <= 126;    ++c) cps.push_back(c);   // ASCII
    for (int c = 0xA0;   c <= 0x17F;  ++c) cps.push_back(c);   // Latin-1 + Latin Extended-A (à á â ã đ ...)
    for (int c = 0x1A0;  c <= 0x1B0;  ++c) cps.push_back(c);   // Ơ ơ Ư ư
    for (int c = 0x300;  c <= 0x323;  ++c) cps.push_back(c);   // dấu tổ hợp
    for (int c = 0x1EA0; c <= 0x1EF9; ++c) cps.push_back(c);   // ạ ả ấ ầ ... ỹ
    const int extra[] = { 0x2022, 0x2026, 0x2190, 0x2191, 0x2192, 0x2193, 0x2605, 0x2606,
                          0x2713, 0x2717, 0x25B6, 0x25C0, 0x25CF, 0x25CB, 0x2014, 0x2013, 0x201C, 0x201D, 0x2264, 0x2265, 0x2260, 0x00D7 };
    for (int c : extra) cps.push_back(c);
    return cps;
}

void ui::LoadFonts() {
    const char* files[3] = { "assets/fonts/DejaVuSans.ttf", "assets/fonts/DejaVuSans-Bold.ttf",
                             "assets/fonts/JetBrainsMono-Regular.ttf" };
    std::vector<int> cps = VietnameseCodepoints();
    for (int i = 0; i < 3; ++i) {
        if (FileExists(files[i])) {
            s_fonts[i] = LoadFontEx(files[i], 48, cps.data(), (int)cps.size());
            GenTextureMipmaps(&s_fonts[i].texture);
            SetTextureFilter(s_fonts[i].texture, TEXTURE_FILTER_TRILINEAR);
            s_loaded[i] = true;
        } else {
            TraceLog(LOG_WARNING, "Khong tim thay font %s, dung font mac dinh", files[i]);
            s_fonts[i] = GetFontDefault();
        }
    }
}

void ui::UnloadFonts() {
    for (int i = 0; i < 3; ++i) if (s_loaded[i]) UnloadFont(s_fonts[i]);
}

float ui::S() { return GetScreenHeight() / 720.0f; }

Vector2 ui::Measure(const std::string& t, float size, int style) {
    return MeasureTextEx(s_fonts[style], t.c_str(), size, size * 0.02f);
}

void ui::Text(const std::string& t, float x, float y, float size, Color c, int style) {
    DrawTextEx(s_fonts[style], t.c_str(), { x, y }, size, size * 0.02f, c);
}

void ui::TextCentered(const std::string& t, float cx, float y, float size, Color c, int style) {
    Vector2 m = Measure(t, size, style);
    Text(t, cx - m.x / 2, y, size, c, style);
}

void ui::TextShadow(const std::string& t, float x, float y, float size, Color c, int style) {
    float o = size * 0.06f;
    Text(t, x + o, y + o, size, Color{ 0, 0, 0, (unsigned char)(c.a * 0.6f) }, style);
    Text(t, x, y, size, c, style);
}

// ---------------------------------------------------------------------------
// Tự xuống dòng
// ---------------------------------------------------------------------------
static std::vector<std::string> SplitLines(const std::string& t) {
    std::vector<std::string> out;
    std::string cur;
    for (char ch : t) {
        if (ch == '\n') { out.push_back(cur); cur.clear(); }
        else cur += ch;
    }
    out.push_back(cur);
    return out;
}

// Ngắt 1 đoạn thành nhiều dòng vừa với maxW
static std::vector<std::string> WrapParagraph(const std::string& p, float size, int style, float maxW) {
    std::vector<std::string> lines;
    std::string line, word;
    auto flushWord = [&]() {
        if (word.empty()) return;
        std::string test = line.empty() ? word : line + " " + word;
        if (ui::Measure(test, size, style).x > maxW && !line.empty()) {
            lines.push_back(line);
            line = word;
        } else {
            line = test;
        }
        word.clear();
    };
    for (char ch : p) {
        if (ch == ' ') flushWord();
        else word += ch;
    }
    flushWord();
    lines.push_back(line);
    return lines;
}

float ui::Wrapped(const std::string& t, Rectangle box, float size, Color c, bool draw) {
    float y = box.y;
    float lh = size * 1.45f;
    for (const std::string& raw : SplitLines(t)) {
        if (raw.empty()) { y += lh * 0.55f; continue; }

        std::string body = raw;
        int style = FS_REGULAR;
        Color col = c;
        float indent = 0, sz = size;
        int kind = 0;   // 0 thường, 1 tiêu đề, 2 bullet, 3 mẹo, 4 cảnh báo
        if (raw.rfind("# ", 0) == 0)      { kind = 1; body = raw.substr(2); style = FS_BOLD; col = pal::accent; sz = size * 1.12f; }
        else if (raw.rfind("- ", 0) == 0) { kind = 2; body = raw.substr(2); indent = size * 1.3f; }
        else if (raw.rfind("> ", 0) == 0) { kind = 3; body = raw.substr(2); indent = size * 0.9f; col = Color{ 255, 226, 150, 255 }; }
        else if (raw.rfind("! ", 0) == 0) { kind = 4; body = raw.substr(2); indent = size * 0.9f; col = Color{ 255, 170, 170, 255 }; }

        float lineH = sz * 1.45f;
        std::vector<std::string> lines = WrapParagraph(body, sz, style, box.width - indent - (kind >= 3 ? size * 0.6f : 0));
        float blockH = lines.size() * lineH;

        if (draw) {
            if (kind == 2) DrawCircle((int)(box.x + size * 0.5f), (int)(y + sz * 0.62f), size * 0.16f, pal::accent);
            if (kind >= 3) {
                Color bc = kind == 3 ? pal::accent : pal::bad;
                DrawRectangleRec({ box.x, y - sz * 0.15f, box.width, blockH + sz * 0.2f }, Fade(bc, 0.10f));
                DrawRectangleRec({ box.x, y - sz * 0.15f, size * 0.18f, blockH + sz * 0.2f }, bc);
            }
            float ly = y;
            for (const std::string& l : lines) {
                Text(l, box.x + indent, ly, sz, col, style);
                ly += lineH;
            }
        }
        y += blockH + (kind == 1 ? size * 0.25f : 0) + (kind >= 3 ? size * 0.35f : 0);
    }
    return y - box.y;
}

// ---------------------------------------------------------------------------
// Tô màu cú pháp C++ (rất đơn giản: đủ để đọc code dễ hơn)
// ---------------------------------------------------------------------------
static const std::unordered_set<std::string>& Keywords() {
    static const std::unordered_set<std::string> kw = {
        "int", "float", "double", "bool", "char", "void", "auto", "const", "constexpr", "if", "else", "for",
        "while", "do", "return", "class", "struct", "public", "private", "protected", "virtual", "override",
        "new", "delete", "nullptr", "true", "false", "namespace", "using", "switch", "case", "break",
        "continue", "enum", "static", "template", "typename", "this", "unsigned", "default", "operator",
        "sizeof", "inline", "final", "explicit", "std",
        "uniform", "in", "out", "vec2", "vec3", "vec4", "mat4", "sampler2D" };
    return kw;
}

namespace {
    const Color C_TEXT    = { 214, 218, 228, 255 };
    const Color C_KW      = { 198, 120, 221, 255 };
    const Color C_TYPE    = {  97, 205, 255, 255 };
    const Color C_STR     = { 245, 172,  98, 255 };
    const Color C_NUM     = { 209, 154, 102, 255 };
    const Color C_COMMENT = { 112, 160, 110, 255 };
    const Color C_PRE     = { 255, 121, 168, 255 };
    const Color C_FUNC    = { 240, 214, 120, 255 };
}

void ui::CodeLine(const std::string& l, float x, float y, float size) {
    size_t i = 0;
    auto put = [&](const std::string& s, Color c) {
        Text(s, x, y, size, c, FS_MONO);
        x += Measure(s, size, FS_MONO).x + size * 0.02f;
    };
    // Dòng tiền xử lý (#include ...)
    size_t first = l.find_first_not_of(' ');
    if (first != std::string::npos && l[first] == '#') {
        size_t cpos = l.find("//");
        put(l.substr(0, cpos), C_PRE);
        if (cpos != std::string::npos) put(l.substr(cpos), C_COMMENT);
        return;
    }
    while (i < l.size()) {
        char ch = l[i];
        if (ch == '/' && i + 1 < l.size() && (l[i + 1] == '/' || l[i + 1] == '*')) { put(l.substr(i), C_COMMENT); break; }
        if (ch == '"' || ch == '\'') {
            size_t j = i + 1;
            while (j < l.size() && l[j] != ch) { if (l[j] == '\\') j++; j++; }
            put(l.substr(i, j - i + 1), C_STR);
            i = j + 1;
            continue;
        }
        if (isdigit((unsigned char)ch)) {
            size_t j = i;
            while (j < l.size() && (isalnum((unsigned char)l[j]) || l[j] == '.')) j++;
            put(l.substr(i, j - i), C_NUM);
            i = j;
            continue;
        }
        if (isalpha((unsigned char)ch) || ch == '_') {
            size_t j = i;
            while (j < l.size() && (isalnum((unsigned char)l[j]) || l[j] == '_')) j++;
            std::string w = l.substr(i, j - i);
            Color c = C_TEXT;
            if (Keywords().count(w)) c = C_KW;
            else if (isupper((unsigned char)w[0])) c = C_TYPE;
            else if (j < l.size() && l[j] == '(') c = C_FUNC;
            else if (w == "vector" || w == "string" || w == "map" || w == "unique_ptr" ||
                     w == "cout" || w == "endl" || w == "make_unique" || w == "size_t") c = C_TYPE;
            put(w, c);
            i = j;
            continue;
        }
        size_t j = i + 1;
        while (j < l.size() && !isalnum((unsigned char)l[j]) && l[j] != '_' && l[j] != '"' &&
               l[j] != '\'' && !(l[j] == '/' && j + 1 < l.size() && (l[j + 1] == '/' || l[j + 1] == '*'))) j++;
        put(l.substr(i, j - i), C_TEXT);
        i = j;
    }
}

float ui::Code(const std::string& code, Rectangle box, float size, bool draw) {
    std::vector<std::string> lines = SplitLines(code);
    // Thu nhỏ chữ nếu có dòng quá dài so với khung
    float pad = size * 0.8f;
    float charW = Measure("M", size, FS_MONO).x;
    size_t maxCols = 1;
    for (auto& l : lines) {
        size_t cols = 0;
        for (unsigned char ch : l) if ((ch & 0xC0) != 0x80) cols++;   // đếm ký tự UTF-8
        if (cols > maxCols) maxCols = cols;
    }
    float gutter = size * 2.4f;
    float avail = box.width - pad * 2 - gutter;
    if (maxCols * charW > avail) size *= avail / (maxCols * charW);
    float lh = size * 1.4f;
    float h = lines.size() * lh + pad * 2;
    if (!draw) return h;

    DrawRectangleRec({ box.x, box.y, box.width, h }, Color{ 13, 15, 22, 255 });
    DrawRectangleLinesEx({ box.x, box.y, box.width, h }, 1, pal::line);

    float y = box.y + pad;
    int lineNo = 1;
    for (const std::string& l : lines) {
        Text(TextFormat("%2d", lineNo++), box.x + pad * 0.6f, y, size, Color{ 80, 88, 110, 255 }, FS_MONO);
        CodeLine(l, box.x + pad + gutter, y, size);
        y += lh;
    }
    return h;
}

bool ui::Hover(Rectangle r) { return CheckCollisionPointRec(GetMousePosition(), r); }

bool ui::Button(Rectangle r, const std::string& label, bool enabled, Color accent) {
    bool hover = enabled && Hover(r);
    Color bg = !enabled ? Color{ 40, 44, 56, 255 } : (hover ? accent : pal::panel2);
    Color fg = !enabled ? Color{ 90, 96, 112, 255 } : (hover ? pal::bg : pal::text);
    DrawRectangleRounded(r, 0.25f, 6, bg);
    DrawRectangleRoundedLinesEx(r, 0.25f, 6, 1.5f, enabled ? accent : pal::line);
    float size = r.height * 0.42f;
    Vector2 m = Measure(label, size, FS_BOLD);
    Text(label, r.x + (r.width - m.x) / 2, r.y + (r.height - m.y) / 2, size, fg, FS_BOLD);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void ui::Panel(Rectangle r, Color bg, Color border, float round) {
    DrawRectangleRounded(r, round, 6, bg);
    DrawRectangleRoundedLinesEx(r, round, 6, 1.5f, border);
}

void ui::Bar(Rectangle r, float t, Color fg, Color bg) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    DrawRectangleRec(r, bg);
    DrawRectangleRec({ r.x, r.y, r.width * t, r.height }, fg);
}

void ui::Star(Vector2 c, float r, Color col) {
    Vector2 pts[10];
    for (int i = 0; i < 10; ++i) {
        float ang = -PI / 2 + i * PI / 5;
        float rr = (i % 2 == 0) ? r : r * 0.45f;
        pts[i] = { c.x + std::cos(ang) * rr, c.y + std::sin(ang) * rr };
    }
    for (int i = 0; i < 10; ++i) DrawTriangle(c, pts[(i + 1) % 10], pts[i], col);   // thứ tự ngược chiều kim đồng hồ
}
