// =============================================================================
//  World.cpp — THẾ GIỚI MỞ "Đảo Lập Trình": người chơi TỰ XÂY mọi thứ
//  bằng chuột (chế độ xây, phím B) hoặc bằng CODE C++ (bản thiết kế, phím C).
// =============================================================================
#include "World.h"
#include "Audio.h"
#include "Character.h"
#include "CodeEditor.h"
#include "Common.h"
#include "Gfx.h"
#include "Lessons.h"
#include "MiniCpp.h"
#include "UI.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

namespace {

// -----------------------------------------------------------------------------
//  Hằng số & lưới lục giác
// -----------------------------------------------------------------------------
const float WS = 6.5f;              // phóng to model KayKit (1 ô lục giác rộng 13 mét)
const int   RAD = 11;               // bán kính đảo tính theo ô
const float SQ3 = 1.7320508f;
const int   CHUNK = 6;              // gộp lưới theo khu 6x6 ô
const char* SAVE_FILE = "world.txt";

// 6 hướng láng giềng (tọa độ axial), theo góc 0°, 60°, ... trong mặt phẳng xz
const int DQ[6] = { 1, 0, -1, -1, 0, 1 };
const int DR[6] = { 0, 1, 1, 0, -1, -1 };

Vector3 HexCenter(int q, int r) { return { WS * 2.0f * (q + r * 0.5f), 0.0f, WS * SQ3 * r }; }
int HexDist(int q, int r) { return (std::abs(q) + std::abs(r) + std::abs(q + r)) / 2; }

void PixelToHex(float x, float z, int& q, int& r) {
    float fr = z / (WS * SQ3);
    float fq = x / (2.0f * WS) - fr * 0.5f;
    float fs = -fq - fr;
    int rq = (int)std::round(fq), rr = (int)std::round(fr), rs = (int)std::round(fs);
    float dq = std::fabs(rq - fq), dr = std::fabs(rr - fr), ds = std::fabs(rs - fs);
    if (dq > dr && dq > ds) rq = -rr - rs;
    else if (dr > ds) rr = -rq - rs;
    q = rq;
    r = rr;
}

// -----------------------------------------------------------------------------
//  Danh mục vật thể có thể xây
// -----------------------------------------------------------------------------
enum Item {
    NONE = 0,
    // Mặt đất
    CO, NUOC, DUONG,
    // Thiên nhiên
    CAY, CAY_THONG, RUNG, BUI_CAY, DA, NUI, DOI,
    // Nhà cửa
    NHA, NHA_DO, NHA_XANH, NHA_VANG, CHO, GIENG, LO_REN, COI_XAY, COI_NUOC, XUONG_GO, THAP,
    DAU_TRUONG, THU_VIEN, KHO_THOC,
    // Đồ vật
    DEN, LEU, THUNG, THUNG_GO, BIA_TAP, CO_CO, XE_DAY, BAO_TAI,
    STUDIO,
    ITEM_COUNT
};

enum Cat { CAT_GROUND, CAT_NATURE, CAT_BUILDING, CAT_PROP, CAT_SPECIAL };

struct ItemDef {
    Item id;
    const char* code;      // tên hằng số trong code người chơi
    const char* name;      // tên tiếng Việt
    const char* file;      // model (rỗng = tự vẽ)
    Cat cat;
    bool blocks;           // chặn cả ô
    float radius;          // bán kính va chạm (mét), 0 = đi xuyên được
    int unlock;            // cần học xong bao nhiêu bài
    bool house;            // có dân làng đến ở
};

const ItemDef ITEMS[] = {
    { NONE, "TRONG", "Trống", "", CAT_SPECIAL, false, 0, 0, false },
    { CO, "CO", "Cỏ", "hex_grass", CAT_GROUND, false, 0, 0, false },
    { NUOC, "NUOC", "Nước", "hex_water", CAT_GROUND, true, 0, 0, false },
    { DUONG, "DUONG", "Đường", "hex_road_A", CAT_GROUND, false, 0, 0, false },
    { CAY, "CAY", "Cây", "tree_single_A", CAT_NATURE, false, 1.0f, 0, false },
    { CAY_THONG, "CAY_THONG", "Cây thông", "tree_single_B", CAT_NATURE, false, 1.0f, 0, false },
    { RUNG, "RUNG", "Rừng", "trees_A_large", CAT_NATURE, true, 0, 0, false },
    { BUI_CAY, "BUI_CAY", "Cụm cây", "trees_B_medium", CAT_NATURE, false, 3.0f, 0, false },
    { DA, "DA", "Đá", "rock_single_A", CAT_NATURE, false, 1.4f, 0, false },
    { NUI, "NUI", "Núi", "mountain_A_grass_trees", CAT_NATURE, true, 0, 0, false },
    { DOI, "DOI", "Đồi", "hills_A_trees", CAT_NATURE, true, 0, 0, false },
    { NHA, "NHA", "Nhà xanh dương", "building_home_A_blue", CAT_BUILDING, false, 3.4f, 0, true },
    { NHA_DO, "NHA_DO", "Nhà đỏ", "building_home_B_red", CAT_BUILDING, false, 3.4f, 0, true },
    { NHA_XANH, "NHA_XANH", "Nhà xanh lá", "building_home_A_green", CAT_BUILDING, false, 3.4f, 0, true },
    { NHA_VANG, "NHA_VANG", "Nhà vàng", "building_home_B_yellow", CAT_BUILDING, false, 3.4f, 0, true },
    { CHO, "CHO", "Chợ", "building_market_green", CAT_BUILDING, false, 4.5f, 1, false },
    { GIENG, "GIENG", "Giếng nước", "building_well_blue", CAT_BUILDING, false, 2.0f, 0, false },
    { LO_REN, "LO_REN", "Lò rèn", "building_blacksmith_red", CAT_BUILDING, false, 4.0f, 2, false },
    { COI_XAY, "COI_XAY", "Cối xay gió", "building_windmill_yellow", CAT_BUILDING, false, 3.5f, 2, false },
    { COI_NUOC, "COI_NUOC", "Cối xay nước", "building_watermill_yellow", CAT_BUILDING, false, 4.0f, 3, false },
    { XUONG_GO, "XUONG_GO", "Xưởng gỗ", "building_lumbermill_yellow", CAT_BUILDING, false, 4.0f, 3, false },
    { THAP, "THAP", "Tháp canh", "building_tower_A_blue", CAT_BUILDING, false, 3.5f, 4, false },
    { DAU_TRUONG, "DAU_TRUONG", "Đấu trường", "building_castle_blue", CAT_BUILDING, true, 0, 2, false },
    { THU_VIEN, "THU_VIEN", "Thư viện", "building_church_green", CAT_BUILDING, false, 3.6f, 1, false },
    { KHO_THOC, "KHO_THOC", "Kho thóc", "building_grain", CAT_BUILDING, false, 3.0f, 2, false },
    { DEN, "DEN", "Đèn đường", "", CAT_PROP, false, 0.4f, 0, false },
    { LEU, "LEU", "Lều", "tent", CAT_PROP, false, 1.8f, 0, false },
    { THUNG, "THUNG", "Thùng gỗ", "barrel", CAT_PROP, false, 0.8f, 0, false },
    { THUNG_GO, "THUNG_GO", "Hòm gỗ", "crate_A_big", CAT_PROP, false, 0.9f, 0, false },
    { BIA_TAP, "BIA_TAP", "Bia tập bắn", "target", CAT_PROP, false, 0.9f, 0, false },
    { CO_CO, "CO_CO", "Lá cờ", "flag_red", CAT_PROP, false, 0.3f, 0, false },
    { XE_DAY, "XE_DAY", "Xe đẩy", "wheelbarrow", CAT_PROP, false, 0.9f, 0, false },
    { BAO_TAI, "BAO_TAI", "Bao tải", "sack", CAT_PROP, false, 0.6f, 0, false },
    { STUDIO, "STUDIO", "Studio của Minh", "building_tavern_red", CAT_SPECIAL, false, 4.2f, 0, false },
};

const ItemDef& Def(int id) { return ITEMS[(id >= 0 && id < ITEM_COUNT) ? id : 0]; }
bool IsObject(int id) { return id >= CAY && id < ITEM_COUNT; }

// Mặt nạ hướng nối của 13 loại ô đường (bit e = có đường ở cạnh góc e*60°), phân tích từ model
struct RoadTile { const char* file; int mask; };
const RoadTile ROADS[] = {
    { "hex_road_A", 9 },  { "hex_road_B", 40 }, { "hex_road_C", 24 }, { "hex_road_D", 42 }, { "hex_road_E", 41 },
    { "hex_road_F", 11 }, { "hex_road_G", 28 }, { "hex_road_H", 29 }, { "hex_road_I", 54 }, { "hex_road_J", 15 },
    { "hex_road_K", 62 }, { "hex_road_L", 63 }, { "hex_road_M", 8 } };

int RotMask(int mask, int k) {           // xoay k*60°: cạnh e -> cạnh (e - k)
    int r = 0;
    for (int e = 0; e < 6; ++e)
        if (mask & (1 << e)) r |= 1 << (((e - k) % 6 + 6) % 6);
    return r;
}

// -----------------------------------------------------------------------------
//  Dữ liệu
// -----------------------------------------------------------------------------
struct Cell {
    int q = 0, r = 0;
    int tile = CO;          // CO / NUOC / DUONG
    int baseTile = CO;      // mặt đất lúc sinh đảo
    int item = NONE;        // vật thể trên ô
    int rot = 0;            // 0..5 (bội số 60°)
    int chunk = 0;
};

struct Proto {                                   // model mẫu (chỉ giữ dữ liệu đỉnh)
    Model model = {};
    bool ok = false;
};

struct Chunk {
    Mesh mesh = {};
    bool has = false, dirty = true;
};

struct Npc {
    std::unique_ptr<AnimChar> ch;
    std::string name;
    Vector3 pos{}, target{}, home{};
    float yaw = 0, wait = 0;
    bool walking = false;
};

struct Pending { int q, r, item, rot; };          // thao tác xây từ code (hiện dần từng cái)

struct Quest { const char* text; bool done; };

struct WorldState {
    bool ready = false;
    std::vector<Cell> cells;
    std::map<long long, int> index;
    std::map<std::string, Proto> protos;
    Texture2D atlas = {};
    Material mat = {};
    std::vector<Chunk> chunks;
    int chunkW = 1;

    // Người chơi
    AnimChar player;
    Vector3 pos{}, vel{};
    float yaw = 0;
    bool onGround = true, running = false;
    Camera3D cam = {};
    float camYaw = 0.6f, camPitch = 0.42f, camDist = 11.0f;
    Vector3 camPos{}, camTarget{};

    // Xây dựng
    bool build = false;
    int sel = CAY, selRot = 1;
    int hoverQ = 0, hoverR = 0;
    bool hoverOk = false;
    bool blueprint = false;               // cửa sổ code
    CodeEditor editor;
    bool editorLoaded = false;
    std::vector<mcpp::Error> errors;
    std::string output;
    int exampleIdx = 0;
    std::vector<Pending> pending;
    float pendingTimer = 0;
    std::vector<std::vector<Cell>> history;   // để hoàn tác lần chạy code
    int lastRunPlaced = 0, lastRunHouses = 0;

    // Thế giới sống
    float hour = 9.0f;
    std::vector<Npc> npcs;
    int coins = 0;
    std::vector<Quest> quests;
    float t = 0;
    std::string sayWho, sayText;
    float sayTime = 0;

    // Hội thoại
    int talkNpc = -1;
    int quizLesson = -1, quizQ = -1, quizPicked = -1;
    std::vector<int> quizOrder;

