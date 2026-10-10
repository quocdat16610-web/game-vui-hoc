#include "Audio.h"
#include "raylib.h"
#include <cmath>
#include <cstdint>
#include <vector>

namespace {
    const int RATE = 22050;                 // số mẫu mỗi giây
    const float TAU = 6.2831853f;

    Sound g_sounds[sfx::COUNT] = {};
    Sound g_music = {};
    bool g_ready = false, g_muted = false, g_musicPlaying = false;
    unsigned int g_seed = 12345;

    float Noise() {                          // nhiễu trắng: số ngẫu nhiên -1..1
        g_seed = g_seed * 1664525u + 1013904223u;
        return ((g_seed >> 9) & 0xFFFF) / 32767.5f - 1.0f;
    }
    float Square(float phase, float duty = 0.5f) { return std::fmod(phase, 1.0f) < duty ? 1.0f : -1.0f; }
    float Triangle(float phase) { float p = std::fmod(phase, 1.0f); return 4.0f * std::fabs(p - 0.5f) - 1.0f; }

    // Bộ đệm âm thanh dạng số thực, đổi sang Sound của raylib
    struct Buf {
        std::vector<float> s;
        explicit Buf(float seconds) : s((size_t)(seconds * RATE), 0.0f) {}
        size_t N() const { return s.size(); }
        float T(size_t i) const { return (float)i / RATE; }
    };

    Sound ToSound(const Buf& b, float gain = 0.8f) {
        std::vector<int16_t> pcm(b.N());
        for (size_t i = 0; i < b.N(); ++i) {
            float v = std::tanh(b.s[i] * gain);           // nén mềm, tránh rè
            pcm[i] = (int16_t)(v * 32000.0f);
        }
        Wave w = {};
        w.frameCount = (unsigned int)pcm.size();
        w.sampleRate = RATE;
        w.sampleSize = 16;
        w.channels = 1;
        w.data = pcm.data();
        return LoadSoundFromWave(w);                      // raylib sao chép dữ liệu
    }

    // "Thump": sóng sin trượt tần số từ f0 xuống f1, tắt dần
    void AddThump(Buf& b, float f0, float f1, float decay, float amp, float start = 0) {
        float phase = 0;
        for (size_t i = (size_t)(start * RATE); i < b.N(); ++i) {
            float t = b.T(i) - start;
            float f = f1 + (f0 - f1) * std::exp(-t * 18.0f);
            phase += f / RATE;
            b.s[i] += std::sin(TAU * phase) * std::exp(-t * decay) * amp;
        }
    }
    // Nhiễu đã lọc thông thấp (one-pole low-pass), tắt dần
    void AddNoise(Buf& b, float cutoff, float decay, float amp, float start = 0, float attack = 0) {
        float lp = 0, k = 1.0f - std::exp(-TAU * cutoff / RATE);
        for (size_t i = (size_t)(start * RATE); i < b.N(); ++i) {
            float t = b.T(i) - start;
            lp += (Noise() - lp) * k;
            float env = std::exp(-t * decay);
            if (attack > 0) env *= std::fmin(1.0f, t / attack);
            b.s[i] += lp * env * amp;
        }
    }
    void AddTone(Buf& b, float freq, float start, float len, float amp, bool square = true, float slideTo = -1) {
        float phase = 0;
        size_t i0 = (size_t)(start * RATE), i1 = (size_t)((start + len) * RATE);
        for (size_t i = i0; i < i1 && i < b.N(); ++i) {
            float t = (float)(i - i0) / RATE;
            float f = slideTo > 0 ? freq + (slideTo - freq) * (t / len) : freq;
            phase += f / RATE;
            float env = std::fmin(1.0f, t / 0.005f) * std::fmin(1.0f, (len - t) / 0.02f);
            b.s[i] += (square ? Square(phase) * 0.5f : std::sin(TAU * phase)) * env * amp;
        }
    }

