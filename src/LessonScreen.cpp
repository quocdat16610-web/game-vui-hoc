#include "LessonScreen.h"
#include "Audio.h"
#include "Common.h"
#include "Lessons.h"
#include "UI.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace {
    struct QuizState {
        std::vector<int> order;   // thứ tự hiển thị đáp án (được xáo trộn)
        int picked = -1;          // chỉ số đáp án gốc đã chọn
        bool solved = false;
        bool missed = false;      // đã từng trả lời sai
    };

    struct ScreenState {
        int lesson = 0;
        int step = 0;
        float scroll = 0, maxScroll = 0;
        std::vector<QuizState> quiz;
        bool finished = false;
        int earnedStars = 0;
        bool newlyUnlocked = false;
        std::string message;
        bool close = false;
        float time = 0;
    };
    ScreenState S;

    const Lesson& Cur() { return GetLessons()[S.lesson]; }
    int PageCount() { return (int)Cur().pages.size(); }
    int QuizCount() { return (int)Cur().quiz.size(); }
    int StepCount() { return 1 + PageCount() + QuizCount() + 1; }
    bool IsQuizStep(int st) { return st > PageCount() && st <= PageCount() + QuizCount(); }
    int QuizIndex(int st) { return st - PageCount() - 1; }
    bool IsResultStep(int st) { return st == StepCount() - 1; }
}

static void Finish() {
    int missed = 0;
    for (auto& q : S.quiz) missed += q.missed ? 1 : 0;
    S.earnedStars = missed == 0 ? 3 : (missed == 1 ? 2 : 1);
    g_progress.stars[S.lesson] = std::max(g_progress.stars[S.lesson], S.earnedStars);
    S.newlyUnlocked = false;
    if (S.lesson == g_progress.completed) {
        g_progress.completed++;
        S.newlyUnlocked = true;
        sfx::Play(sfx::UNLOCK, 0);
    }
    g_progress.Save();
    S.finished = true;
}

void lessonscreen::Open(int idx) {
    S = ScreenState{};
    S.lesson = std::clamp(idx, 0, (int)GetLessons().size() - 1);
    for (const Question& q : Cur().quiz) {
        QuizState qs;
        for (int i = 0; i < (int)q.options.size(); ++i) qs.order.push_back(i);
        for (int i = (int)qs.order.size() - 1; i > 0; --i) std::swap(qs.order[i], qs.order[GetRandomValue(0, i)]);
        S.quiz.push_back(qs);
    }
}

std::string lessonscreen::TakeMessage() {
    std::string m = S.message;
    S.message.clear();
    return m;
}

static bool CanGoNext() {
    if (IsResultStep(S.step)) return false;
    if (IsQuizStep(S.step)) return S.quiz[QuizIndex(S.step)].solved;
    return true;
}

static void GoTo(int st) {
    st = std::clamp(st, 0, StepCount() - 1);
    if (st == S.step) return;
    S.step = st;
    S.scroll = 0;
    if (IsResultStep(S.step) && !S.finished) Finish();
}

static void Pick(int orderPos) {
    QuizState& qs = S.quiz[QuizIndex(S.step)];
    if (qs.solved || qs.picked >= 0) return;
    if (orderPos < 0 || orderPos >= (int)qs.order.size()) return;
    qs.picked = qs.order[orderPos];
    if (qs.picked == Cur().quiz[QuizIndex(S.step)].correct) { qs.solved = true; sfx::Play(sfx::CORRECT, 0); }
    else { qs.missed = true; sfx::Play(sfx::WRONG, 0); }
}

static void CloseScreen() {
    if (S.finished && S.newlyUnlocked) {
        if (g_progress.completed >= F_COUNT)
            S.message = "XONG! \"Code Võ Đài\" đã hoàn chỉnh: hiệp đấu, AI, camera, hiệu ứng... Ra võ đài đánh một trận ăn mừng nào! "
                        "Sau đó thử đọc toàn bộ mã nguồn trong thư mục src/ nhé.";
        else
            S.message = "Tuyệt! Mình vừa code xong \"" + Cur().unlock + "\". " + Cur().unlockHow;
    }
    S.close = true;
}

