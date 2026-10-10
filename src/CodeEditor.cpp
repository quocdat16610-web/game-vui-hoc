#include "CodeEditor.h"
#include "UI.h"
#include <algorithm>
#include <cctype>
#include <cmath>

// ---- UTF-8: một ký tự tiếng Việt có thể chiếm 2-3 byte ----
int CodeEditor::CpCount(const std::string& s) {
    int n = 0;
    for (unsigned char c : s) if ((c & 0xC0) != 0x80) n++;
    return n;
}

size_t CodeEditor::ByteAt(const std::string& s, int cp) {
    size_t i = 0;
    int n = 0;
    while (i < s.size() && n < cp) {
        i++;
        while (i < s.size() && ((unsigned char)s[i] & 0xC0) == 0x80) i++;
        n++;
    }
    return i;
}

void CodeEditor::SetText(const std::string& text) {
    lines_.clear();
    std::string cur;
    for (char c : text) {
        if (c == '\r') continue;
        if (c == '\n') { lines_.push_back(cur); cur.clear(); }
        else if (c == '\t') cur += "    ";
        else cur += c;
    }
    lines_.push_back(cur);
    while (lines_.size() > 1 && lines_.back().empty() && lines_[lines_.size() - 2].empty()) lines_.pop_back();
    cur_ = anchor_ = {};
    selecting_ = false;
    scrollY_ = scrollX_ = 0;
    undo_.clear();
    redo_.clear();
    errLine_ = 0;
}

std::string CodeEditor::Text() const {
    std::string r;
    for (size_t i = 0; i < lines_.size(); ++i) {
        r += lines_[i];
        if (i + 1 < lines_.size()) r += '\n';
    }
    if (!r.empty() && r.back() != '\n') r += '\n';
    return r;
}

void CodeEditor::SelRange(Pos& a, Pos& b) const {
    a = anchor_;
    b = cur_;
    if (b.y < a.y || (b.y == a.y && b.x < a.x)) std::swap(a, b);
}

std::string CodeEditor::SelectedText() const {
    if (!HasSelection()) return "";
    Pos a, b;
    SelRange(a, b);
    if (a.y == b.y) {
        const std::string& l = lines_[a.y];
        return l.substr(ByteAt(l, a.x), ByteAt(l, b.x) - ByteAt(l, a.x));
    }
    std::string r = lines_[a.y].substr(ByteAt(lines_[a.y], a.x)) + "\n";
    for (int y = a.y + 1; y < b.y; ++y) r += lines_[y] + "\n";
    r += lines_[b.y].substr(0, ByteAt(lines_[b.y], b.x));
    return r;
}

void CodeEditor::DeleteSelection() {
    if (!HasSelection()) { selecting_ = false; return; }
    Pos a, b;
    SelRange(a, b);
    std::string head = lines_[a.y].substr(0, ByteAt(lines_[a.y], a.x));
    std::string tail = lines_[b.y].substr(ByteAt(lines_[b.y], b.x));
    lines_.erase(lines_.begin() + a.y + 1, lines_.begin() + b.y + 1);
    lines_[a.y] = head + tail;
    cur_ = anchor_ = a;
    selecting_ = false;
    changed_ = true;
}

void CodeEditor::Insert(const std::string& text) {
    DeleteSelection();
    std::string& l = lines_[cur_.y];
    size_t at = ByteAt(l, cur_.x);
    std::string tail = l.substr(at);
    l = l.substr(0, at);
    std::string piece;
    for (char c : text) {
        if (c == '\r') continue;
        if (c == '\t') { piece += "    "; continue; }
        if (c == '\n') {
            lines_[cur_.y] += piece;
            cur_.x += CpCount(piece);
            piece.clear();
            lines_.insert(lines_.begin() + cur_.y + 1, "");
            cur_.y++;
            cur_.x = 0;
            continue;
        }
        piece += c;
    }
    lines_[cur_.y] += piece;
    cur_.x = CpCount(lines_[cur_.y]);
    lines_[cur_.y] += tail;
    anchor_ = cur_;
    changed_ = true;
}

void CodeEditor::PushUndo(bool typing) {
    double now = GetTime();
    if (typing && lastWasTyping_ && now - lastTypeTime_ < 1.0) { lastTypeTime_ = now; return; }   // gộp các lần gõ liên tiếp
    undo_.push_back({ lines_, cur_ });
    if (undo_.size() > 300) undo_.erase(undo_.begin());
    redo_.clear();
    lastWasTyping_ = typing;
    lastTypeTime_ = now;
}