    RenderTexture2D minimap = {};
    bool minimapDirty = true;
};
WorldState W;

long long Key(int q, int r) { return (long long)(q + 1000) * 4000 + (r + 1000); }
Cell* CellAt(int q, int r) {
    auto it = W.index.find(Key(q, r));
    return it == W.index.end() ? nullptr : &W.cells[it->second];
}

// -----------------------------------------------------------------------------
//  Noise để sinh đảo (value noise nhiều tầng)
// -----------------------------------------------------------------------------
float Hash(int x, int y) {
    unsigned int h = (unsigned int)(x * 374761393 + y * 668265263);
    h = (h ^ (h >> 13)) * 1274126177u;
    return ((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
}
float Noise(float x, float y) {
    int xi = (int)std::floor(x), yi = (int)std::floor(y);
    float fx = x - xi, fy = y - yi;
    fx = fx * fx * (3 - 2 * fx);
    fy = fy * fy * (3 - 2 * fy);
    float a = Hash(xi, yi), b = Hash(xi + 1, yi), c = Hash(xi, yi + 1), d = Hash(xi + 1, yi + 1);
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}
float Fbm(float x, float y) { return Noise(x, y) * 0.6f + Noise(x * 2.1f, y * 2.1f) * 0.3f + Noise(x * 4.3f, y * 4.3f) * 0.1f; }

// -----------------------------------------------------------------------------
//  Model mẫu & gộp lưới
// -----------------------------------------------------------------------------
Proto* GetProto(const std::string& name) {
    if (name.empty()) return nullptr;
    auto it = W.protos.find(name);
    if (it != W.protos.end()) return it->second.ok ? &it->second : nullptr;
    Proto& p = W.protos[name];
    std::string path = "assets/models/world/" + name + ".gltf";
    if (!FileExists(path.c_str())) return nullptr;
    p.model = LoadModel(path.c_str());
    p.ok = p.model.meshCount > 0;
    // Mọi model dùng chung 1 texture atlas: bỏ texture riêng để tiết kiệm bộ nhớ đồ họa
    for (int i = 0; i < p.model.materialCount; ++i) {
        Texture2D& tx = p.model.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture;
        if (tx.id != 0 && tx.id != rlGetTextureIdDefault()) { UnloadTexture(tx); tx = Texture2D{ rlGetTextureIdDefault(), 1, 1, 1, 7 }; }
    }
    return p.ok ? &p : nullptr;
}

struct MeshBuilder {
    std::vector<float> v, n, t;
    std::vector<unsigned char> c;           // màu đỉnh: nhân thêm vào màu texture
    void Add(const Model& m, const Matrix& xf, Color tint = WHITE) {
        Matrix rot = xf;
        rot.m12 = rot.m13 = rot.m14 = 0;
        for (int i = 0; i < m.meshCount; ++i) {
            const Mesh& me = m.meshes[i];
            int count = me.indices ? me.triangleCount * 3 : me.vertexCount;
            for (int k = 0; k < count; ++k) {
                int idx = me.indices ? me.indices[k] : k;
                Vector3 p = Vector3Transform({ me.vertices[idx * 3], me.vertices[idx * 3 + 1], me.vertices[idx * 3 + 2] }, xf);
                Vector3 nn = me.normals ? Vector3Normalize(Vector3Transform({ me.normals[idx * 3], me.normals[idx * 3 + 1], me.normals[idx * 3 + 2] }, rot))
                                        : Vector3{ 0, 1, 0 };
                v.insert(v.end(), { p.x, p.y, p.z });
                n.insert(n.end(), { nn.x, nn.y, nn.z });
                if (me.texcoords) t.insert(t.end(), { me.texcoords[idx * 2], me.texcoords[idx * 2 + 1] });
                else t.insert(t.end(), { 0.0f, 0.0f });
                c.insert(c.end(), { tint.r, tint.g, tint.b, tint.a });
            }
        }
    }
    Mesh Build() {
        Mesh m = {};
        m.vertexCount = (int)(v.size() / 3);
        m.triangleCount = m.vertexCount / 3;
        m.vertices = (float*)MemAlloc((unsigned int)(v.size() * sizeof(float)));
        m.normals = (float*)MemAlloc((unsigned int)(n.size() * sizeof(float)));
        m.texcoords = (float*)MemAlloc((unsigned int)(t.size() * sizeof(float)));
        m.colors = (unsigned char*)MemAlloc((unsigned int)c.size());
        std::copy(c.begin(), c.end(), m.colors);
        std::copy(v.begin(), v.end(), m.vertices);
        std::copy(n.begin(), n.end(), m.normals);
        std::copy(t.begin(), t.end(), m.texcoords);
        UploadMesh(&m, false);
        return m;
    }
};

int RoadMaskAt(const Cell& c) {
    int mask = 0;
    for (int e = 0; e < 6; ++e) {
        Cell* nb = CellAt(c.q + DQ[e], c.r + DR[e]);
        if (!nb) continue;
        bool connect = nb->tile == DUONG || (nb->item != NONE && Def(nb->item).cat == CAT_BUILDING && ((nb->rot + 3) % 6) == e) ||
                       (nb->item == STUDIO && ((nb->rot + 3) % 6) == e);
        if (connect) mask |= 1 << e;
    }
    return mask;
}

// Chọn ô đường + góc xoay khớp với các hướng cần nối
void PickRoad(int mask, std::string& file, int& rot) {
    file = "hex_road_M";
    rot = 0;
    if (mask == 0) { file = "hex_road_A"; return; }
    for (const RoadTile& rt : ROADS)
        for (int k = 0; k < 6; ++k)
            if (RotMask(rt.mask, k) == mask) { file = rt.file; rot = k; return; }
}

Matrix PlaceMatrix(int q, int r, int rot, float extraScale = 1.0f, float yOffset = 0.0f) {
    Vector3 c = HexCenter(q, r);
    float s = WS * extraScale;
    Matrix m = MatrixMultiply(MatrixScale(s, s, s), MatrixRotateY(rot * 60.0f * DEG2RAD));
    return MatrixMultiply(m, MatrixTranslate(c.x, c.y + yOffset, c.z));
}

// Nhà/công trình: cửa (+Z của model) hướng về cạnh e -> góc xoay = 90° - 60°*e
float BuildingYaw(int rot) { return (90.0f - 60.0f * rot) * DEG2RAD; }

void RebuildChunk(int ci) {
    MeshBuilder mb;
    for (Cell& c : W.cells) {
        if (c.chunk != ci) continue;
        // Mặt đất
        if (c.tile == DUONG) {
            std::string f;
            int rot;
            PickRoad(RoadMaskAt(c), f, rot);
            if (Proto* p = GetProto(f)) mb.Add(p->model, PlaceMatrix(c.q, c.r, rot));
        } else if (Proto* p = GetProto(Def(c.tile).file)) {
            // Mỗi ô cỏ sáng/tối lệch nhau một chút cho đỡ đều màu
            Color tint = WHITE;
            if (c.tile == CO) {
                unsigned char k = (unsigned char)(232 + Hash(c.q * 7, c.r * 13) * 23);
                tint = Color{ k, k, k, 255 };
            }
            mb.Add(p->model, PlaceMatrix(c.q, c.r, 0), tint);
        }
        // Vật thể
        if (c.item != NONE && c.item != DEN) {
            if (Proto* p = GetProto(Def(c.item).file)) {
                const ItemDef& d = Def(c.item);
                Vector3 ctr = HexCenter(c.q, c.r);
                float yaw = (d.cat == CAT_BUILDING || c.item == STUDIO) ? BuildingYaw(c.rot) : c.rot * 60.0f * DEG2RAD;
                float sc = WS * ((d.cat == CAT_PROP) ? 1.6f : 1.0f);
                Matrix m = MatrixMultiply(MatrixMultiply(MatrixScale(sc, sc, sc), MatrixRotateY(yaw)), MatrixTranslate(ctr.x, 0, ctr.z));
                mb.Add(p->model, m);
            }
        }
    }
    Chunk& ch = W.chunks[ci];
    if (ch.has) UnloadMesh(ch.mesh);
    ch.has = !mb.v.empty();
    if (ch.has) ch.mesh = mb.Build();
    ch.dirty = false;
}

void MarkDirty(int q, int r) {
    for (int e = -1; e < 6; ++e) {           // ô + 6 láng giềng (vì đường tự nối)
        Cell* c = CellAt(e < 0 ? q : q + DQ[e], e < 0 ? r : r + DR[e]);
        if (c) W.chunks[c->chunk].dirty = true;
    }
    W.minimapDirty = true;
}

// -----------------------------------------------------------------------------
//  Lưu / tải thế giới
// -----------------------------------------------------------------------------
void SaveWorld() {
    std::ofstream out(SAVE_FILE);
    out << "codevodai_world 1\n";
    out << "coins " << W.coins << "\n";
    out << "quests";
    for (auto& qd : W.quests) out << " " << (qd.done ? 1 : 0);
    out << "\n";
    for (const Cell& c : W.cells)
        if (c.tile != c.baseTile || c.item != NONE)
            out << "c " << c.q << " " << c.r << " " << c.tile << " " << c.item << " " << c.rot << "\n";
}

void LoadWorld() {
    std::ifstream in(SAVE_FILE);
    if (!in) return;
    std::string tag;
    in >> tag;
    if (tag != "codevodai_world") return;
    int ver;
    in >> ver;
    while (in >> tag) {
        if (tag == "coins") in >> W.coins;
        else if (tag == "quests") { for (auto& qd : W.quests) { int d; in >> d; qd.done = d != 0; } }
        else if (tag == "c") {
            int q, r, tile, item, rot;
            in >> q >> r >> tile >> item >> rot;
            if (Cell* c = CellAt(q, r)) {
                if (c->item == STUDIO) continue;
                c->tile = (tile == CO || tile == NUOC || tile == DUONG) ? tile : CO;
                c->item = (item >= 0 && item < ITEM_COUNT && item != STUDIO) ? item : NONE;
                c->rot = ((rot % 6) + 6) % 6;
            }
        }
    }
}

// -----------------------------------------------------------------------------
//  Nhiệm vụ gợi ý
// -----------------------------------------------------------------------------
enum QuestId { Q_HOUSE, Q_TREES, Q_ROAD, Q_CODE5, Q_ARENA, Q_LIBRARY, Q_LAMPS, Q_GRID9, Q_COUNT };

int CountItems(std::initializer_list<int> ids) {
    int n = 0;
    for (const Cell& c : W.cells)
        for (int id : ids) if (c.item == id) n++;
    return n;
}

void CompleteQuest(int id) {
    if (W.quests[id].done) return;
    W.quests[id].done = true;
    W.coins += 20;
    sfx::Play(sfx::UNLOCK, 0);
    world::Say("Nhiệm vụ hoàn thành!", std::string(W.quests[id].text) + "  (+20 xu)", 5);
    SaveWorld();
}

void CheckQuests() {
    if (CountItems({ NHA, NHA_DO, NHA_XANH, NHA_VANG }) >= 1) CompleteQuest(Q_HOUSE);
    if (CountItems({ CAY, CAY_THONG }) >= 10) CompleteQuest(Q_TREES);
    int roads = 0;
    for (const Cell& c : W.cells) roads += c.tile == DUONG;
    if (roads >= 6) CompleteQuest(Q_ROAD);
    if (W.lastRunPlaced >= 5) CompleteQuest(Q_CODE5);
    if (CountItems({ DAU_TRUONG }) >= 1) CompleteQuest(Q_ARENA);
    if (CountItems({ THU_VIEN }) >= 1) CompleteQuest(Q_LIBRARY);
    if (CountItems({ DEN }) >= 5) CompleteQuest(Q_LAMPS);
    if (W.lastRunHouses >= 9) CompleteQuest(Q_GRID9);
}

// -----------------------------------------------------------------------------
//  Đặt / xóa (dùng chung cho chuột và code)
// -----------------------------------------------------------------------------
std::string PlaceError(int q, int r, int item) {
    Cell* c = CellAt(q, r);
    if (!c) return "ô (" + std::to_string(q) + ", " + std::to_string(r) + ") nằm ngoài đảo";
    if (c->item == STUDIO) return "không thể xây đè lên Studio của Minh";
    if (item < 0 || item >= ITEM_COUNT || item == STUDIO) return "vật thể không hợp lệ";
    const ItemDef& d = Def(item);
    if (g_progress.completed < d.unlock)
        return std::string(d.name) + " cần học xong Bài " + std::to_string(d.unlock) + " mới xây được";
    if (IsObject(item) && c->tile == NUOC) return std::string("không thể đặt ") + d.name + " trên mặt nước";
    if (IsObject(item) && c->tile == DUONG && item != DEN) return std::string("không thể đặt ") + d.name + " lên đường";
    // Không xây đè lên chỗ người chơi đang đứng
    Vector3 ctr = HexCenter(q, r);
    if ((d.blocks || item == NUOC) && Vector2Distance({ ctr.x, ctr.z }, { W.pos.x, W.pos.z }) < WS * 1.1f)
        return "bạn đang đứng ở ô này, hãy bước ra trước";
    return "";
}

bool Place(int q, int r, int item, int rot) {
    if (!PlaceError(q, r, item).empty()) return false;
    Cell* c = CellAt(q, r);
    if (item == CO || item == NUOC || item == DUONG) {
        c->tile = item;
        if (item != CO && c->item != DEN) c->item = NONE;
        if (item == NUOC) c->item = NONE;
    } else if (item == NONE) {
        c->item = NONE;
    } else {
        c->item = item;
        c->rot = ((rot % 6) + 6) % 6;
    }
    MarkDirty(q, r);
    return true;
}

void Remove(int q, int r) {
    Cell* c = CellAt(q, r);
    if (!c || c->item == STUDIO) return;
    if (c->item != NONE) c->item = NONE;
    else if (c->tile != CO) c->tile = CO;
    MarkDirty(q, r);
}

// Hướng mặc định cho nhà: quay cửa về phía có đường gần nhất
int AutoRot(int q, int r, int fallback) {
    for (int e = 0; e < 6; ++e) {
        Cell* nb = CellAt(q + DQ[e], r + DR[e]);
        if (nb && nb->tile == DUONG) return e;
    }
    return fallback;
}

// -----------------------------------------------------------------------------
//  Va chạm
// -----------------------------------------------------------------------------
bool Walkable(Vector3 p) {
    int q, r;
    PixelToHex(p.x, p.z, q, r);
    Cell* c = CellAt(q, r);
    if (!c || c->tile == NUOC) return false;
    for (int e = -1; e < 6; ++e) {                     // kiểm tra vật thể ở ô này và ô kề
        Cell* o = CellAt(e < 0 ? q : q + DQ[e], e < 0 ? r : r + DR[e]);
        if (!o || o->item == NONE) continue;
        const ItemDef& d = Def(o->item);
        Vector3 ctr = HexCenter(o->q, o->r);
        if (e < 0 && d.blocks) return false;
        if (d.radius > 0 && Vector2Distance({ ctr.x, ctr.z }, { p.x, p.z }) < d.radius + 0.4f) return false;
    }
    return true;
}

// Điểm cửa của một công trình (để tương tác)
Vector3 DoorOf(const Cell& c) {
    Vector3 ctr = HexCenter(c.q, c.r);
    float a = c.rot * 60.0f * DEG2RAD;
    float dist = (c.item == DAU_TRUONG) ? WS * 1.05f : WS * 0.75f;
    return { ctr.x + std::cos(a) * dist, 0, ctr.z + std::sin(a) * dist };
}

// -----------------------------------------------------------------------------
//  Sinh đảo ban đầu (chỉ có đất, nước và Studio — phần còn lại do bạn xây!)
// -----------------------------------------------------------------------------
void Generate() {
    W.cells.clear();
    W.index.clear();
    W.chunkW = (2 * RAD) / CHUNK + 1;
    for (int q = -RAD; q <= RAD; ++q)
        for (int r = -RAD; r <= RAD; ++r) {
            if (HexDist(q, r) > RAD) continue;
            Cell c;
            c.q = q;
            c.r = r;
            Vector3 p = HexCenter(q, r);
            float d = HexDist(q, r) / (float)RAD;
            float n = Fbm(p.x * 0.012f + 3.7f, p.z * 0.012f + 1.3f);
            bool land = d + (n - 0.5f) * 0.45f < 0.82f;
            if (HexDist(q, r) <= 3) land = true;                 // khu trung tâm luôn là đất
            c.tile = c.baseTile = land ? CO : NUOC;
            c.chunk = ((q + RAD) / CHUNK) * W.chunkW + (r + RAD) / CHUNK;
            W.index[Key(q, r)] = (int)W.cells.size();
            W.cells.push_back(c);
        }
    W.chunks.assign(W.chunkW * W.chunkW, Chunk{});
    if (Cell* s = CellAt(0, 0)) { s->item = STUDIO; s->rot = 1; }   // cửa Studio hướng về ô (0, 1)
}

// -----------------------------------------------------------------------------
//  API xây dựng cho code người chơi (Bản thiết kế)
// -----------------------------------------------------------------------------
// Người chơi TỰ TAY viết toàn bộ code: game chỉ đưa khung trống + thẻ KIẾN THỨC
// (giải thích cú pháp bằng ví dụ không liên quan tới việc xây — không có lời giải sẵn).
const char* STARTER =
    "// BẢN THIẾT KẾ của bạn. Game sẽ gọi hàm XayDung() khi bạn bấm F5.\n"
    "// Tự viết code để xây đảo — đọc thẻ Kiến thức và danh sách lệnh bên phải.\n"
    "\n"
    "void XayDung() {\n"
    "    \n"
    "}\n";

struct KnowCard { const char* title; const char* text; const char* code; };
const KnowCard KNOW[] = {
    { "1. Gọi hàm",
      "Một lệnh gọi hàm gồm TÊN hàm và các THAM SỐ trong ngoặc tròn, kết thúc bằng dấu chấm phẩy. "
      "Lệnh Dat cần 3 tham số: q, r và vật thể. Tọa độ (q, r) của ô hiện ở góc chuột khi bấm B.",
      "std::cout << \"xin chao\";   // gọi 'hàm' in ra màn hình\nint x = abs(-5);           // hàm có giá trị trả về" },
    { "2. Biến",
      "Biến là chiếc hộp có tên để giữ một giá trị. int giữ số nguyên. Đổi giá trị biến một chỗ thì mọi nơi dùng nó đều đổi theo.",
      "int mau = 100;\nint satThuong = 15;\nmau = mau - satThuong;   // mau bây giờ là 85" },
    { "3. Vòng lặp for",
      "for lặp lại một khối lệnh nhiều lần. Gồm 3 phần: khởi tạo; điều kiện; bước nhảy. "
      "Biến đếm i có thể dùng ngay trong thân vòng lặp.",
      "for (int i = 0; i < 5; i++) {\n    std::cout << i << \" \";   // in 0 1 2 3 4\n}" },
    { "4. if / else và %",
      "if chỉ chạy khối lệnh khi điều kiện đúng. Toán tử % lấy phần dư: i % 2 == 0 nghĩa là i chẵn. "
      "&& là VÀ, || là HOẶC, ! là PHỦ ĐỊNH.",
      "if (diem >= 8 && !gianLan) {\n    std::cout << \"gioi\";\n} else {\n    std::cout << \"co gang them\";\n}" },
    { "5. Viết hàm của riêng bạn",
      "Khi một nhóm lệnh lặp lại ở nhiều nơi, hãy gom nó thành hàm có tham số. "
      "Hàm phải được viết TRƯỚC chỗ gọi nó. void nghĩa là hàm không trả về gì.",
      "int BinhPhuong(int x) {\n    return x * x;\n}\n\nvoid InBang(int n) {\n    for (int i = 1; i <= n; i++) std::cout << BinhPhuong(i) << \" \";\n}" },
    { "6. Vòng lặp lồng nhau",
      "Đặt một vòng for bên trong một vòng for khác để duyệt cả HÀNG lẫn CỘT, như duyệt bảng hay lưới ô. "
      "Vòng trong chạy hết mỗi lần vòng ngoài bước một bước.",
      "for (int hang = 0; hang < 3; hang++) {\n    for (int cot = 0; cot < 4; cot++) {\n        std::cout << \"*\";\n    }\n    std::cout << \"\\n\";\n}" },
    { "7. continue, break và ngẫu nhiên",
      "continue bỏ qua phần còn lại của lần lặp hiện tại; break thoát hẳn vòng lặp. "
      "Kết hợp với NgauNhien(a, b) để mỗi lần chạy cho kết quả khác nhau.",
      "for (int i = 0; i < 100; i++) {\n    if (i % 3 == 0) continue;   // bỏ số chia hết cho 3\n    if (i > 20) break;          // dừng khi quá 20\n    std::cout << i << \" \";\n}" },
    { "8. Lưới lục giác",
      "Mỗi ô có 6 ô kề, đánh số hướng 0..5. Ô kề theo hướng h là (q + LangGiengQ(h), r + LangGiengR(h)). "
      "KhoangCach cho biết cách nhau bao nhiêu bước ô — các ô có cùng KhoangCach tới một tâm tạo thành một vòng tròn lục giác.",
      "// Đếm số ô kề là nước quanh ô (q, r):\nint dem = 0;\nfor (int h = 0; h < 6; h++)\n    if (LaNuoc(q + LangGiengQ(h), r + LangGiengR(h))) dem++;" },
};
const int KNOW_COUNT = sizeof(KNOW) / sizeof(KNOW[0]);

void RunBlueprint() {
    std::string src = W.editor.Text();
    {
        std::ofstream out("code/ban_thiet_ke.cpp");
        if (!out) { MakeDirectory("code"); std::ofstream o2("code/ban_thiet_ke.cpp"); o2 << src; }
        else out << src;
    }
    mcpp::Program p;
    p.SetStepLimit(3000000);
    for (int i = 0; i < ITEM_COUNT; ++i) if (i != STUDIO) p.DefineConstant(ITEMS[i].code, i);
    std::vector<Pending> ops;
    std::set<long long> taken;
    auto argInt = [](std::vector<mcpp::Value>& a, size_t k, int line, const char* fn) {
        if (k >= a.size() || !a[k].IsNumber()) throw mcpp::Error{ line, std::string(fn) + " thiếu tham số số nguyên" };
        return (int)a[k].AsInt();
    };
    // Trạng thái "giả lập" để các hàm hỏi (OTrong, LaDat...) thấy cả những gì vừa đặt trong lần chạy này
    std::map<long long, std::pair<int, int>> sim;    // key -> (tile, item)
    auto look = [&](int q, int r, int& tile, int& item) {
        Cell* c = CellAt(q, r);
        if (!c) { tile = -1; item = -1; return; }
        tile = c->tile;
        item = c->item;
        auto it = sim.find(Key(q, r));
        if (it != sim.end()) { tile = it->second.first; item = it->second.second; }
    };
    p.RegisterFunction("Dat", [&](std::vector<mcpp::Value>& a, int line) {
        if (a.size() < 3 || a.size() > 4) throw mcpp::Error{ line, "Dat(q, r, vatThe) hoặc Dat(q, r, vatThe, huong)" };
        int q = argInt(a, 0, line, "Dat"), r = argInt(a, 1, line, "Dat"), item = argInt(a, 2, line, "Dat");
        int rot = a.size() == 4 ? argInt(a, 3, line, "Dat") : -1;
        if (ops.size() >= 3000) throw mcpp::Error{ line, "quá nhiều lệnh Dat (tối đa 3000 mỗi lần chạy)" };
        std::string err = PlaceError(q, r, item);
        if (!err.empty()) { p.Output() += "Dòng " + std::to_string(line) + ": bỏ qua — " + err + "\n"; return mcpp::Value::Bool(false); }
        int tile, cur;
        look(q, r, tile, cur);
        if (IsObject(item) && tile == NUOC) { p.Output() += "Dòng " + std::to_string(line) + ": bỏ qua — ô nước\n"; return mcpp::Value::Bool(false); }
        if (item == CO || item == NUOC || item == DUONG) sim[Key(q, r)] = { item, item == NUOC ? NONE : cur };
        else sim[Key(q, r)] = { tile, item };
        ops.push_back({ q, r, item, rot });
        return mcpp::Value::Bool(true);
    });
    p.RegisterFunction("Xoa", [&](std::vector<mcpp::Value>& a, int line) {
        int q = argInt(a, 0, line, "Xoa"), r = argInt(a, 1, line, "Xoa");
        ops.push_back({ q, r, -1, 0 });
        int tile, cur;
        look(q, r, tile, cur);
        sim[Key(q, r)] = { cur != NONE ? tile : CO, NONE };
        return mcpp::Value();
    });
    p.RegisterFunction("OTrong", [&](std::vector<mcpp::Value>& a, int line) {
        int tile, item;
        look(argInt(a, 0, line, "OTrong"), argInt(a, 1, line, "OTrong"), tile, item);
        return mcpp::Value::Bool(tile >= 0 && item == NONE);
    });
    p.RegisterFunction("LaDat", [&](std::vector<mcpp::Value>& a, int line) {
        int tile, item;
        look(argInt(a, 0, line, "LaDat"), argInt(a, 1, line, "LaDat"), tile, item);
        return mcpp::Value::Bool(tile == CO || tile == DUONG);
    });
    p.RegisterFunction("LaNuoc", [&](std::vector<mcpp::Value>& a, int line) {
        int tile, item;
        look(argInt(a, 0, line, "LaNuoc"), argInt(a, 1, line, "LaNuoc"), tile, item);
        return mcpp::Value::Bool(tile == NUOC);
    });
    p.RegisterFunction("VatThe", [&](std::vector<mcpp::Value>& a, int line) {
        int tile, item;
        look(argInt(a, 0, line, "VatThe"), argInt(a, 1, line, "VatThe"), tile, item);
        return mcpp::Value::Int(item < 0 ? NONE : item);
    });
    p.RegisterFunction("TrongDao", [&](std::vector<mcpp::Value>& a, int line) {
        return mcpp::Value::Bool(CellAt(argInt(a, 0, line, "TrongDao"), argInt(a, 1, line, "TrongDao")) != nullptr);
    });
    p.RegisterFunction("KhoangCach", [&](std::vector<mcpp::Value>& a, int line) {
        int q1 = argInt(a, 0, line, "KhoangCach"), r1 = argInt(a, 1, line, "KhoangCach");
        int q2 = argInt(a, 2, line, "KhoangCach"), r2 = argInt(a, 3, line, "KhoangCach");
        return mcpp::Value::Int(HexDist(q2 - q1, r2 - r1));
    });
    p.RegisterFunction("LangGiengQ", [&](std::vector<mcpp::Value>& a, int line) {
        int h = argInt(a, 0, line, "LangGiengQ");
        return mcpp::Value::Int(DQ[((h % 6) + 6) % 6]);
    });
    p.RegisterFunction("LangGiengR", [&](std::vector<mcpp::Value>& a, int line) {
        int h = argInt(a, 0, line, "LangGiengR");
        return mcpp::Value::Int(DR[((h % 6) + 6) % 6]);
    });
    p.RegisterFunction("NgauNhien", [&](std::vector<mcpp::Value>& a, int line) {
        int lo = argInt(a, 0, line, "NgauNhien"), hi = argInt(a, 1, line, "NgauNhien");
        if (hi < lo) std::swap(lo, hi);
        return mcpp::Value::Int(GetRandomValue(lo, hi));
    });
    p.RegisterFunction("ViTriQ", [&](std::vector<mcpp::Value>&, int) {     // ô bạn đang đứng
        int q, r; PixelToHex(W.pos.x, W.pos.z, q, r); return mcpp::Value::Int(q);
    });
    p.RegisterFunction("ViTriR", [&](std::vector<mcpp::Value>&, int) {
        int q, r; PixelToHex(W.pos.x, W.pos.z, q, r); return mcpp::Value::Int(r);
    });

    W.output.clear();
    if (!p.Compile(src, W.errors)) {
        W.editor.SetErrorLine(W.errors.empty() ? 0 : W.errors[0].line);
        sfx::Play(sfx::WRONG, 0);
        return;
    }
    if (!p.HasFunction("XayDung")) {
        W.errors = { mcpp::Error{ 1, "không tìm thấy hàm void XayDung() — game sẽ gọi hàm này" } };
        sfx::Play(sfx::WRONG, 0);
        return;
    }
    try {
        p.Call("XayDung", {});
    } catch (const mcpp::Error& e) {
        W.errors = { e };
        W.editor.SetErrorLine(e.line);
        W.output = p.Output();
        sfx::Play(sfx::WRONG, 0);
        return;
    }
    W.errors.clear();
    W.editor.SetErrorLine(0);
    W.history.push_back(W.cells);
    if (W.history.size() > 10) W.history.erase(W.history.begin());
    W.pending = ops;
    W.pendingTimer = 0;
    int houses = 0;
    for (auto& o : ops) if (o.item >= NHA && o.item <= NHA_VANG) houses++;
    W.lastRunPlaced = (int)ops.size();
    W.lastRunHouses = houses;
    W.output = p.Output() + "Xây " + std::to_string(ops.size()) + " vật thể. Hãy xem chúng mọc lên!\n";
    sfx::Play(sfx::CORRECT, 0);
    if (!ops.empty()) W.blueprint = false;      // đóng cửa sổ để xem công trình hiện ra
}

// -----------------------------------------------------------------------------
//  Dân làng: mỗi ngôi nhà có một người đến ở
// -----------------------------------------------------------------------------
const char* NPC_NAMES[] = { "Bác Tùng", "Cô Lan", "Anh Khoa", "Chị Mai", "Ông Bảy", "Bé Na", "Chú Hùng", "Cô Hà" };

void SyncNpcs() {
    std::vector<Vector3> homes;
    for (const Cell& c : W.cells)
        if (c.item != NONE && Def(c.item).house) homes.push_back(DoorOf(c));
    size_t want = std::min<size_t>(homes.size(), 8);
    while (W.npcs.size() > want) W.npcs.pop_back();
    while (W.npcs.size() < want) {
        Npc n;
        n.ch = std::make_unique<AnimChar>();
        CharModel kinds[4] = { CharModel::Barbarian, CharModel::Knight, CharModel::Mage, CharModel::RogueHooded };
        n.ch->Load(kinds[W.npcs.size() % 4]);
        n.name = NPC_NAMES[W.npcs.size() % 8];
        n.home = homes[W.npcs.size()];
        n.pos = n.target = n.home;
        n.wait = GetRandomValue(0, 30) / 10.0f;
        W.npcs.push_back(std::move(n));
    }
    for (size_t i = 0; i < W.npcs.size(); ++i) W.npcs[i].home = homes[i];
}

// -----------------------------------------------------------------------------
//  Ngày & đêm
// -----------------------------------------------------------------------------
float Night() {                                  // 0 = ban ngày, 1 = đêm
    float h = W.hour;
    if (h >= 7 && h <= 17) return 0;
    if (h >= 19.5f || h <= 4.5f) return 1;
    if (h > 17) return (h - 17) / 2.5f;
    return 1 - (h - 4.5f) / 2.5f;
}

Color SkyTop() {
    float n = Night();
    float dusk = std::max(0.0f, 1.0f - std::fabs(W.hour - 18.0f) / 1.5f) + std::max(0.0f, 1.0f - std::fabs(W.hour - 6.0f) / 1.5f);
    Color c = Lerp(Color{ 84, 150, 230, 255 }, Color{ 8, 12, 34, 255 }, n);
    return Lerp(c, Color{ 70, 60, 130, 255 }, Clamp01(dusk) * 0.6f);
}
Color SkyBottom() {
    float n = Night();
    float dusk = std::max(0.0f, 1.0f - std::fabs(W.hour - 18.0f) / 1.5f) + std::max(0.0f, 1.0f - std::fabs(W.hour - 6.0f) / 1.5f);
    Color c = Lerp(Color{ 196, 224, 250, 255 }, Color{ 24, 30, 64, 255 }, n);
    return Lerp(c, Color{ 250, 160, 100, 255 }, Clamp01(dusk) * 0.8f);
}

Vector3 SunDirection() {                         // hướng tới mặt trời (hoặc mặt trăng ban đêm)
    float a = (W.hour - 6.0f) / 12.0f * PI;
    Vector3 s = { std::cos(a), std::sin(a), 0.45f };
    if (s.y < 0.15f) {                               // ban đêm: dùng mặt trăng
        float m = (W.hour - 18.0f) / 12.0f * PI;
        if (W.hour < 6) m = (W.hour + 6.0f) / 12.0f * PI;
        s = { -std::cos(m) * 0.6f, std::max(0.35f, std::sin(m)), 0.5f };
    }
    return Vector3Normalize(s);
}

void SetupEnvironment() {
    gfx::Environment e;
    float n = Night();
    Vector3 toSun = SunDirection();
    e.sunDir = Vector3Negate(toSun);
    e.sunColor = Lerp(Color{ 255, 240, 214, 255 }, Color{ 140, 160, 255, 255 }, n);
    float dusk = std::max(0.0f, 1.0f - std::fabs(W.hour - 18.0f) / 1.5f) + std::max(0.0f, 1.0f - std::fabs(W.hour - 6.0f) / 1.5f);
    e.sunColor = Lerp(e.sunColor, Color{ 255, 160, 90, 255 }, Clamp01(dusk) * 0.7f);
    e.sunIntensity = 0.95f - n * 0.65f;
    e.sky = Lerp(Color{ 160, 190, 230, 255 }, Color{ 50, 60, 120, 255 }, n);
    e.ground = Lerp(Color{ 110, 120, 70, 255 }, Color{ 20, 20, 40, 255 }, n);
    e.ambient = 0.5f - n * 0.2f;
    e.exposure = 0.82f;
    e.fog = SkyBottom();
    e.fogNear = 110;
    e.fogFar = 330;
    e.shadowCenter = { W.pos.x, 0, W.pos.z };
    e.shadowSize = 70;
    e.bloom = 0.35f + n * 0.5f;
    e.bloomThreshold = 0.8f - n * 0.15f;
    e.vignette = 0.3f;
    e.saturation = 0.95f;
    // Đèn đường (chỉ sáng khi trời tối): chọn những cây đèn gần người chơi nhất
    if (n > 0.05f) {
        std::vector<std::pair<float, Vector3>> lamps;
        for (const Cell& c : W.cells)
            if (c.item == DEN) {
                Vector3 p = HexCenter(c.q, c.r);
                p.y = 4.6f;
                lamps.push_back({ Vector3Distance(p, W.pos), p });
            }
        std::sort(lamps.begin(), lamps.end(), [](auto& a, auto& b) { return a.first < b.first; });
        for (size_t i = 0; i < lamps.size() && e.lightCount < 7; ++i)
            e.lights[e.lightCount++] = { lamps[i].second, Color{ 255, 190, 110, 255 }, 1.6f * n, 18 };
    }
    gfx::SetEnvironment(e);
}

// -----------------------------------------------------------------------------
//  Vẽ
// -----------------------------------------------------------------------------
void DrawLamps(bool glow) {
    float n = Night();
    for (const Cell& c : W.cells) {
        if (c.item != DEN) continue;
        Vector3 p = HexCenter(c.q, c.r);
        if (!glow) {
            gfx::Cylinder(p, { p.x, 4.2f, p.z }, 0.12f, 0.1f, Color{ 50, 46, 44, 255 }, 8);
            gfx::Box({ p.x, 4.5f, p.z }, { 0.55f, 0.6f, 0.55f }, Color{ 60, 54, 50, 255 });
            gfx::Box({ p.x, 4.85f, p.z }, { 0.7f, 0.12f, 0.7f }, Color{ 40, 36, 34, 255 });
        } else {
            Color lc = Lerp(Color{ 120, 110, 90, 255 }, Color{ 255, 210, 140, 255 }, n);
            DrawCube({ p.x, 4.5f, p.z }, 0.42f, 0.45f, 0.42f, lc);
        }
    }
}

void DrawWorldGeometry() {
    for (Chunk& ch : W.chunks)
        if (ch.has) gfx::DrawMeshLit(ch.mesh, W.mat, MatrixIdentity());
    DrawLamps(false);
}

void DrawOcean() {
    float y = -0.2f * WS - 0.05f;
    float R = 1200.0f;
    rlBegin(RL_QUADS);
    Color c = Lerp(Color{ 70, 150, 215, 255 }, Color{ 20, 40, 80, 255 }, Night());
    rlColor4ub(c.r, c.g, c.b, 255);
    rlNormal3f(0, 1, 0);
    rlVertex3f(-R, y, -R);
    rlVertex3f(-R, y, R);
    rlVertex3f(R, y, R);
    rlVertex3f(R, y, -R);
    rlEnd();
}

Vector3 PlayerSpawn() {
    if (Cell* s = CellAt(0, 0)) {
        Vector3 d = DoorOf(*s);
        Vector3 ctr = HexCenter(0, 0);
        Vector3 dir = Vector3Normalize(Vector3Subtract(d, ctr));
        return Vector3Add(d, Vector3Scale(dir, 3.0f));
    }
    return { 0, 0, 10 };
}

void BuildMinimap() {
    const int S = 256;
    if (W.minimap.id == 0) W.minimap = LoadRenderTexture(S, S);
    BeginTextureMode(W.minimap);
    ClearBackground(Color{ 0, 0, 0, 0 });
    float extent = WS * 2.0f * (RAD + 1);
    float k = S / (2 * extent);
    for (const Cell& c : W.cells) {
        Vector3 p = HexCenter(c.q, c.r);
        Vector2 m = { S / 2 + p.x * k, S / 2 + p.z * k };
        Color col = c.tile == NUOC ? Color{ 60, 130, 210, 255 } : (c.tile == DUONG ? Color{ 225, 190, 130, 255 } : Color{ 120, 180, 80, 255 });
        if (c.item != NONE) {
            const ItemDef& d = Def(c.item);
            if (d.cat == CAT_BUILDING || c.item == STUDIO) col = Color{ 205, 90, 70, 255 };
            else if (c.item == NUI || c.item == DOI) col = Color{ 140, 140, 130, 255 };
            else if (d.cat == CAT_NATURE) col = Color{ 60, 120, 60, 255 };
        }
        DrawPoly(m, 6, WS * 1.12f * k, 30, col);
    }
    EndTextureMode();
    W.minimapDirty = false;
}

} // namespace

// =============================================================================
//  API
// =============================================================================
bool world::Ready() { return W.ready; }

bool world::Init() {
    if (!FileExists("assets/models/world/hexagons_medieval.png")) return false;
    {
        // Cỏ trong atlas gốc khá vàng chanh: xử lý ảnh trên CPU, kéo các điểm ảnh
        // "vàng-xanh" về xanh lá hơn (xử lý ảnh từng pixel — một kỹ thuật hay dùng khi làm tool)
        Image img = LoadImage("assets/models/world/hexagons_medieval.png");
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        Color* px = (Color*)img.data;
        for (int i = 0; i < img.width * img.height; ++i) {
            Color& c = px[i];
            if (c.g > 60 && c.r > c.g * 0.7f && c.r <= c.g * 1.05f && c.b < c.g * 0.6f) {
                c.r = (unsigned char)(c.r * 0.7f);
                c.b = (unsigned char)std::min(255.0f, c.b + c.g * 0.12f);
            }
        }
        W.atlas = LoadTextureFromImage(img);
        UnloadImage(img);
        GenTextureMipmaps(&W.atlas);
        SetTextureFilter(W.atlas, TEXTURE_FILTER_BILINEAR);
    }
    W.mat = LoadMaterialDefault();
    W.mat.maps[MATERIAL_MAP_DIFFUSE].texture = W.atlas;
    W.quests = {
        { "Đặt ngôi nhà đầu tiên", false },
        { "Trồng 10 cây", false },
        { "Làm đường dài 6 ô", false },
        { "Dùng CODE xây 5 vật thể trong một lần chạy", false },
        { "Xây Đấu trường", false },
        { "Xây Thư viện", false },
        { "Thắp sáng 5 cây đèn đường", false },
        { "Dùng 2 vòng for lồng nhau xây 9 ngôi nhà", false },
    };
    Generate();
    LoadWorld();
    for (size_t i = 0; i < W.chunks.size(); ++i) RebuildChunk((int)i);
    W.player.Load(CharModel::Rogue);
    W.pos = PlayerSpawn();
    W.camPos = Vector3Add(W.pos, { 0, 6, 10 });
    W.camTarget = W.pos;
    SyncNpcs();
    W.ready = true;
    return true;
}

void world::Shutdown() {
    for (Chunk& ch : W.chunks) if (ch.has) UnloadMesh(ch.mesh);
    for (auto& kv : W.protos) if (kv.second.ok) UnloadModel(kv.second.model);
    if (W.minimap.id) UnloadRenderTexture(W.minimap);
    if (W.atlas.id) UnloadTexture(W.atlas);
    W.npcs.clear();
}

void world::Say(const std::string& who, const std::string& msg, float seconds) {
    W.sayWho = who;
    W.sayText = msg;
    W.sayTime = seconds;
}

void world::SpawnAtStudio() {
    W.pos = PlayerSpawn();
    W.vel = { 0, 0, 0 };
    if (Cell* s = CellAt(0, 0)) {
        Vector3 d = Vector3Subtract(DoorOf(*s), HexCenter(0, 0));
        W.yaw = std::atan2(d.x, d.z) * RAD2DEG;
        W.camYaw = std::atan2(d.x, d.z);
    }
}

void world::SetHour(float h) { W.hour = std::fmod(std::fabs(h), 24.0f); }

bool world::WantsTextInput() { return W.ready && W.blueprint && W.editor.Focused(); }

void world::SpawnAtArena() {
    for (const Cell& c : W.cells)
        if (c.item == DAU_TRUONG) {
            Vector3 d = DoorOf(c), ctr = HexCenter(c.q, c.r);
            W.pos = Vector3Add(d, Vector3Scale(Vector3Normalize(Vector3Subtract(d, ctr)), 3.0f));
            return;
        }
    SpawnAtStudio();
}

// -----------------------------------------------------------------------------
//  UPDATE
// -----------------------------------------------------------------------------
static bool Ctrl() { return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL); }

static void UpdateCamera(float dt, bool titleMode) {
    if (titleMode) {
        float a = W.t * 0.05f;
        W.cam.position = { std::sin(a) * 120.0f, 60.0f, std::cos(a) * 120.0f };
        W.cam.target = { 0, 0, 0 };
    } else {
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !W.build) {
            Vector2 d = GetMouseDelta();
            W.camYaw -= d.x * 0.005f;
            W.camPitch = Clamp(W.camPitch + d.y * 0.004f, 0.12f, 1.35f);
        }
        if (IsKeyDown(KEY_Q)) W.camYaw += 1.6f * dt;
        if (IsKeyDown(KEY_E) && W.build) W.camYaw -= 1.6f * dt;
        float wheel = GetMouseWheelMove();
        float minD = W.build ? 18.0f : 5.0f, maxD = W.build ? 90.0f : 28.0f;
        W.camDist = Clamp(W.camDist - wheel * (W.build ? 5.0f : 1.5f), minD, maxD);
        if (W.build) W.camPitch = std::max(W.camPitch, 0.75f);
        Vector3 target = { W.pos.x, W.pos.y + 1.6f, W.pos.z };
        Vector3 off = { std::sin(W.camYaw) * std::cos(W.camPitch), std::sin(W.camPitch), std::cos(W.camYaw) * std::cos(W.camPitch) };
        Vector3 want = Vector3Add(target, Vector3Scale(off, W.camDist));
        if (want.y < 1.0f) want.y = 1.0f;
        float k = std::min(1.0f, 10.0f * dt);
        W.camPos = Vector3Lerp(W.camPos, want, k);
        W.camTarget = Vector3Lerp(W.camTarget, target, k);
        W.cam.position = W.camPos;
        W.cam.target = W.camTarget;
    }
    W.cam.up = { 0, 1, 0 };
    W.cam.fovy = 50;
    W.cam.projection = CAMERA_PERSPECTIVE;
}

