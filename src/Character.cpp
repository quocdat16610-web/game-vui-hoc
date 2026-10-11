#include "Character.h"
#include "Gfx.h"
#include "raymath.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>

namespace {
    ModelAnimation* g_anims = nullptr;
    int g_animCount = 0;
    bool g_ready = false;
    std::map<std::string, int> g_byName;
    const float FPS = 60.0f;              // raylib lấy mẫu hoạt ảnh glTF ở ~60 khung/giây

    const char* FILES[(int)CharModel::COUNT] = { "knight", "barbarian", "mage", "rogue", "rogue_hooded" };

    int Find(const char* name) {
        auto it = g_byName.find(name);
        return it == g_byName.end() ? -1 : it->second;
    }

    bool IsWeapon(const char* n) {
        const char* w[] = { "Sword", "Shield", "Axe", "Mug", "Spellbook", "Wand", "Staff", "Knife", "Crossbow", "Throwable" };
        for (const char* s : w) if (std::strstr(n, s)) return true;
        return false;
    }
}

bool chars::Init() {
    const char* path = "assets/models/characters/animations.glb";
    if (!FileExists(path)) return false;
    g_anims = LoadModelAnimations(path, &g_animCount);
    for (int i = 0; i < g_animCount; ++i) g_byName[g_anims[i].name] = i;
    g_ready = g_animCount > 0;
    return g_ready;
}

void chars::Shutdown() {
    if (g_anims) UnloadModelAnimations(g_anims, g_animCount);
    g_anims = nullptr;
    g_ready = false;
}

bool chars::Ready() { return g_ready; }

AnimChar::~AnimChar() {
    if (loaded_) UnloadModel(model_);
}

// Mesh "cứng" (mũ, áo choàng...) không có dữ liệu skin: gắn toàn bộ đỉnh vào xương cha
// với trọng số 1.0 để chúng chuyển động theo xương đó.
static void AttachRigidMesh(Model& m, int idx, int bone) {
    Mesh& me = m.meshes[idx];
    int n = me.vertexCount;
    if (!me.boneIds) me.boneIds = (unsigned char*)RL_CALLOC(n * 4, 1);
    if (!me.boneWeights) me.boneWeights = (float*)RL_CALLOC(n * 4, sizeof(float));
    for (int i = 0; i < n; ++i) {
        me.boneIds[i * 4] = (unsigned char)bone;
        me.boneWeights[i * 4] = 1.0f;
    }
    if (!me.animVertices) {
        me.animVertices = (float*)RL_MALLOC(n * 3 * sizeof(float));
        std::memcpy(me.animVertices, me.vertices, n * 3 * sizeof(float));
    }
    if (!me.animNormals && me.normals) {
        me.animNormals = (float*)RL_MALLOC(n * 3 * sizeof(float));
        std::memcpy(me.animNormals, me.normals, n * 3 * sizeof(float));
    }
    if (!me.boneMatrices) {
        me.boneCount = m.boneCount;
        me.boneMatrices = (Matrix*)RL_CALLOC(m.boneCount, sizeof(Matrix));
        for (int k = 0; k < m.boneCount; ++k) me.boneMatrices[k] = MatrixIdentity();
    }
}

bool AnimChar::Load(CharModel kind) {
    if (loaded_) { UnloadModel(model_); loaded_ = false; }
    kind_ = kind;
    char glb[128], att[128];
    std::snprintf(glb, sizeof(glb), "assets/models/characters/%s.glb", FILES[(int)kind]);
    std::snprintf(att, sizeof(att), "assets/models/characters/%s.attach", FILES[(int)kind]);
    if (!chars::Ready() || !FileExists(glb) || !FileExists(att)) return false;
    model_ = LoadModel(glb);
    if (model_.meshCount == 0) return false;
    hidden_.assign(model_.meshCount, false);

    FILE* f = std::fopen(att, "r");
    if (f) {
        int idx;
        char name[128], bone[32];
        while (std::fscanf(f, "%d %127s %31s", &idx, name, bone) == 3) {
            if (idx < 0 || idx >= model_.meshCount || std::strcmp(bone, "skin") == 0) continue;
            int b = std::atoi(bone);
            if (IsWeapon(name)) { hidden_[idx] = true; continue; }      // đánh tay không: ẩn vũ khí
            if (b >= 0 && b < model_.boneCount) AttachRigidMesh(model_, idx, b);
            else hidden_[idx] = true;
        }
        std::fclose(f);
    }
    // Mesh nào vẫn chưa có dữ liệu xương thì ẩn đi để tránh lỗi khi cập nhật hoạt ảnh
    for (int i = 0; i < model_.meshCount; ++i)
        if (!model_.meshes[i].boneWeights || !model_.meshes[i].boneIds || !model_.meshes[i].boneMatrices) hidden_[i] = true;
    loaded_ = true;
    anim_ = -1;
    Play("Unarmed_Idle");
    return true;
}

