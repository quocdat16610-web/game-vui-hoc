// =============================================================================
//  CodeEditor.h — trình soạn thảo code bên trong game.
//  Gõ phím (kể cả tiếng Việt qua Unikey/EVKey), chọn vùng bằng Shift/chuột,
//  Ctrl+C/X/V/A/Z/Y, Tab thụt lề, tự thụt lề khi xuống dòng, Ctrl+/ chú thích.
// =============================================================================
#pragma once
#include "raylib.h"
#include <string>
#include <vector>

class CodeEditor {
public:
    void SetText(const std::string& text);
    std::string Text() const;
    void Update(Rectangle area);           // xử lý bàn phím/chuột
    void Draw(Rectangle area);
    void SetErrorLine(int line) { errLine_ = line; }
    bool Focused() const { return focused_; }
    void Focus(bool f) { focused_ = f; }
    bool TakeChanged() { bool c = changed_; changed_ = false; return c; }

private:
    struct Pos { int y = 0, x = 0; };      // x = số ký tự (codepoint) trong dòng
    struct Snapshot { std::vector<std::string> lines; Pos cur; };

    std::vector<std::string> lines_ = { "" };
    Pos cur_, anchor_;
    bool selecting_ = false;
    bool focused_ = false, dragging_ = false, changed_ = false;
    float scrollY_ = 0, scrollX_ = 0, blink_ = 0;
    int errLine_ = 0;
    std::vector<Snapshot> undo_, redo_;
    double lastTypeTime_ = 0;
    bool lastWasTyping_ = false;
    float fs_ = 16, cw_ = 9, lh_ = 22, gutter_ = 40;

    static int CpCount(const std::string& s);
    static size_t ByteAt(const std::string& s, int cp);
    int LineLen(int y) const { return CpCount(lines_[y]); }

    bool HasSelection() const { return selecting_ && (anchor_.y != cur_.y || anchor_.x != cur_.x); }
    void SelRange(Pos& a, Pos& b) const;
    std::string SelectedText() const;
    void DeleteSelection();
    void Insert(const std::string& text);
    void PushUndo(bool typing = false);
    void Restore(Snapshot& s);
    void Move(Pos p, bool shift);
    void ClampCursor();
    void EnsureVisible(Rectangle area);
    Pos PosFromMouse(Rectangle area) const;
    void Measure(Rectangle area);
    int Indent(int y) const;
    void ToggleComment();
    void IndentLines(bool dedent);
};