static void UpdatePlayer(float dt) {
    Vector3 in = { 0, 0, 0 };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) in.z -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) in.z += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) in.x -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) in.x += 1;
    W.running = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    bool moving = Vector3Length(in) > 0;
    if (moving) {
        in = Vector3Normalize(in);
        // Đổi hướng phím theo góc camera (toán vector, Bài 9)
        float cy = std::cos(W.camYaw), sy = std::sin(W.camYaw);
        Vector3 dir = { in.x * cy + in.z * sy, 0, -in.x * sy + in.z * cy };
        float speed = (W.running ? 10.0f : 5.0f) * (W.build ? 2.0f : 1.0f);
        Vector3 next = { W.pos.x + dir.x * speed * dt, W.pos.y, W.pos.z };
        if (Walkable(next)) W.pos.x = next.x;
        next = { W.pos.x, W.pos.y, W.pos.z + dir.z * speed * dt };
        if (Walkable(next)) W.pos.z = next.z;
        float want = std::atan2(dir.x, dir.z) * RAD2DEG;
        float diff = std::fmod(want - W.yaw + 540.0f, 360.0f) - 180.0f;
        W.yaw += diff * std::min(1.0f, 12.0f * dt);
    }
    if (IsKeyPressed(KEY_SPACE) && W.onGround && !W.build) { W.vel.y = 8.5f; W.onGround = false; sfx::Play(sfx::JUMP, 0.05f, 0.5f); }
    if (!W.onGround) {
        W.vel.y -= 25.0f * dt;
        W.pos.y += W.vel.y * dt;
        if (W.pos.y <= 0) { W.pos.y = 0; W.vel.y = 0; W.onGround = true; }
    }
    if (!W.onGround) W.player.Play("Jump_Idle");
    else if (moving) W.player.Play(W.running ? "Running_A" : "Walking_A", true, W.running ? 1.0f : 1.3f);
    else W.player.Play("Idle");
    W.player.Update(dt);
}