void AnimChar::Play(const char* name, bool loop, float speed) {
    int a = Find(name);
    if (a < 0) return;
    scrubbed_ = false;
    speed_ = speed;
    if (a == anim_) { loop_ = loop; return; }
    anim_ = a;
    loop_ = loop;
    time_ = 0;
}

void AnimChar::Scrub(const char* name, float progress, float from, float to) {
    int a = Find(name);
    if (a < 0) return;
    anim_ = a;
    scrubbed_ = true;
    if (progress < 0) progress = 0;
    if (progress > 1) progress = 1;
    int fc = g_anims[a].frameCount;
    scrubFrame_ = (int)((from + (to - from) * progress) * (fc - 1));
}

int AnimChar::CurrentFrame() const {
    if (anim_ < 0) return 0;
    int fc = g_anims[anim_].frameCount;
    if (scrubbed_) return scrubFrame_ < fc ? scrubFrame_ : fc - 1;
    int f = (int)(time_ * FPS);
    if (loop_) return fc > 0 ? f % fc : 0;
    return f < fc ? f : fc - 1;
}

bool AnimChar::Finished() const {
    if (anim_ < 0 || loop_) return false;
    return (int)(time_ * FPS) >= g_anims[anim_].frameCount - 1;
}

void AnimChar::Update(float dt) {
    if (!loaded_ || anim_ < 0) return;
    time_ += dt * speed_;
    int f = CurrentFrame();
    if (f != lastFrame_) {
        // Không dùng UpdateModelAnimation trực tiếp vì cần bỏ qua mesh đã ẩn
        // (raylib cập nhật mọi mesh, mesh thiếu dữ liệu xương sẽ gây lỗi).
        int saved = model_.meshCount;
        std::vector<int> order;
        for (int i = 0; i < saved; ++i) if (!hidden_[i]) order.push_back(i);
        // Hoán đổi các mesh hiện lên đầu mảng để raylib chỉ duyệt chúng
        std::vector<Mesh> backup(model_.meshes, model_.meshes + saved);
        for (size_t k = 0; k < order.size(); ++k) model_.meshes[k] = backup[order[k]];
        model_.meshCount = (int)order.size();
        UpdateModelAnimation(model_, g_anims[anim_], f);
        for (size_t k = 0; k < order.size(); ++k) backup[order[k]] = model_.meshes[k];
        std::memcpy(model_.meshes, backup.data(), saved * sizeof(Mesh));
        model_.meshCount = saved;
        lastFrame_ = f;
    }
}

void AnimChar::Draw(Vector3 pos, float yawDeg, float scale, float flash, float tiltDeg) {
    if (!loaded_) return;
    Matrix t = MatrixMultiply(MatrixScale(scale, scale, scale), MatrixRotateX(tiltDeg * DEG2RAD));
    t = MatrixMultiply(t, MatrixRotateY(yawDeg * DEG2RAD));
    t = MatrixMultiply(t, MatrixTranslate(pos.x, pos.y, pos.z));
    for (int i = 0; i < model_.meshCount; ++i) {
        if (hidden_[i]) continue;
        gfx::DrawMeshLit(model_.meshes[i], model_.materials[model_.meshMaterial[i]], t, flash);
    }
}