    void BuildSounds() {
        { Buf b(0.22f); AddThump(b, 180, 70, 22, 1.0f); AddNoise(b, 2500, 40, 0.9f); g_sounds[sfx::PUNCH] = ToSound(b); }
        { Buf b(0.35f); AddThump(b, 140, 45, 12, 1.2f); AddNoise(b, 1800, 30, 0.8f); g_sounds[sfx::KICK] = ToSound(b); }
        {
            Buf b(0.35f);
            for (float f : { 920.0f, 1370.0f, 2230.0f }) {
                for (size_t i = 0; i < b.N(); ++i) b.s[i] += std::sin(TAU * f * b.T(i)) * std::exp(-b.T(i) * 14) * 0.3f;
            }
            AddNoise(b, 6000, 80, 0.5f);
            g_sounds[sfx::BLOCK] = ToSound(b);
        }
        { Buf b(1.2f); AddThump(b, 120, 30, 3.5f, 1.4f); AddNoise(b, 900, 5, 0.7f); AddTone(b, 700, 0.05f, 0.7f, 0.25f, true, 90); g_sounds[sfx::KO] = ToSound(b); }
        {
            Buf b(0.3f);
            float lp = 0;
            for (size_t i = 0; i < b.N(); ++i) {
                float t = b.T(i) / 0.3f;
                float cutoff = 400 + 3000 * std::sin(t * 3.14159f);
                lp += (Noise() - lp) * (1.0f - std::exp(-TAU * cutoff / RATE));
                b.s[i] = lp * std::sin(t * 3.14159f) * 0.9f;
            }
            g_sounds[sfx::WHOOSH] = ToSound(b);
        }
        { Buf b(0.16f); AddTone(b, 280, 0, 0.15f, 0.5f, true, 640); g_sounds[sfx::JUMP] = ToSound(b); }
        { Buf b(0.05f); AddTone(b, 1250, 0, 0.045f, 0.5f, false); g_sounds[sfx::CLICK] = ToSound(b); }
        { Buf b(0.26f); AddTone(b, 660, 0, 0.1f, 0.4f); AddTone(b, 990, 0.1f, 0.15f, 0.4f); g_sounds[sfx::CORRECT] = ToSound(b); }
        { Buf b(0.3f); AddTone(b, 190, 0, 0.12f, 0.45f); AddTone(b, 150, 0.13f, 0.16f, 0.45f); g_sounds[sfx::WRONG] = ToSound(b); }
        {
            Buf b(0.9f);
            const float notes[] = { 523.3f, 659.3f, 784.0f, 1046.5f };
            for (int i = 0; i < 4; ++i) AddTone(b, notes[i], i * 0.08f, i == 3 ? 0.55f : 0.09f, 0.35f);
            AddTone(b, 1568.0f, 0.32f, 0.5f, 0.12f, false);
            g_sounds[sfx::UNLOCK] = ToSound(b);
        }
        { Buf b(0.06f); AddNoise(b, 700, 60, 0.5f); g_sounds[sfx::STEP] = ToSound(b); }
    }