static void UpdateNpcs(float dt) {
    for (size_t i = 0; i < W.npcs.size(); ++i) {
        Npc& n = W.npcs[i];
        bool talking = W.talkNpc == (int)i;
        float dp = Vector3Distance(n.pos, W.pos);
        if (talking || dp < 3.0f) {                                   // đứng lại, nhìn người chơi
            Vector3 d = Vector3Subtract(W.pos, n.pos);
            n.yaw = std::atan2(d.x, d.z) * RAD2DEG;
            n.ch->Play(talking ? "Interact" : "Idle", true);
            n.walking = false;
        } else if (n.walking) {
            Vector3 d = Vector3Subtract(n.target, n.pos);
            d.y = 0;
            float len = Vector3Length(d);
            if (len < 0.5f) { n.walking = false; n.wait = 2.0f + GetRandomValue(0, 40) / 10.0f; }
            else {
                Vector3 step = Vector3Scale(d, 1.8f * dt / len);
                Vector3 next = Vector3Add(n.pos, step);
                if (Walkable(next)) n.pos = next;
                else { n.walking = false; n.wait = 1.0f; }
                n.yaw = std::atan2(d.x, d.z) * RAD2DEG;
                n.ch->Play("Walking_A", true, 1.0f);
            }
        } else {
            n.ch->Play("Idle");
            n.wait -= dt;
            if (n.wait <= 0) {                                        // đi dạo quanh nhà
                float a = GetRandomValue(0, 628) / 100.0f, rr = 3.0f + GetRandomValue(0, 120) / 10.0f;
                n.target = { n.home.x + std::cos(a) * rr, 0, n.home.z + std::sin(a) * rr };
                n.walking = Walkable(n.target);
                n.wait = 1.0f;
            }
        }
        n.ch->Update(dt);
    }
}