bool lessonscreen::Update(float dt) {
    S.time += dt;
    if (S.close) return true;
    if (IsKeyPressed(KEY_ESCAPE)) { CloseScreen(); return true; }

    S.scroll -= GetMouseWheelMove() * 50.0f * ui::S();
    if (IsKeyDown(KEY_DOWN)) S.scroll += 500 * dt;
    if (IsKeyDown(KEY_UP)) S.scroll -= 500 * dt;
    S.scroll = std::clamp(S.scroll, 0.0f, std::max(0.0f, S.maxScroll));

    if (IsQuizStep(S.step)) {
        for (int k = 0; k < 4; ++k)
            if (IsKeyPressed(KEY_ONE + k) || IsKeyPressed(KEY_KP_1 + k)) Pick(k);
        QuizState& qs = S.quiz[QuizIndex(S.step)];
        if (!qs.solved && qs.picked >= 0 && (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER))) qs.picked = -1;
    }
    if ((IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_ENTER)) && CanGoNext()) GoTo(S.step + 1);
    else if (IsKeyPressed(KEY_ENTER) && IsResultStep(S.step)) { CloseScreen(); return true; }
    if (IsKeyPressed(KEY_LEFT)) GoTo(S.step - 1);
    return S.close;
}

// =============================================================================
//  VẼ
// =============================================================================
static float DrawStory(Rectangle c, float s) {
    const Lesson& l = Cur();
    float y = c.y;
    ui::Text(TextFormat("BÀI %d  ·  %s", S.lesson + 1, l.level.c_str()), c.x, y, 15 * s, pal::accent2, FS_BOLD);
    y += 26 * s;
    ui::Text(l.title, c.x, y, 32 * s, pal::text, FS_BOLD);
    y += 56 * s;

    // Lời kể của anh dev
    float th = ui::Wrapped(l.story, { c.x + 70 * s, 0, c.width - 90 * s, 0 }, 19 * s, pal::text, false);
    Rectangle box = { c.x, y, c.width, th + 52 * s };
    ui::Panel(box, pal::panel2, pal::line, 0.06f);
    DrawCircle((int)(c.x + 36 * s), (int)(y + 36 * s), 22 * s, Color{ 58, 104, 196, 255 });
    DrawCircle((int)(c.x + 36 * s), (int)(y + 30 * s), 11 * s, Color{ 240, 200, 160, 255 });
    DrawRectangle((int)(c.x + 26 * s), (int)(y + 26 * s), (int)(20 * s), (int)(4 * s), Color{ 20, 20, 24, 255 });
    ui::Text("Minh (dev)", c.x + 70 * s, y + 12 * s, 15 * s, pal::accent2, FS_BOLD);
    ui::Wrapped(l.story, { c.x + 70 * s, y + 36 * s, c.width - 90 * s, 0 }, 19 * s, pal::text);
    y += box.height + 28 * s;

    ui::Text("TRONG BÀI NÀY", c.x, y, 15 * s, pal::dim, FS_BOLD);
    y += 28 * s;
    for (int i = 0; i < PageCount(); ++i) {
        ui::Text(TextFormat("%d.  %s", i + 1, l.pages[i].heading.c_str()), c.x + 10 * s, y, 18 * s, pal::text);
        y += 30 * s;
    }
    ui::Text(TextFormat("%d.  Thử thách: %d câu hỏi", PageCount() + 1, QuizCount()), c.x + 10 * s, y, 18 * s, pal::text);
    y += 44 * s;

    Rectangle un = { c.x, y, c.width, 62 * s };
    ui::Panel(un, Fade(pal::accent, 0.12f), pal::accent, 0.12f);
    ui::Text("MỞ KHÓA CHO GAME ĐỐI KHÁNG", un.x + 16 * s, un.y + 10 * s, 13 * s, pal::accent, FS_BOLD);
    ui::Text(l.unlock, un.x + 16 * s, un.y + 30 * s, 20 * s, pal::text, FS_BOLD);
    y += un.height + 16 * s;
    ui::Text(TextFormat("File ví dụ chạy được: %s", l.file.c_str()), c.x, y, 15 * s, pal::dim, FS_MONO);
    y += 30 * s;
    return y - c.y;
}

static float DrawPage(Rectangle c, float s, int pageIdx) {
    const LessonPage& p = Cur().pages[pageIdx];
    float y = c.y;
    ui::Text(TextFormat("BÀI %d  ·  PHẦN %d/%d", S.lesson + 1, pageIdx + 1, PageCount()), c.x, y, 15 * s, pal::accent2, FS_BOLD);
    y += 26 * s;
    ui::Text(p.heading, c.x, y, 30 * s, pal::text, FS_BOLD);
    y += 52 * s;
    y += ui::Wrapped(p.text, { c.x, y, c.width, 0 }, 19 * s, pal::text) + 14 * s;
    if (!p.code.empty()) {
        y += ui::Code(p.code, { c.x, y, c.width, 0 }, 17 * s) + 16 * s;
    }
    if (pageIdx == PageCount() - 1) {
        y += 6 * s;
        y += ui::Wrapped(std::string("> Xem trong mã nguồn game: ") + Cur().inGame, { c.x, y, c.width, 0 }, 16 * s, pal::text) + 10 * s;
    }
    return y - c.y;
}

