// =============================================================================
//  BÀI 14 — KIẾN TRÚC GAME: Scene, luật chơi dạng dữ liệu, ECS
//  Chạy: g++ -std=c++17 14_kien_truc.cpp -o bai14 && ./bai14
// =============================================================================
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// 1. SCENE: mỗi màn hình là một lớp, game loop chỉ gọi Update/Draw
// ---------------------------------------------------------------------------
class Scene {
public:
    virtual ~Scene() = default;
    virtual void Update(float dt) = 0;
    virtual void Draw() const = 0;
    virtual std::unique_ptr<Scene> Next() { return nullptr; }   // muốn đổi màn?
    virtual bool Quit() const { return false; }
};

// ---------------------------------------------------------------------------
// 2. LUẬT CHƠI LÀ DỮ LIỆU
// ---------------------------------------------------------------------------
struct MatchRules {
    int   hiepCanThang = 2;
    float thoiGianHiep = 5;     // giây (ngắn cho dễ xem)
};

// ---------------------------------------------------------------------------
// 3. ECS mini: entity = chỉ số, component = mảng dữ liệu, system = hàm
// ---------------------------------------------------------------------------
struct Health   { int hp, maxHp; };
struct Attacker { int dmg; float cooldown, timer; };

struct World {
    std::vector<std::string> names;      // component "tên"
    std::vector<Health>      healths;    // component "máu"
    std::vector<Attacker>    attackers;  // component "tấn công"
    int Create(const std::string& n, int hp, int dmg, float cd) {
        names.push_back(n);
        healths.push_back({ hp, hp });
        attackers.push_back({ dmg, cd, cd });
        return (int)names.size() - 1;    // entity chỉ là một ID
    }
};

// System: xử lý tất cả entity có component Attacker
void AttackSystem(World& w, float dt) {
    for (size_t i = 0; i < w.attackers.size(); i++) {
        Attacker& a = w.attackers[i];
        a.timer -= dt;
        if (a.timer > 0) continue;
        a.timer = a.cooldown;
        size_t target = (i + 1) % w.healths.size();
        w.healths[target].hp -= a.dmg + std::rand() % 3;
    }
}
void HealthSystem(World& w) {
    for (auto& h : w.healths) if (h.hp < 0) h.hp = 0;
}

// ---------------------------------------------------------------------------
class ResultScene : public Scene {
public:
    explicit ResultScene(std::string w) : winner_(std::move(w)) {}
    void Update(float) override { frames_++; }
    void Draw() const override { if (frames_ == 1) std::cout << "\n*** " << winner_ << " VO DICH! ***\n"; }
    bool Quit() const override { return frames_ > 1; }
private:
    std::string winner_;
    int frames_ = 0;
};

class FightScene : public Scene {
public:
    FightScene() { NewRound(); }
    void Update(float dt) override {
        timer_ -= dt;
        AttackSystem(world_, dt);
        HealthSystem(world_);
        int a = world_.healths[0].hp, b = world_.healths[1].hp;
        if (a == 0 || b == 0 || timer_ <= 0) {
            int winner = (a > b) ? 0 : 1;            // hết giờ: ai nhiều máu hơn thắng
            wins_[winner]++;
            std::cout << "  -> " << world_.names[winner] << " thang hiep " << round_
                      << (timer_ <= 0 ? " (het gio)" : " (K.O.)") << "\n";
            if (wins_[winner] >= rules_.hiepCanThang) next_ = std::make_unique<ResultScene>(world_.names[winner]);
            else { round_++; NewRound(); }
        }
    }
    void Draw() const override {
        if (++drawCount_ % 10 == 0)
            std::cout << "  [hiep " << round_ << "] " << world_.names[0] << " " << world_.healths[0].hp
                      << "  vs  " << world_.names[1] << " " << world_.healths[1].hp << "\n";
    }
    std::unique_ptr<Scene> Next() override { return std::move(next_); }

private:
    void NewRound() {
        world_ = World{};
        world_.Create("Ninja", 90, 6, 0.4f);
        world_.Create("Robot", 130, 11, 0.9f);
        timer_ = rules_.thoiGianHiep;
        std::cout << "\nHIEP " << round_ << " - DAU!\n";
    }
    MatchRules rules_;
    World world_;
    float timer_ = 0;
    int round_ = 1;
    int wins_[2] = { 0, 0 };
    mutable int drawCount_ = 0;
    std::unique_ptr<Scene> next_;
};

class MenuScene : public Scene {
public:
    void Update(float) override { ready_ = true; }
    void Draw() const override { std::cout << "=== CODE VO DAI ===\n(menu: tu dong bat dau)\n"; }
    std::unique_ptr<Scene> Next() override { return ready_ ? std::make_unique<FightScene>() : nullptr; }
private:
    bool ready_ = false;
};

int main() {
    std::srand(42);
    std::unique_ptr<Scene> scene = std::make_unique<MenuScene>();
    const float DT = 0.1f;
    while (!scene->Quit()) {               // GAME LOOP gọn gàng
        scene->Update(DT);
        scene->Draw();
        if (auto next = scene->Next()) scene = std::move(next);   // đổi màn
    }
    // BÀI TẬP: thêm TrainingScene: máu đối thủ tự hồi đầy sau 2 giây không bị đánh.
    return 0;
}