static void StartTalk(int i) {
    W.talkNpc = i;
    W.quizPicked = -1;
    W.quizLesson = -1;
    int maxL = std::max(1, std::min(g_progress.completed, (int)GetLessons().size()));
    if (GetRandomValue(0, 2) > 0) {                                  // đố vui về bài đã học
        W.quizLesson = GetRandomValue(0, maxL - 1);
        const Lesson& L = GetLessons()[W.quizLesson];
        W.quizQ = GetRandomValue(0, (int)L.quiz.size() - 1);
        W.quizOrder.clear();
        for (int k = 0; k < (int)L.quiz[W.quizQ].options.size(); ++k) W.quizOrder.push_back(k);
        for (int k = (int)W.quizOrder.size() - 1; k > 0; --k) std::swap(W.quizOrder[k], W.quizOrder[GetRandomValue(0, k)]);
    }
}

world::Action world::Update(float dt, bool titleMode) {
    if (!W.ready) return Action::None;
    W.t += dt;
    W.sayTime -= dt;
    W.hour = std::fmod(W.hour + dt / 25.0f, 24.0f);          // 1 giờ trong game = 25 giây

    // Xây dần các vật thể từ lần chạy code
    if (!W.pending.empty()) {
        W.pendingTimer -= dt;
        while (W.pendingTimer <= 0 && !W.pending.empty()) {
            Pending o = W.pending.front();
            W.pending.erase(W.pending.begin());
            if (o.item < 0) Remove(o.q, o.r);
            else {
                int rot = o.rot >= 0 ? o.rot : (Def(o.item).cat == CAT_BUILDING ? AutoRot(o.q, o.r, 1) : GetRandomValue(0, 5));
                Place(o.q, o.r, o.item, rot);
            }
            W.pendingTimer += std::max(0.004f, 0.6f / (W.pending.size() + 6));
            if (W.pending.empty()) { SyncNpcs(); CheckQuests(); SaveWorld(); }
            sfx::Play(sfx::STEP, 0.2f, 0.5f);
        }
    }
    int rebuilt = 0;
    for (size_t i = 0; i < W.chunks.size() && rebuilt < 3; ++i)
        if (W.chunks[i].dirty) { RebuildChunk((int)i); rebuilt++; }
    if (W.minimapDirty) BuildMinimap();

    UpdateNpcs(dt);
    if (titleMode) { UpdateCamera(dt, true); W.player.Update(dt); return Action::None; }

    // ---- Cửa sổ bản thiết kế (code) ----
    if (W.blueprint) {
        if (!W.editorLoaded) {
            std::ifstream in("code/ban_thiet_ke.cpp");
            std::stringstream ss;
            if (in) { ss << in.rdbuf(); W.editor.SetText(ss.str()); }
            else W.editor.SetText(STARTER);
            W.editorLoaded = true;
            W.editor.Focus(true);
        }
        float s = ui::S();
        float Wd = (float)GetScreenWidth(), Hd = (float)GetScreenHeight();
        Rectangle ed = { 30 * s, 92 * s, Wd * 0.58f, Hd - 92 * s - 150 * s };
        if (IsKeyPressed(KEY_F5) || (Ctrl() && IsKeyPressed(KEY_ENTER))) RunBlueprint();
        else if (W.editor.Focused() && IsKeyPressed(KEY_ESCAPE)) W.editor.Focus(false);
        else if (!W.editor.Focused() && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_C))) W.blueprint = false;
        else W.editor.Update(ed);
        if (W.editor.TakeChanged()) W.editor.SetErrorLine(0);
        UpdateCamera(dt, false);
        return Action::None;
    }

    // ---- Hội thoại ----
    if (W.talkNpc >= 0) {
        if (W.quizLesson >= 0 && W.quizPicked < 0) {
            for (int k = 0; k < 4; ++k)
                if (IsKeyPressed(KEY_ONE + k) && k < (int)W.quizOrder.size()) {
                    W.quizPicked = W.quizOrder[k];
                    bool ok = W.quizPicked == GetLessons()[W.quizLesson].quiz[W.quizQ].correct;
                    sfx::Play(ok ? sfx::CORRECT : sfx::WRONG, 0);
                    if (ok) { W.coins += 5; SaveWorld(); }
                }
        } else if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
            W.talkNpc = -1;
        }
        if (IsKeyPressed(KEY_ESCAPE)) W.talkNpc = -1;
        UpdatePlayer(0);
        UpdateCamera(dt, false);
        return Action::None;
    }

    if (IsKeyPressed(KEY_B)) { W.build = !W.build; W.camDist = W.build ? 45.0f : 11.0f; sfx::Play(sfx::CLICK, 0); }
    if (IsKeyPressed(KEY_C)) { W.blueprint = true; sfx::Play(sfx::CLICK, 0); return Action::None; }

    UpdatePlayer(dt);
    UpdateCamera(dt, false);

    // ---- Chế độ xây bằng chuột ----
    if (W.build) {
        Ray ray = GetScreenToWorldRay(GetMousePosition(), W.cam);
        W.hoverOk = false;
        if (std::fabs(ray.direction.y) > 0.001f) {
            float tHit = -ray.position.y / ray.direction.y;
            if (tHit > 0) {
                Vector3 hit = Vector3Add(ray.position, Vector3Scale(ray.direction, tHit));
                PixelToHex(hit.x, hit.z, W.hoverQ, W.hoverR);
                W.hoverOk = CellAt(W.hoverQ, W.hoverR) != nullptr;
            }
        }
        // Chọn vật thể: phím số hoặc Tab
        if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_PERIOD)) { do { W.sel = W.sel % (ITEM_COUNT - 2) + 1; } while (W.sel == STUDIO); }
        if (IsKeyPressed(KEY_COMMA)) { W.sel = (W.sel + ITEM_COUNT - 4) % (ITEM_COUNT - 2) + 1; }
        if (IsKeyPressed(KEY_R)) W.selRot = (W.selRot + 1) % 6;
        float s = ui::S();
        bool overUi = GetMousePosition().y > GetScreenHeight() - 120 * s;
        if (W.hoverOk && !overUi) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                std::string err = PlaceError(W.hoverQ, W.hoverR, W.sel);
                if (!err.empty()) { Say("Không xây được", err, 3); sfx::Play(sfx::WRONG, 0); }
                else {
                    int rot = Def(W.sel).cat == CAT_BUILDING ? W.selRot : W.selRot;
                    W.history.push_back(W.cells);
                    if (W.history.size() > 30) W.history.erase(W.history.begin());
                    Place(W.hoverQ, W.hoverR, W.sel, rot);
                    W.lastRunPlaced = 0;
                    SyncNpcs();
                    CheckQuests();
                    SaveWorld();
                    sfx::Play(sfx::STEP, 0.2f);
                }
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_X)) {
                W.history.push_back(W.cells);
                Remove(W.hoverQ, W.hoverR);
                SyncNpcs();
                SaveWorld();
                sfx::Play(sfx::CLICK, 0.1f);
            }
        }
        if (Ctrl() && IsKeyPressed(KEY_Z) && !W.history.empty()) {   // hoàn tác
            W.cells = W.history.back();
            W.history.pop_back();
            for (Chunk& ch : W.chunks) ch.dirty = true;
            W.minimapDirty = true;
            W.pending.clear();
            SyncNpcs();
            SaveWorld();
        }
        if (IsKeyPressed(KEY_ESCAPE)) { W.build = false; W.camDist = 11.0f; }
        return Action::None;
    }

    // ---- Tương tác ----
    if (IsKeyPressed(KEY_E)) {
        for (size_t i = 0; i < W.npcs.size(); ++i)
            if (Vector3Distance(W.npcs[i].pos, W.pos) < 3.2f) { StartTalk((int)i); return Action::None; }
        for (const Cell& c : W.cells) {
            if (c.item != STUDIO && c.item != DAU_TRUONG && c.item != THU_VIEN) continue;
            if (Vector3Distance(DoorOf(c), W.pos) > (c.item == DAU_TRUONG ? 7.0f : 5.0f)) continue;
            sfx::Play(sfx::CLICK, 0);
            if (c.item == STUDIO) return Action::EnterOffice;
            if (c.item == THU_VIEN) return Action::OpenLesson;
            if (g_progress.Has(F_ARENA)) return Action::EnterArena;
            Say("Đấu trường", "Bên trong chưa có game loop nên chưa đấu được. Học xong Bài 2 ở Studio nhé!", 5);
        }
    }
    return Action::None;
}