void CodeEditor::Restore(Snapshot& s) {
    lines_ = s.lines;
    cur_ = anchor_ = s.cur;
    selecting_ = false;
    ClampCursor();
    changed_ = true;
}

void CodeEditor::ClampCursor() {
    if (lines_.empty()) lines_.push_back("");
    cur_.y = std::max(0, std::min(cur_.y, (int)lines_.size() - 1));
    cur_.x = std::max(0, std::min(cur_.x, LineLen(cur_.y)));
}

void CodeEditor::Move(Pos p, bool shift) {
    if (shift && !selecting_) { anchor_ = cur_; selecting_ = true; }
    if (!shift) selecting_ = false;
    cur_ = p;
    ClampCursor();
    if (!shift) anchor_ = cur_;
    blink_ = 0;
}

int CodeEditor::Indent(int y) const {
    int n = 0;
    while (n < (int)lines_[y].size() && lines_[y][n] == ' ') n++;
    return n;
}

void CodeEditor::ToggleComment() {
    Pos a = cur_, b = cur_;
    if (HasSelection()) SelRange(a, b);
    bool allCommented = true;
    for (int y = a.y; y <= b.y; ++y) {
        const std::string& l = lines_[y];
        size_t f = l.find_first_not_of(' ');
        if (f != std::string::npos && l.compare(f, 2, "//") != 0) allCommented = false;
    }
    for (int y = a.y; y <= b.y; ++y) {
        std::string& l = lines_[y];
        size_t f = l.find_first_not_of(' ');
        if (f == std::string::npos) continue;
        if (allCommented) l.erase(f, (l.compare(f, 3, "// ") == 0) ? 3 : 2);
        else l.insert(f, "// ");
    }
    ClampCursor();
    changed_ = true;
}

void CodeEditor::IndentLines(bool dedent) {
    Pos a, b;
    SelRange(a, b);
    for (int y = a.y; y <= b.y; ++y) {
        if (dedent) lines_[y].erase(0, std::min(4, Indent(y)));
        else lines_[y].insert(0, "    ");
    }
    ClampCursor();
    anchor_.x = std::max(0, std::min(anchor_.x, LineLen(anchor_.y)));
    changed_ = true;
}

void CodeEditor::Measure(Rectangle area) {
    fs_ = 16.5f * ui::S();
    cw_ = ui::Measure("MM", fs_, FS_MONO).x - ui::Measure("M", fs_, FS_MONO).x;
    lh_ = fs_ * 1.42f;
    gutter_ = cw_ * 4.5f;
    (void)area;
}

CodeEditor::Pos CodeEditor::PosFromMouse(Rectangle area) const {
    Vector2 m = GetMousePosition();
    Pos p;
    p.y = (int)std::floor((m.y - area.y - 6 + scrollY_) / lh_);
    p.y = std::max(0, std::min(p.y, (int)lines_.size() - 1));
    p.x = (int)std::floor((m.x - area.x - gutter_ - 8 + scrollX_) / cw_ + 0.5f);
    p.x = std::max(0, std::min(p.x, CpCount(lines_[p.y])));
    return p;
}

void CodeEditor::EnsureVisible(Rectangle area) {
    float cy = cur_.y * lh_;
    float viewH = area.height - 12;
    if (cy < scrollY_) scrollY_ = cy;
    if (cy + lh_ > scrollY_ + viewH) scrollY_ = cy + lh_ - viewH;
    float cx = cur_.x * cw_;
    float viewW = area.width - gutter_ - 20;
    if (cx < scrollX_) scrollX_ = std::max(0.0f, cx - cw_ * 4);
    if (cx > scrollX_ + viewW) scrollX_ = cx - viewW + cw_ * 4;
}

static bool KeyHit(int k) { return IsKeyPressed(k) || IsKeyPressedRepeat(k); }