    // Nhạc nền chiptune 8 ô nhịp, vòng hợp âm Am - F - C - G
    void BuildMusic() {
        const float BPM = 132, beat = 60.0f / BPM;
        const int bars = 8;
        Buf b(bars * 4 * beat + 0.05f);
        const float roots[4] = { 110.0f, 87.31f, 130.81f, 98.0f };            // A2 F2 C3 G2
        const int scale[5] = { 0, 3, 5, 7, 10 };                                // ngũ cung thứ
        const int melody[16] = { 0, 2, 3, 2, 4, 3, 2, 0, 1, 2, 4, 5, 4, 2, 1, -1 };
        for (int bar = 0; bar < bars; ++bar) {
            float root = roots[bar % 4];
            for (int e = 0; e < 8; ++e) {                                       // bass móc nốt 1/8
                float t = (bar * 4 + e * 0.5f) * beat;
                float f = root * ((e % 2) ? 2.0f : 1.0f);
                size_t i0 = (size_t)(t * RATE), i1 = (size_t)((t + beat * 0.45f) * RATE);
                for (size_t i = i0; i < i1 && i < b.N(); ++i) {
                    float tt = (float)(i - i0) / RATE;
                    b.s[i] += Triangle(f * tt) * 0.35f * std::exp(-tt * 3);
                }
            }
            for (int q = 0; q < 4; ++q) {                                       // trống: kick + hi-hat
                float t = (bar * 4 + q) * beat;
                if (q % 2 == 0) AddThump(b, 150, 50, 14, 0.7f, t);
                AddNoise(b, 7000, 60, 0.12f, t + beat * 0.5f);
                if (q % 2 == 1) AddNoise(b, 3000, 25, 0.25f, t);                // snare
            }
            if (bar % 2 == 1 || bar >= 4) {                                     // giai điệu
                for (int n = 0; n < 8; ++n) {
                    int idx = melody[(bar % 2) * 8 + n];
                    if (idx < 0) continue;
                    int semi = scale[idx % 5] + 12 * (idx / 5);
                    float f = 440.0f * std::pow(2.0f, semi / 12.0f) * (bar >= 6 ? 2.0f : 1.0f);
                    float t = (bar * 4 + n * 0.5f) * beat;
                    float phase = 0;
                    size_t i0 = (size_t)(t * RATE), i1 = (size_t)((t + beat * 0.42f) * RATE);
                    for (size_t i = i0; i < i1 && i < b.N(); ++i) {
                        float tt = (float)(i - i0) / RATE;
                        phase += f * (1.0f + 0.004f * std::sin(TAU * 6 * tt)) / RATE;   // rung nhẹ (vibrato)
                        b.s[i] += Square(phase, 0.25f) * 0.13f * std::exp(-tt * 4);
                    }
                }
            }
        }
        g_music = ToSound(b, 0.9f);
    }
}

void sfx::Init() {
    InitAudioDevice();
    g_ready = IsAudioDeviceReady();
    if (!g_ready) { TraceLog(LOG_WARNING, "AUDIO: khong mo duoc thiet bi am thanh, game chay khong tieng"); return; }
    BuildSounds();
    BuildMusic();
    SetMasterVolume(0.8f);
}

void sfx::Shutdown() {
    if (!g_ready) return;
    for (Sound& s : g_sounds) UnloadSound(s);
    UnloadSound(g_music);
    CloseAudioDevice();
}

void sfx::Play(Id id, float pitchVariation, float volume) {
    if (!g_ready || g_muted) return;
    Sound& s = g_sounds[id];
    // Đổi cao độ ngẫu nhiên một chút để tiếng lặp lại không bị nhàm
    float p = 1.0f + (GetRandomValue(-100, 100) / 100.0f) * pitchVariation;
    SetSoundPitch(s, p);
    SetSoundVolume(s, volume);
    PlaySound(s);
}

void sfx::SetCustomPunch(const std::vector<float>& samples, int sampleRate) {
    if (!g_ready || samples.empty()) return;
    std::vector<int16_t> pcm(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        float v = std::isfinite(samples[i]) ? std::tanh(samples[i]) : 0.0f;
        pcm[i] = (int16_t)(v * 30000.0f);
    }
    Wave w = {};
    w.frameCount = (unsigned int)pcm.size();
    w.sampleRate = (unsigned int)sampleRate;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = pcm.data();
    UnloadSound(g_sounds[PUNCH]);
    g_sounds[PUNCH] = LoadSoundFromWave(w);
}

void sfx::Update(bool wantMusic) {
    if (!g_ready) return;
    bool on = wantMusic && !g_muted;
    if (on && !IsSoundPlaying(g_music)) { SetSoundVolume(g_music, 0.45f); PlaySound(g_music); }
    if (!on && g_musicPlaying) StopSound(g_music);
    g_musicPlaying = on;
}

void sfx::ToggleMute() {
    g_muted = !g_muted;
    if (g_ready && g_muted) StopSound(g_music);
}

bool sfx::Muted() { return g_muted; }