// -----------------------------------------------------------------------------
//  RENDER
// -----------------------------------------------------------------------------
void world::RenderOffscreen() {
    if (!W.ready) return;
    SetupEnvironment();
    gfx::RenderShadowMap([] {
        DrawWorldGeometry();
        W.player.Draw(W.pos, W.yaw, 0.8f);
        for (Npc& n : W.npcs) n.ch->Draw(n.pos, n.yaw, 0.8f);
    });
}

void world::Draw3D() {
    float Wd = (float)GetScreenWidth(), Hd = (float)GetScreenHeight();
    DrawRectangleGradientV(0, 0, (int)Wd, (int)Hd, SkyTop(), SkyBottom());
    float n = Night();
    if (n > 0.05f)
        for (int i = 0; i < 140; ++i) {                              // sao
            float x = std::fmod(i * 137.5f, Wd), y = std::fmod(i * 71.3f, Hd * 0.55f);
            float tw = 0.5f + 0.5f * std::sin(W.t * 2 + i);
            DrawCircle((int)x, (int)y, 1.0f + (i % 3) * 0.5f, Fade(WHITE, n * (0.3f + 0.6f * tw)));
        }
    if (!W.ready) return;

    BeginMode3D(W.cam);
    // Mặt trời / mặt trăng
    Vector3 sd = SunDirection();
    Vector3 sp = Vector3Add(W.cam.position, Vector3Scale(sd, 600.0f));
    gfx::BeginGlow();
    DrawSphere(sp, n > 0.5f ? 14.0f : 22.0f, n > 0.5f ? Color{ 200, 210, 255, 255 } : Color{ 255, 236, 180, 255 });
    gfx::EndGlow();

    gfx::BeginLit(W.cam);
    DrawOcean();
    DrawWorldGeometry();
    W.player.Draw(W.pos, W.yaw, 0.8f);
    for (Npc& np : W.npcs) np.ch->Draw(np.pos, np.yaw, 0.8f);
    // Mây trôi
    if (Proto* cl = GetProto("cloud_big")) {
        for (int i = 0; i < 9; ++i) {
            float x = std::fmod(i * 97.0f + W.t * 2.0f, 520.0f) - 260.0f;
            float z = -200.0f + i * 47.0f;
            Matrix m = MatrixMultiply(MatrixScale(WS * 2.2f, WS * 1.4f, WS * 2.2f), MatrixTranslate(x, 70.0f + (i % 3) * 12.0f, z));
            for (int k = 0; k < cl->model.meshCount; ++k) gfx::DrawMeshLit(cl->model.meshes[k], W.mat, m, 0.15f);
        }
    }
    // Bóng ma của vật thể sắp đặt (chế độ xây)
    if (W.build && W.hoverOk) {
        const ItemDef& d = Def(W.sel);
        bool ok = PlaceError(W.hoverQ, W.hoverR, W.sel).empty();
        Proto* p = nullptr;
        if (W.sel == DUONG) p = GetProto("hex_road_A");
        else p = GetProto(d.file);
        if (p) {
            Vector3 c = HexCenter(W.hoverQ, W.hoverR);
            float yaw = d.cat == CAT_BUILDING ? BuildingYaw(W.selRot) : W.selRot * 60.0f * DEG2RAD;
            float sc = WS * ((d.cat == CAT_PROP) ? 1.6f : 1.0f);
            float bob = (d.cat == CAT_GROUND) ? 0.3f : 0.4f + std::sin(W.t * 4) * 0.3f;
            Matrix m = MatrixMultiply(MatrixMultiply(MatrixScale(sc, sc, sc), MatrixRotateY(yaw)), MatrixTranslate(c.x, bob, c.z));
            for (int k = 0; k < p->model.meshCount; ++k) gfx::DrawMeshLit(p->model.meshes[k], W.mat, m, ok ? 0.25f : 0.0f);
        }
    }
    gfx::EndLit();

    gfx::BeginGlow();
    DrawLamps(true);
    if (W.build && W.hoverOk) {                                       // viền ô đang chọn
        Vector3 c = HexCenter(W.hoverQ, W.hoverR);
        bool ok = PlaceError(W.hoverQ, W.hoverR, W.sel).empty();
        Color col = ok ? Color{ 120, 255, 160, 255 } : Color{ 255, 90, 90, 255 };
        for (int i = 0; i < 6; ++i) {
            float a0 = (30 + i * 60) * DEG2RAD, a1 = (90 + i * 60) * DEG2RAD;
            float R = WS * 1.1547f;
            DrawLine3D({ c.x + std::cos(a0) * R, 0.15f, c.z + std::sin(a0) * R }, { c.x + std::cos(a1) * R, 0.15f, c.z + std::sin(a1) * R }, col);
            DrawCube({ c.x + std::cos(a0) * R, 0.15f, c.z + std::sin(a0) * R }, 0.3f, 0.3f, 0.3f, col);
        }
    }
    // Dấu chấm than trên đầu dân làng / mũi tên ở cửa các công trình đi vào được
    for (const Cell& c : W.cells)
        if (c.item == STUDIO || c.item == DAU_TRUONG || c.item == THU_VIEN) {
            Vector3 d = DoorOf(c);
            float bob = std::sin(W.t * 3) * 0.3f;
            DrawCube({ d.x, 3.2f + bob, d.z }, 0.35f, 0.35f, 0.35f, c.item == STUDIO ? pal::accent : (c.item == THU_VIEN ? pal::good : pal::accent2));
        }
    gfx::EndGlow();
    EndMode3D();
}