void CodeEditor::Update(Rectangle area) {
    Measure(area);
    blink_ += GetFrameTime();
    bool hover = CheckCollisionPointRec(GetMousePosition(), area);

    // ---- Chuột ----
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        focused_ = hover;
        if (hover) {
            bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            Move(PosFromMouse(area), shift);
            dragging_ = true;
        }
    }
    if (dragging_) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Pos p = PosFromMouse(area);
            if (p.y != cur_.y || p.x != cur_.x) Move(p, true);
        } else {
            dragging_ = false;
        }
    }
    if (hover) {
        float wheel = GetMouseWheelMove();
        if (IsKeyDown(KEY_LEFT_SHIFT)) scrollX_ = std::max(0.0f, scrollX_ - wheel * cw_ * 6);
        else scrollY_ -= wheel * lh_ * 3;
    }
    float maxScroll = std::max(0.0f, lines_.size() * lh_ - (area.height - 12) + lh_ * 2);
    scrollY_ = std::max(0.0f, std::min(scrollY_, maxScroll));

    if (!focused_) return;

    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) || IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    bool moved = false;

    // ---- Phím tắt Ctrl ----
    if (ctrl) {
        if (IsKeyPressed(KEY_A)) { anchor_ = { 0, 0 }; cur_ = { (int)lines_.size() - 1, LineLen((int)lines_.size() - 1) }; selecting_ = true; }
        if (IsKeyPressed(KEY_C) && HasSelection()) SetClipboardText(SelectedText().c_str());
        if (IsKeyPressed(KEY_X) && HasSelection()) { PushUndo(); SetClipboardText(SelectedText().c_str()); DeleteSelection(); moved = true; }
        if (IsKeyPressed(KEY_V)) {
            const char* clip = GetClipboardText();
            if (clip && *clip) { PushUndo(); Insert(clip); moved = true; }
        }
        if (IsKeyPressed(KEY_Z) && !shift && !undo_.empty()) {
            redo_.push_back({ lines_, cur_ });
            Restore(undo_.back());
            undo_.pop_back();
            lastWasTyping_ = false;
            moved = true;
        }
        if ((IsKeyPressed(KEY_Y) || (IsKeyPressed(KEY_Z) && shift)) && !redo_.empty()) {
            undo_.push_back({ lines_, cur_ });
            Restore(redo_.back());
            redo_.pop_back();
            moved = true;
        }
        if (IsKeyPressed(KEY_SLASH)) { PushUndo(); ToggleComment(); }
        if (KeyHit(KEY_LEFT)) {                             // nhảy theo từ
            Pos p = cur_;
            const std::string& l = lines_[p.y];
            if (p.x == 0 && p.y > 0) { p.y--; p.x = LineLen(p.y); }
            else {
                while (p.x > 0 && l[ByteAt(l, p.x - 1)] == ' ') p.x--;
                while (p.x > 0 && (isalnum((unsigned char)l[ByteAt(l, p.x - 1)]) || l[ByteAt(l, p.x - 1)] == '_')) p.x--;
                if (p.x == cur_.x && p.x > 0) p.x--;
            }
            Move(p, shift); moved = true;
        }
        if (KeyHit(KEY_RIGHT)) {
            Pos p = cur_;
            const std::string& l = lines_[p.y];
            int len = LineLen(p.y);
            if (p.x == len && p.y + 1 < (int)lines_.size()) { p.y++; p.x = 0; }
            else {
                while (p.x < len && (isalnum((unsigned char)l[ByteAt(l, p.x)]) || l[ByteAt(l, p.x)] == '_')) p.x++;
                while (p.x < len && l[ByteAt(l, p.x)] == ' ') p.x++;
                if (p.x == cur_.x && p.x < len) p.x++;
            }
            Move(p, shift); moved = true;
        }
        if (IsKeyPressed(KEY_HOME)) { Move({ 0, 0 }, shift); moved = true; }
        if (IsKeyPressed(KEY_END)) { Move({ (int)lines_.size() - 1, LineLen((int)lines_.size() - 1) }, shift); moved = true; }
    } else {
        // ---- Gõ chữ ----
        int cp;
        while ((cp = GetCharPressed()) != 0) {
            if (cp < 32) continue;
            int n = 0;
            const char* utf8 = CodepointToUTF8(cp, &n);
            std::string s(utf8, n);
            PushUndo(true);
            if (s == "}" && !HasSelection()) {               // tự lùi lề khi gõ '}'
                std::string& l = lines_[cur_.y];
                if (l.find_first_not_of(' ') == std::string::npos && Indent(cur_.y) >= 4 && cur_.x == LineLen(cur_.y)) {
                    l.erase(0, 4);
                    cur_.x -= 4;
                }
            }
            Insert(s);
            moved = true;
        }

        // ---- Phím điều hướng & sửa ----
        if (KeyHit(KEY_LEFT)) {
            if (HasSelection() && !shift) { Pos a, b; SelRange(a, b); Move(a, false); }
            else if (cur_.x > 0) Move({ cur_.y, cur_.x - 1 }, shift);
            else if (cur_.y > 0) Move({ cur_.y - 1, LineLen(cur_.y - 1) }, shift);
            moved = true;
        }
        if (KeyHit(KEY_RIGHT)) {
            if (HasSelection() && !shift) { Pos a, b; SelRange(a, b); Move(b, false); }
            else if (cur_.x < LineLen(cur_.y)) Move({ cur_.y, cur_.x + 1 }, shift);
            else if (cur_.y + 1 < (int)lines_.size()) Move({ cur_.y + 1, 0 }, shift);
            moved = true;
        }
        if (KeyHit(KEY_UP)) { if (cur_.y > 0) Move({ cur_.y - 1, cur_.x }, shift); else Move({ 0, 0 }, shift); moved = true; }
        if (KeyHit(KEY_DOWN)) {
            if (cur_.y + 1 < (int)lines_.size()) Move({ cur_.y + 1, cur_.x }, shift);
            else Move({ cur_.y, LineLen(cur_.y) }, shift);
            moved = true;
        }
        if (KeyHit(KEY_PAGE_UP)) { Move({ cur_.y - (int)(area.height / lh_), cur_.x }, shift); moved = true; }
        if (KeyHit(KEY_PAGE_DOWN)) { Move({ cur_.y + (int)(area.height / lh_), cur_.x }, shift); moved = true; }
        if (IsKeyPressed(KEY_HOME)) {                        // về đầu chữ, bấm lần nữa về cột 0
            int ind = Indent(cur_.y);
            Move({ cur_.y, cur_.x == ind ? 0 : ind }, shift);
            moved = true;
        }
        if (IsKeyPressed(KEY_END)) { Move({ cur_.y, LineLen(cur_.y) }, shift); moved = true; }

        if (KeyHit(KEY_BACKSPACE)) {
            PushUndo();
            if (HasSelection()) DeleteSelection();
            else if (cur_.x > 0) {
                std::string& l = lines_[cur_.y];
                int remove = 1;
                bool onlySpaces = l.substr(0, ByteAt(l, cur_.x)).find_first_not_of(' ') == std::string::npos;
                if (onlySpaces && cur_.x >= 4) remove = ((cur_.x - 1) % 4) + 1;    // xóa cả mức thụt lề
                size_t a = ByteAt(l, cur_.x - remove), b = ByteAt(l, cur_.x);
                l.erase(a, b - a);
                cur_.x -= remove;
            } else if (cur_.y > 0) {
                int len = LineLen(cur_.y - 1);
                lines_[cur_.y - 1] += lines_[cur_.y];
                lines_.erase(lines_.begin() + cur_.y);
                cur_ = { cur_.y - 1, len };
            }
            anchor_ = cur_;
            selecting_ = false;
            changed_ = true;
            moved = true;
        }
        if (KeyHit(KEY_DELETE)) {
            PushUndo();
            if (HasSelection()) DeleteSelection();
            else if (cur_.x < LineLen(cur_.y)) {
                std::string& l = lines_[cur_.y];
                size_t a = ByteAt(l, cur_.x), b = ByteAt(l, cur_.x + 1);
                l.erase(a, b - a);
            } else if (cur_.y + 1 < (int)lines_.size()) {
                lines_[cur_.y] += lines_[cur_.y + 1];
                lines_.erase(lines_.begin() + cur_.y + 1);
            }
            changed_ = true;
            moved = true;
        }
        if (KeyHit(KEY_ENTER) || KeyHit(KEY_KP_ENTER)) {
            PushUndo();
            DeleteSelection();
            const std::string& l = lines_[cur_.y];
            int ind = Indent(cur_.y);
            std::string before = l.substr(0, ByteAt(l, cur_.x));
            std::string after = l.substr(ByteAt(l, cur_.x));
            size_t lastNon = before.find_last_not_of(' ');
            bool openBrace = lastNon != std::string::npos && before[lastNon] == '{';
            size_t firstNon = after.find_first_not_of(' ');
            bool closeNext = firstNon != std::string::npos && after[firstNon] == '}';
            std::string pad(ind + (openBrace ? 4 : 0), ' ');
            if (openBrace && closeNext) Insert("\n" + pad + "\n" + std::string(ind, ' ')), Move({ cur_.y - 1, (int)pad.size() }, false);
            else Insert("\n" + pad);
            moved = true;
        }
        if (KeyHit(KEY_TAB)) {
            PushUndo();
            if (HasSelection() && anchor_.y != cur_.y) IndentLines(shift);
            else if (shift) { int d = std::min(4, Indent(cur_.y)); lines_[cur_.y].erase(0, d); cur_.x = std::max(0, cur_.x - d); anchor_ = cur_; changed_ = true; }
            else Insert(std::string(4 - (cur_.x % 4), ' '));
            moved = true;
        }
    }
    if (moved) EnsureVisible(area);
}