static float DrawQuiz(Rectangle c, float s, int qi) {
    const Question& q = Cur().quiz[qi];
    QuizState& qs = S.quiz[qi];
    float y = c.y;
    ui::Text(TextFormat("THỬ THÁCH  ·  CÂU %d/%d", qi + 1, QuizCount()), c.x, y, 15 * s, pal::accent, FS_BOLD);
    y += 30 * s;
    y += ui::Wrapped("# " + q.q, { c.x, y, c.width, 0 }, 22 * s, pal::text) + 18 * s;

    for (int i = 0; i < (int)qs.order.size(); ++i) {
        int opt = qs.order[i];
        const std::string& label = q.options[opt];
        float th = ui::Wrapped(label, { 0, 0, c.width - 80 * s, 0 }, 18 * s, pal::text, false);
        Rectangle r = { c.x, y, c.width, std::max(52 * s, th + 26 * s) };
        bool chosen = qs.picked == opt;
        bool correct = opt == q.correct;
        Color border = pal::line, bg = pal::panel2;
        bool marked = true;
        if (qs.picked >= 0 && chosen) { border = correct ? pal::good : pal::bad; bg = Fade(border, 0.18f); }
        else if (qs.solved && correct) { border = pal::good; }
        else if (qs.picked < 0 && ui::Hover(r)) { border = pal::accent; bg = Fade(pal::accent, 0.08f); }
        else marked = false;
        ui::Panel(r, bg, border, 0.12f);
        Rectangle key = { r.x + 12 * s, r.y + (r.height - 28 * s) / 2, 28 * s, 28 * s };
        DrawRectangleRounded(key, 0.3f, 4, marked ? border : pal::panel);
        ui::TextCentered(TextFormat("%d", i + 1), key.x + key.width / 2, key.y + 4 * s, 18 * s,
                         marked ? pal::bg : pal::text, FS_BOLD);
        ui::Wrapped(label, { r.x + 56 * s, r.y + (r.height - th) / 2 + 3 * s, c.width - 80 * s, 0 }, 18 * s, pal::text);
        if (qs.picked < 0 && ui::Hover(r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) Pick(i);
        y += r.height + 10 * s;
    }

    if (qs.picked >= 0) {
        y += 8 * s;
        bool ok = qs.solved;
        std::string head = ok ? "Chính xác!" : "Chưa đúng rồi.";
        ui::Text(head, c.x, y, 22 * s, ok ? pal::good : pal::bad, FS_BOLD);
        y += 34 * s;
        // Chỉ giải thích khi đã đúng, để người học tự suy nghĩ lại khi sai
        const std::string& note = ok ? q.explain : std::string("Gợi ý: quay lại đọc phần lý thuyết (nút ← Trước) rồi thử lại nhé.");
        y += ui::Wrapped(note, { c.x, y, c.width, 0 }, 18 * s, pal::text) + 10 * s;
        if (!ok) {
            if (ui::Button({ c.x, y, 200 * s, 42 * s }, "Thử lại (R)", true, pal::accent2)) qs.picked = -1;
            y += 52 * s;
        }
    }
    return y - c.y;
}

static float DrawResult(Rectangle c, float s) {
    const Lesson& l = Cur();
    float y = c.y + 10 * s;
    ui::TextCentered("HOÀN THÀNH BÀI HỌC!", c.x + c.width / 2, y, 34 * s, pal::accent, FS_BOLD);
    y += 56 * s;
    // Sao
    for (int i = 0; i < 3; ++i) {
        bool on = i < S.earnedStars;
        float cx = c.x + c.width / 2 + (i - 1) * 64 * s;
        float pulse = on ? 1.0f + 0.08f * std::sin(S.time * 4 + i) : 1.0f;
        ui::Star({ cx, y + 26 * s }, 28 * s * pulse, on ? pal::accent : pal::panel2);
    }
    y += 66 * s;
    ui::TextCentered(S.earnedStars == 3 ? "Trả lời đúng hết ngay lần đầu!" : "Làm lại quiz bất cứ lúc nào để lấy đủ 3 sao.",
                     c.x + c.width / 2, y, 16 * s, pal::dim);
    y += 40 * s;

    Rectangle un = { c.x, y, c.width, 90 * s };
    ui::Panel(un, Fade(pal::good, 0.12f), pal::good, 0.1f);
    ui::Text(S.newlyUnlocked ? "ĐÃ MỞ KHÓA TÍNH NĂNG MỚI" : "TÍNH NĂNG CỦA BÀI NÀY", un.x + 16 * s, un.y + 12 * s, 14 * s, pal::good, FS_BOLD);
    ui::Text(l.unlock, un.x + 16 * s, un.y + 32 * s, 22 * s, pal::text, FS_BOLD);
    ui::Text(l.unlockHow, un.x + 16 * s, un.y + 62 * s, 15 * s, pal::dim);
    y += un.height + 22 * s;

    ui::Text("BÀI TẬP TỰ LÀM", c.x, y, 15 * s, pal::accent, FS_BOLD);
    y += 26 * s;
    y += ui::Wrapped(l.exercise, { c.x, y, c.width, 0 }, 18 * s, pal::text) + 8 * s;
    y += ui::Wrapped(std::string("> Mở file ") + l.file + " trong IDE, biên dịch và chạy thử. Hướng dẫn biên dịch có trong README.md.",
                     { c.x, y, c.width, 0 }, 16 * s, pal::text) + 10 * s;
    return y - c.y;
}

void lessonscreen::Draw() {
    float s = ui::S();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    DrawRectangle(0, 0, (int)W, (int)H, Color{ 0, 0, 0, 170 });

    float m = 20 * s;
    Rectangle win = { m, m, W - 2 * m, H - 2 * m };
    ui::Panel(win, pal::bg, pal::line, 0.015f);

    // Thanh tiêu đề
    DrawRectangleRec({ win.x + 2, win.y + 2, win.width - 4, 36 * s }, pal::panel);
    DrawCircle((int)(win.x + 20 * s), (int)(win.y + 20 * s), 6 * s, pal::bad);
    DrawCircle((int)(win.x + 40 * s), (int)(win.y + 20 * s), 6 * s, pal::accent);
    DrawCircle((int)(win.x + 60 * s), (int)(win.y + 20 * s), 6 * s, pal::good);
    ui::TextCentered(TextFormat("DevStudio++  —  %s", Cur().file.c_str()), win.x + win.width / 2, win.y + 9 * s, 16 * s, pal::dim, FS_MONO);

    // Thanh bên: danh sách bài học
    float sbW = std::min(280 * s, win.width * 0.3f);
    Rectangle sb = { win.x + 2, win.y + 38 * s, sbW, win.height - 40 * s };
    DrawRectangleRec(sb, pal::panel);
    ui::Text("BÀI HỌC", sb.x + 16 * s, sb.y + 12 * s, 14 * s, pal::dim, FS_BOLD);
    const auto& L = GetLessons();
    float rowH = std::min(40 * s, (sb.height - 50 * s) / L.size());
    std::string lastLevel;
    for (int i = 0; i < (int)L.size(); ++i) {
        Rectangle r = { sb.x + 6 * s, sb.y + 40 * s + i * rowH, sb.width - 12 * s, rowH - 2 * s };
        bool unlocked = i <= g_progress.completed;
        bool done = i < g_progress.completed;
        bool active = i == S.lesson;
        if (active) DrawRectangleRounded(r, 0.2f, 4, pal::panel2);
        else if (unlocked && ui::Hover(r)) DrawRectangleRounded(r, 0.2f, 4, Fade(pal::panel2, 0.6f));
        if (unlocked && !active && ui::Hover(r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { Open(i); return; }
        Color c = active ? pal::accent : (done ? pal::good : (unlocked ? pal::text : Color{ 80, 86, 104, 255 }));
        const char* icon = done ? "✓" : (unlocked ? "▶" : "·");
        float ts = std::min(15 * s, rowH * 0.42f);
        ui::Text(icon, r.x + 8 * s, r.y + (r.height - ts) / 2 - 1, ts, c, FS_BOLD);
        std::string name = TextFormat("%02d %s", i + 1, L[i].title.c_str());
        bool cut = false;
        while (ui::Measure(name, ts).x > r.width - 60 * s && name.size() > 4) {
            cut = true;
            name.pop_back();
            while (!name.empty() && ((unsigned char)name.back() & 0xC0) == 0x80) name.pop_back();
            if (!name.empty() && ((unsigned char)name.back() & 0x80)) name.pop_back();
        }
        if (cut) name += "…";
        ui::Text(name, r.x + 28 * s, r.y + (r.height - ts) / 2 - 1, ts, c);
        for (int k = 0; k < g_progress.stars[i]; ++k)
            ui::Star({ r.x + r.width - 34 * s + k * 11 * s, r.y + r.height / 2 }, 5.5f * s, pal::accent);
    }

    // Vùng nội dung chính
    Rectangle main = { sb.x + sb.width + 1, sb.y, win.x + win.width - (sb.x + sb.width) - 3, sb.height };
    // Các "tab" bước
    float tabY = main.y + 8 * s;
    float tx = main.x + 16 * s;
    for (int st = 0; st < StepCount(); ++st) {
        std::string label;
        if (st == 0) label = "câu_chuyện.txt";
        else if (st <= PageCount()) label = TextFormat("phần_%d.cpp", st);
        else if (IsResultStep(st)) label = "kết_quả";
        else label = TextFormat("quiz_%d", QuizIndex(st) + 1);
        float tw = ui::Measure(label, 13 * s, FS_MONO).x + 20 * s;
        if (tx + tw > main.x + main.width - 10 * s) break;
        Rectangle r = { tx, tabY, tw, 26 * s };
        bool cur = st == S.step;
        bool reachable = st <= S.step || (st == S.step + 1 && CanGoNext());
        DrawRectangleRec(r, cur ? pal::panel2 : pal::bg);
        if (cur) DrawRectangle((int)r.x, (int)(r.y + r.height - 2 * s), (int)r.width, (int)(2 * s), pal::accent);
        ui::Text(label, r.x + 10 * s, r.y + 6 * s, 13 * s, cur ? pal::text : (reachable ? pal::dim : Color{ 70, 76, 92, 255 }), FS_MONO);
        if (reachable && !cur && ui::Hover(r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) GoTo(st);
        tx += tw + 4 * s;
    }

    float barH = 64 * s;
    Rectangle content = { main.x + 36 * s, main.y + 54 * s, std::min(main.width - 72 * s, 900 * s), main.height - 54 * s - barH };
    content.x = main.x + (main.width - content.width) / 2;

    BeginScissorMode((int)main.x, (int)content.y - 4, (int)main.width, (int)content.height + 4);
    Rectangle c = { content.x, content.y + 10 * s - S.scroll, content.width, 0 };
    float h;
    if (S.step == 0) h = DrawStory(c, s);
    else if (S.step <= PageCount()) h = DrawPage(c, s, S.step - 1);
    else if (IsQuizStep(S.step)) h = DrawQuiz(c, s, QuizIndex(S.step));
    else h = DrawResult(c, s);
    EndScissorMode();
    S.maxScroll = h + 30 * s - content.height;
    if (S.maxScroll > 0) {
        float trackH = content.height;
        float thumbH = std::max(30 * s, trackH * content.height / (h + 30 * s));
        float ty = content.y + (trackH - thumbH) * (S.scroll / S.maxScroll);
        DrawRectangleRounded({ main.x + main.width - 10 * s, ty, 5 * s, thumbH }, 1, 4, pal::line);
    }

    // Thanh nút dưới cùng
    Rectangle bar = { main.x, main.y + main.height - barH, main.width, barH };
    DrawRectangleRec(bar, pal::panel);
    float bw = 190 * s, bh = 42 * s, by = bar.y + (barH - bh) / 2;
    if (ui::Button({ bar.x + 16 * s, by, 150 * s, bh }, "Thoát (Esc)", true, pal::bad)) CloseScreen();
    if (S.step > 0 && ui::Button({ bar.x + bar.width - bw * 2 - 30 * s, by, bw, bh }, "← Trước")) GoTo(S.step - 1);
    if (IsResultStep(S.step)) {
        if (ui::Button({ bar.x + bar.width - bw - 16 * s, by, bw, bh }, "Về văn phòng", true, pal::good)) CloseScreen();
    } else {
        std::string nextLabel = (S.step == PageCount()) ? "Làm thử thách →" : "Tiếp →";
        if (ui::Button({ bar.x + bar.width - bw - 16 * s, by, bw, bh }, nextLabel, CanGoNext())) GoTo(S.step + 1);
    }
    // Chấm tiến độ
    float dotsW = StepCount() * 16 * s;
    float dx = bar.x + 180 * s + (bar.width - 180 * s - bw * 2 - 40 * s - dotsW) / 2;
    for (int st = 0; st < StepCount(); ++st)
        DrawCircle((int)(dx + st * 16 * s), (int)(bar.y + barH / 2), (st == S.step ? 5 : 3.5f) * s,
                   st <= S.step ? pal::accent : pal::line);
}