// -----------------------------------------------------------------------------
//  HUD
// -----------------------------------------------------------------------------
static void DrawPrompt3D(Vector3 p, const std::string& text) {
    float s = ui::S();
    Vector2 sp = GetWorldToScreen(p, W.cam);
    Vector2 m = ui::Measure(text, 17 * s, FS_BOLD);
    Rectangle r = { sp.x - m.x / 2 - 30 * s, sp.y - 20 * s, m.x + 44 * s, 34 * s };
    DrawRectangleRounded(r, 0.4f, 6, Color{ 15, 17, 26, 225 });
    DrawRectangleRoundedLinesEx(r, 0.4f, 6, 1.5f, pal::accent);
    Rectangle key = { r.x + 6 * s, r.y + 5 * s, 24 * s, 24 * s };
    DrawRectangleRounded(key, 0.3f, 4, pal::accent);
    ui::Text("E", key.x + 6.5f * s, key.y + 1.5f * s, 19 * s, pal::bg, FS_BOLD);
    ui::Text(text, r.x + 36 * s, r.y + 7 * s, 17 * s, pal::text, FS_BOLD);
}

static void DrawBlueprint() {
    float s = ui::S();
    float Wd = (float)GetScreenWidth(), Hd = (float)GetScreenHeight();
    DrawRectangle(0, 0, (int)Wd, (int)Hd, Color{ 6, 8, 14, 200 });
    ui::Text("BẢN THIẾT KẾ  ·  xây thế giới bằng C++", 30 * s, 22 * s, 26 * s, pal::accent, FS_BOLD);
    ui::Text("Viết hàm void XayDung(). F5: chạy · Esc: rời ô code · C/Esc: đóng · code lưu ở code/ban_thiet_ke.cpp",
             30 * s, 58 * s, 15 * s, pal::dim);
    Rectangle ed = { 30 * s, 92 * s, Wd * 0.58f, Hd - 92 * s - 150 * s };
    W.editor.Draw(ed);

    // Kết quả
    Rectangle out = { ed.x, ed.y + ed.height + 10 * s, ed.width, Hd - (ed.y + ed.height + 10 * s) - 70 * s };
    ui::Panel(out, pal::panel, W.errors.empty() ? pal::line : pal::bad, 0.05f);
    std::string res;
    if (!W.errors.empty()) res = TextFormat("Lỗi (dòng %d): %s", W.errors[0].line, W.errors[0].msg.c_str());
    else res = W.output.empty() ? "Kết quả chạy hiện ở đây." : W.output;
    BeginScissorMode((int)out.x, (int)out.y, (int)out.width, (int)out.height);
    ui::Wrapped(res, { out.x + 12 * s, out.y + 8 * s, out.width - 24 * s, 0 }, 14 * s, W.errors.empty() ? pal::text : Color{ 255, 200, 200, 255 });
    EndScissorMode();

    float by = Hd - 58 * s;
    if (ui::Button({ ed.x, by, 190 * s, 42 * s }, "▶ Chạy (F5)", true, pal::good)) RunBlueprint();
    if (ui::Button({ ed.x + 200 * s, by, 190 * s, 42 * s }, "Kiến thức tiếp ▶", true, pal::accent2)) {
        W.exampleIdx = (W.exampleIdx + 1) % KNOW_COUNT;
    }
    if (ui::Button({ ed.x + 400 * s, by, 220 * s, 42 * s }, "Hoàn tác lần xây", !W.history.empty(), pal::purple)) {
        W.cells = W.history.back();
        W.history.pop_back();
        W.pending.clear();
        for (Chunk& ch : W.chunks) ch.dirty = true;
        W.minimapDirty = true;
        SyncNpcs();
        SaveWorld();
    }
    if (ui::Button({ ed.x + 630 * s, by, 130 * s, 42 * s }, "Đóng", true, pal::bad)) W.blueprint = false;

    // Danh sách lệnh (API)
    Rectangle doc = { ed.x + ed.width + 20 * s, 92 * s, Wd - (ed.x + ed.width + 20 * s) - 30 * s, Hd - 92 * s - 70 * s };
    ui::Panel(doc, pal::panel, pal::line, 0.03f);
    BeginScissorMode((int)doc.x, (int)doc.y, (int)doc.width, (int)doc.height);
    float y = doc.y + 12 * s;
    Rectangle tb = { doc.x + 14 * s, 0, doc.width - 28 * s, 0 };
    auto para = [&](const std::string& t, float size, Color c) { tb.y = y; y += ui::Wrapped(t, tb, size * s, c) + 4 * s; };
    {   // Thẻ kiến thức hiện tại (chỉ dạy cú pháp — code xây đảo là của bạn)
        const KnowCard& k = KNOW[W.exampleIdx % KNOW_COUNT];
        para(TextFormat("# KIẾN THỨC %d/%d — %s", W.exampleIdx % KNOW_COUNT + 1, KNOW_COUNT, k.title), 15, pal::accent);
        para(k.text, 13.5f, pal::text);
        Rectangle cb = { tb.x, y, tb.width, 0 };
        float ch = ui::Code(k.code, cb, 13 * s, false);
        DrawRectangleRounded({ cb.x - 4 * s, y - 4 * s, cb.width + 8 * s, ch + 8 * s }, 0.05f, 4, pal::bg);
        ui::Code(k.code, cb, 13 * s);
        y += ch + 14 * s;
    }
    para("# Lệnh xây dựng", 15, pal::text);
    para("- Dat(q, r, VAT_THE): đặt vật thể / mặt đất tại ô (q, r)\n- Dat(q, r, VAT_THE, huong): huong 0..5 là hướng cửa\n- Xoa(q, r): xóa vật thể (hoặc trả ô về cỏ)", 13.5f, pal::text);
    para("# Lệnh hỏi", 15, pal::text);
    para("- OTrong(q, r), LaDat(q, r), LaNuoc(q, r), TrongDao(q, r): true/false\n- VatThe(q, r): vật thể đang ở ô\n- KhoangCach(q1, r1, q2, r2): số ô giữa 2 ô\n- LangGiengQ(h), LangGiengR(h): bước sang ô kề theo hướng h (0..5)\n- NgauNhien(a, b): số ngẫu nhiên từ a đến b\n- ViTriQ(), ViTriR(): ô bạn đang đứng", 13.5f, pal::text);
    para("# Vật thể", 15, pal::text);
    std::string list;
    for (int i = CO; i < ITEM_COUNT; ++i) {
        if (i == STUDIO) continue;
        const ItemDef& d = Def(i);
        bool locked = g_progress.completed < d.unlock;
        list += std::string(d.code) + " — " + d.name + (locked ? TextFormat(" (Bài %d)", d.unlock) : "") + "\n";
    }
    para(list, 13, pal::dim);
    para("> Tọa độ ô: q tăng sang phải, r tăng về phía dưới-phải. Studio ở (0, 0), cửa nhìn về ô (0, 1).", 13, pal::text);
    EndScissorMode();
}

