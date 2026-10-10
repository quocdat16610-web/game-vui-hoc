#include "Common.h"
#include <fstream>

Progress g_progress;

static const char* SAVE_FILE = "save.txt";

void Progress::Load() {
    std::ifstream in(SAVE_FILE);
    if (!in) return;                       // chưa có file -> giữ mặc định
    std::string key;
    while (in >> key) {
        if (key == "completed") in >> completed;
        else if (key == "wins") in >> wins;
        else if (key == "intro") in >> introSeen;
        else if (key == "gfx") in >> gfxQuality;
        else if (key == "stars") for (int& s : stars) in >> s;
    }
    if (completed < 0) completed = 0;
    if (completed > F_COUNT) completed = F_COUNT;
}

void Progress::Save() const {
    std::ofstream out(SAVE_FILE);
    out << "completed " << completed << "\n";
    out << "wins " << wins << "\n";
    out << "intro " << introSeen << "\n";
    out << "gfx " << gfxQuality << "\n";
    out << "stars";
    for (int s : stars) out << " " << s;
    out << "\n";
}

int Progress::TotalStars() const {
    int t = 0;
    for (int s : stars) t += s;
    return t;
}

float Clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

float Approach(float cur, float target, float step) {
    if (cur < target) return (cur + step > target) ? target : cur + step;
    return (cur - step < target) ? target : cur - step;
}

Color Fade2(Color c, float a) {
    c.a = (unsigned char)(c.a * Clamp01(a));
    return c;
}

Color Lerp(Color a, Color b, float t) {
    t = Clamp01(t);
    return Color{ (unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
                  (unsigned char)(a.b + (b.b - a.b) * t), (unsigned char)(a.a + (b.a - a.a) * t) };
}