void CodeEditor::Draw(Rectangle area) {
    Measure(area);
    DrawRectangleRec(area, Color{ 13, 15, 22, 255 });
    DrawRectangleLinesEx(area, 1.5f, focused_ ? pal::accent2 : pal::line);
    DrawRectangle((int)area.x, (int)area.y, (int)gutter_, (int)area.height, Color{ 18, 21, 30, 255 });

    BeginScissorMode((int)area.x + 1, (int)area.y + 1, (int)area.width - 2, (int)area.height - 2);
    float x0 = area.x + gutter_ + 8 - scrollX_;
    float y0 = area.y + 6 - scrollY_;
    int first = std::max(0, (int)(scrollY_ / lh_) - 1);
    int last = std::min((int)lines_.size() - 1, (int)((scrollY_ + area.height) / lh_) + 1);
    Pos sa, sb;
    bool sel = HasSelection();
    if (sel) SelRange(sa, sb);

    for (int y = first; y <= last; ++y) {
        float ly = y0 + y * lh_;
        if (y + 1 == errLine_) {
            DrawRectangle((int)area.x, (int)ly, (int)area.width, (int)lh_, Color{ 120, 30, 36, 120 });
        } else if (focused_ && y == cur_.y && !sel) {
            DrawRectangle((int)(area.x + gutter_), (int)ly, (int)(area.width - gutter_), (int)lh_, Color{ 30, 34, 48, 255 });
        }
        if (sel && y >= sa.y && y <= sb.y) {
            int a = (y == sa.y) ? sa.x : 0;
            int b = (y == sb.y) ? sb.x : LineLen(y) + 1;
            DrawRectangle((int)(x0 + a * cw_), (int)ly, (int)((b - a) * cw_), (int)lh_, Color{ 60, 90, 150, 150 });
        }
        ui::CodeLine(lines_[y], x0, ly + (lh_ - fs_) * 0.35f, fs_);
    }
    // Con trỏ nhấp nháy
    if (focused_ && std::fmod(blink_, 1.0f) < 0.6f) {
        float cx = x0 + cur_.x * cw_, cy = y0 + cur_.y * lh_;
        DrawRectangle((int)cx, (int)cy + 2, 2, (int)lh_ - 4, pal::accent);
    }
    // Số dòng
    DrawRectangle((int)area.x + 1, (int)area.y + 1, (int)gutter_ - 1, (int)area.height - 2, Color{ 18, 21, 30, 255 });
    for (int y = first; y <= last; ++y) {
        float ly = y0 + y * lh_;
        Color c = (y + 1 == errLine_) ? pal::bad : (y == cur_.y ? pal::text : Color{ 80, 88, 110, 255 });
        std::string n = std::to_string(y + 1);
        ui::Text(n, area.x + gutter_ - 8 - ui::Measure(n, fs_ * 0.9f, FS_MONO).x, ly + (lh_ - fs_) * 0.4f, fs_ * 0.9f, c, FS_MONO);
    }
    EndScissorMode();

    if (!focused_) {
        const char* tip = "Bấm vào đây để gõ code";
        Vector2 m = ui::Measure(tip, 14 * ui::S());
        ui::Text(tip, area.x + area.width - m.x - 10, area.y + area.height - m.y - 8, 14 * ui::S(), pal::dim);
    }
}