static void DrawMinimap() {
    if (W.minimap.id == 0) return;
    float s = ui::S();
    float size = 190 * s;
    float Wd = (float)GetScreenWidth();
    Rectangle r = { Wd - size - 16 * s, 16 * s, size, size };
    DrawCircle((int)(r.x + size / 2), (int)(r.y + size / 2), size / 2 + 4 * s, Color{ 10, 14, 24, 200 });
    DrawTexturePro(W.minimap.texture, { 0, 0, 256, -256 }, r, { 0, 0 }, 0, WHITE);
    float extent = WS * 2.0f * (RAD + 1);
    auto toMap = [&](Vector3 p) { return Vector2{ r.x + size / 2 + p.x / (2 * extent) * size, r.y + size / 2 + p.z / (2 * extent) * size }; };
    for (const Npc& n : W.npcs) DrawCircleV(toMap(n.pos), 2.5f * s, YELLOW);
    Vector2 pp = toMap(W.pos);
    float a = W.yaw * DEG2RAD;
    Vector2 f = { std::sin(a), std::cos(a) }, side = { f.y, -f.x };
    float k = 7 * s;
    DrawTriangle({ pp.x + f.x * k, pp.y + f.y * k }, { pp.x - f.x * k * 0.6f + side.x * k * 0.6f, pp.y - f.y * k * 0.6f + side.y * k * 0.6f },
                 { pp.x - f.x * k * 0.6f - side.x * k * 0.6f, pp.y - f.y * k * 0.6f - side.y * k * 0.6f }, WHITE);
    DrawTriangle({ pp.x + f.x * k, pp.y + f.y * k }, { pp.x - f.x * k * 0.6f - side.x * k * 0.6f, pp.y - f.y * k * 0.6f - side.y * k * 0.6f },
                 { pp.x - f.x * k * 0.6f + side.x * k * 0.6f, pp.y - f.y * k * 0.6f + side.y * k * 0.6f }, WHITE);
    int hh = (int)W.hour, mm = (int)((W.hour - hh) * 60);
    ui::TextCentered(TextFormat("%02d:%02d %s", hh, mm, Night() > 0.5f ? "· Đêm" : ""), r.x + size / 2, r.y + size + 6 * s, 16 * s, pal::text, FS_BOLD);
}

void world::DrawHUD() {
    if (!W.ready) return;
    float s = ui::S();
    float Wd = (float)GetScreenWidth(), Hd = (float)GetScreenHeight();

    if (W.blueprint) { DrawBlueprint(); return; }

    // Bảng thông tin + nhiệm vụ
    Rectangle hud = { 16 * s, 16 * s, 340 * s, 76 * s };
    ui::Panel(hud, Fade(pal::bg, 0.88f), pal::line, 0.08f);
    ui::Text("ĐẢO LẬP TRÌNH", hud.x + 14 * s, hud.y + 10 * s, 20 * s, pal::accent, FS_BOLD);
    ui::Text(TextFormat("Bài đã học: %d/%d    Xu: %d", g_progress.completed, F_COUNT, W.coins), hud.x + 14 * s, hud.y + 40 * s, 15 * s, pal::text);
    float qy = hud.y + hud.height + 8 * s;
    int shown = 0;
    for (const Quest& q : W.quests) {
        if (q.done) continue;
        if (shown++ >= 3) break;
        Rectangle qr = { hud.x, qy, hud.width, 26 * s };
        DrawRectangleRounded(qr, 0.3f, 4, Fade(pal::bg, 0.75f));
        ui::Text(std::string("○  ") + q.text, qr.x + 10 * s, qr.y + 5 * s, 14 * s, pal::text);
        qy += 30 * s;
    }
    if (shown == 0) ui::Text("✓ Đã xong mọi nhiệm vụ — tiếp tục xây tùy thích!", hud.x, qy, 14 * s, pal::good);

    DrawMinimap();

    // Gợi ý tương tác
    if (!W.build && W.talkNpc < 0) {
        bool prompted = false;
        for (size_t i = 0; i < W.npcs.size() && !prompted; ++i)
            if (Vector3Distance(W.npcs[i].pos, W.pos) < 3.2f) {
                DrawPrompt3D(Vector3Add(W.npcs[i].pos, { 0, 2.6f, 0 }), "Nói chuyện với " + W.npcs[i].name);
                prompted = true;
            }
        for (const Cell& c : W.cells) {
            if (prompted) break;
            if (c.item != STUDIO && c.item != DAU_TRUONG && c.item != THU_VIEN) continue;
            Vector3 d = DoorOf(c);
            if (Vector3Distance(d, W.pos) > (c.item == DAU_TRUONG ? 7.0f : 5.0f)) continue;
            const char* t = c.item == STUDIO ? "Vào Studio của Minh" : (c.item == THU_VIEN ? "Thư viện: học bài" : "Vào Đấu trường");
            DrawPrompt3D({ d.x, 2.6f, d.z }, t);
            prompted = true;
        }
    }

    // Thanh chọn vật thể (chế độ xây)
    if (W.build) {
        float barH = 104 * s;
        Rectangle bar = { 0, Hd - barH, Wd, barH };
        DrawRectangleRec(bar, Color{ 10, 12, 20, 220 });
        const ItemDef& d = Def(W.sel);
        bool locked = g_progress.completed < d.unlock;
        ui::Text(TextFormat("ĐANG CHỌN: %s  (%s)%s", d.name, d.code, locked ? TextFormat("  — cần học Bài %d", d.unlock) : ""),
                 16 * s, bar.y + 8 * s, 17 * s, locked ? pal::bad : pal::accent, FS_BOLD);
        float x = 16 * s, cw = 98 * s;
        int first = std::max(1, std::min(W.sel - 5, (int)ITEM_COUNT - 2 - (int)((Wd - 32 * s) / (cw + 6 * s)) + 1));
        for (int i = first; i < ITEM_COUNT && x + cw < Wd - 16 * s; ++i) {
            if (i == STUDIO) continue;
            const ItemDef& it = Def(i);
            Rectangle r = { x, bar.y + 36 * s, cw, 56 * s };
            bool cur = i == W.sel;
            bool lk = g_progress.completed < it.unlock;
            ui::Panel(r, cur ? Fade(pal::accent, 0.25f) : pal::panel, cur ? pal::accent : pal::line, 0.12f);
            ui::Wrapped(it.name, { r.x + 6 * s, r.y + 6 * s, r.width - 12 * s, 0 }, 13 * s, lk ? pal::dim : pal::text);
            if (ui::Hover(r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) W.sel = i;
            x += cw + 6 * s;
        }
        ui::Text("Chuột trái: đặt · Chuột phải / X: xóa · R: xoay · Tab / , . : đổi vật thể · Ctrl+Z: hoàn tác · Q/E: xoay camera · B: thoát",
                 Wd - ui::Measure("Chuột trái: đặt · Chuột phải / X: xóa · R: xoay · Tab / , . : đổi vật thể · Ctrl+Z: hoàn tác · Q/E: xoay camera · B: thoát", 13 * s).x - 16 * s,
                 bar.y + 10 * s, 13 * s, pal::dim);
        if (W.hoverOk) ui::Text(TextFormat("Ô (%d, %d)", W.hoverQ, W.hoverR), GetMousePosition().x + 16, GetMousePosition().y + 8, 15 * s, WHITE, FS_BOLD);
    } else {
        DrawRectangle(0, (int)(Hd - 36 * s), (int)Wd, (int)(36 * s), Color{ 0, 0, 0, 140 });
        ui::Text("WASD: đi · Shift: chạy · Space: nhảy · Chuột phải kéo / Q: xoay camera · Lăn chuột: zoom · E: tương tác · B: XÂY bằng chuột · C: XÂY bằng CODE",
                 16 * s, Hd - 27 * s, 14 * s, pal::text);
    }

    // Hội thoại với dân làng
    if (W.talkNpc >= 0 && W.talkNpc < (int)W.npcs.size()) {
        float bw = std::min(Wd - 40 * s, 760 * s);
        Rectangle box = { (Wd - bw) / 2, Hd - 300 * s, bw, 250 * s };
        ui::Panel(box, Fade(pal::bg, 0.95f), pal::accent2, 0.06f);
        ui::Text(W.npcs[W.talkNpc].name, box.x + 20 * s, box.y + 12 * s, 17 * s, pal::accent2, FS_BOLD);
        float y = box.y + 42 * s;
        if (W.quizLesson >= 0) {
            const Question& q = GetLessons()[W.quizLesson].quiz[W.quizQ];
            y += ui::Wrapped(TextFormat("Này, đố bạn câu này (Bài %d): %s", W.quizLesson + 1, q.q.c_str()), { box.x + 20 * s, y, bw - 40 * s, 0 }, 16 * s, pal::text) + 8 * s;
            for (size_t k = 0; k < W.quizOrder.size(); ++k) {
                int opt = W.quizOrder[k];
                Color c = pal::text;
                if (W.quizPicked >= 0 && opt == q.correct) c = pal::good;
                else if (W.quizPicked == opt) c = pal::bad;
                ui::Text(TextFormat("%d. %s", (int)k + 1, q.options[opt].c_str()), box.x + 30 * s, y, 15 * s, c);
                y += 24 * s;
            }
            if (W.quizPicked >= 0)
                ui::Text(W.quizPicked == q.correct ? "Giỏi quá! +5 xu. (E để đóng)" : "Chưa đúng rồi, về Studio ôn lại nhé! (E để đóng)",
                         box.x + 20 * s, box.y + box.height - 30 * s, 15 * s, W.quizPicked == q.correct ? pal::good : pal::bad, FS_BOLD);
            else ui::Text("Nhấn 1-4 để trả lời", box.x + 20 * s, box.y + box.height - 30 * s, 14 * s, pal::dim);
        } else {
            static const char* LINES[] = {
                "Cảm ơn bạn đã xây nhà cho tôi! Nghe nói ở Studio có anh Minh dạy lập trình game hay lắm.",
                "Bạn biết không, chỉ với 2 vòng for lồng nhau là xây được cả một khu phố đấy. Thử nhấn C xem!",
                "Tối đến nhớ thắp đèn đường nhé. Đèn sáng nhờ shader ánh sáng điểm (Bài 15) đó.",
                "Hãy xây một Đấu trường, rồi bước vào để đấu bằng chính code bạn viết!",
                "Mỗi khi xây xong, thế giới được lưu vào file world.txt. Mở ra xem thử, toàn là số thôi!",
                "Xây Thư viện để ôn bài ngay giữa làng, khỏi phải về Studio.",
            };
            ui::Wrapped(LINES[(W.talkNpc + (int)W.hour) % 6], { box.x + 20 * s, y, bw - 40 * s, 0 }, 16 * s, pal::text);
            ui::Text("E để đóng", box.x + 20 * s, box.y + box.height - 30 * s, 14 * s, pal::dim);
        }
    }

    // Lời nhắn
    if (W.sayTime > 0 && !W.sayText.empty() && W.talkNpc < 0 && !W.build) {
        float a = Clamp01(W.sayTime / 0.4f);
        float bw = std::min(Wd - 40 * s, 700 * s);
        float th = ui::Wrapped(W.sayText, { 0, 0, bw - 40 * s, 0 }, 17 * s, pal::text, false);
        Rectangle box = { (Wd - bw) / 2, Hd * 0.18f, bw, th + 48 * s };
        DrawRectangleRounded(box, 0.12f, 6, Fade2(Color{ 15, 17, 26, 235 }, a));
        DrawRectangleRoundedLinesEx(box, 0.12f, 6, 1.5f, Fade2(pal::accent, a));
        ui::Text(W.sayWho, box.x + 20 * s, box.y + 10 * s, 15 * s, Fade2(pal::accent, a), FS_BOLD);
        ui::Wrapped(W.sayText, { box.x + 20 * s, box.y + 34 * s, bw - 40 * s, 0 }, 17 * s, Fade2(pal::text, a));
    }
}
