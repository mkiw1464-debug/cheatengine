#pragma once
#include "Il2CppHelper.h"
#include "Offsets.h"
#include <simd/simd.h>

// ═══════════════════════════════════════
//  PlayerEntity — wraps COW.GamePlay.Player*
//  OB54 offset map from dump.cs
// ═══════════════════════════════════════

struct Vector3 { float x, y, z; };
struct Vector2 { float x, y; };

// ── PropertyData HP reader ────────────────────────────────
// PropertyData stores properties as an int array internally.
// EPRI_PLAYER_CUR_HP=0, EPRI_PLAYER_MAX_HP=1
// The PropertyData class at OFF_PLAYER_PROPDATA has an
// internal array field. Based on FF dump pattern: first object
// field is the backing array, read as: propdata->m_arr[index]
struct PropertyDataArray {
    uintptr_t obj_klass;   // 0x0
    uintptr_t obj_monitor; // 0x8
    uintptr_t arr_bounds;  // 0x10
    uint32_t  arr_len;     // 0x18
    // values start at 0x20 each 4 bytes (int)
    int32_t values[1];
};

// ── Bone target selection ─────────────────────────────────
enum class BoneTarget : int {
    Head   = 0,
    Neck   = 1,
    Body   = 2, // Spine2
    Pelvis = 3,
    LLeg   = 4,
    RLeg   = 5,
};

static const char* kBoneNames[] = {
    BONE_NAME_HEAD,
    BONE_NAME_NECK,
    BONE_NAME_SPINE,
    BONE_NAME_PELVIS,
    BONE_NAME_LTHIGH,
    BONE_NAME_RTHIGH,
};

// ── GetBoneTransform native function type ─────────────────
// RVA: 0x594D978 — Player::GetBoneTransform(string boneName) → DAPOIBANHEH*
using fn_GetBoneTransform = uintptr_t (*)(uintptr_t player, Il2CppString* boneName, void** exc);

// ── Transform::get_position icall ────────────────────────
using fn_TransformGetPosition = void (*)(uintptr_t transform, Vector3* out);

// ── Camera::WorldToScreenPoint icall ─────────────────────
using fn_W2S = void (*)(uintptr_t camera, Vector3* world, int eye, Vector2* out);

// ── Screen::get_width / get_height ───────────────────────
using fn_ScreenDim = int (*)();

namespace ME {

static fn_GetBoneTransform    g_GetBoneTransform = nullptr;
static fn_TransformGetPosition g_GetPosition     = nullptr;
static fn_W2S                  g_W2S             = nullptr;
static fn_ScreenDim            g_ScreenW         = nullptr;
static fn_ScreenDim            g_ScreenH         = nullptr;

static bool InitFunctions() {
    auto base = Il2Cpp::GetGameBase();
    if (!base) return false;

    // GetBoneTransform — Offset: 0x594D978
    g_GetBoneTransform = (fn_GetBoneTransform)(base + 0x594D978);

    // Unity icalls
    auto icall = Il2Cpp::API::resolve_icall;
    if (!icall) return false;

    g_GetPosition = (fn_TransformGetPosition)
        icall("UnityEngine.Transform::get_position_Injected");

    g_W2S = (fn_W2S)
        icall("UnityEngine.Camera::WorldToScreenPoint_Injected");

    g_ScreenW = (fn_ScreenDim)icall("UnityEngine.Screen::get_width");
    g_ScreenH = (fn_ScreenDim)icall("UnityEngine.Screen::get_height");

    return g_GetBoneTransform && g_W2S;
}

// ── PlayerEntity wrapper ──────────────────────────────────
class PlayerEntity {
public:
    uintptr_t ptr = 0;

    explicit PlayerEntity(uintptr_t p) : ptr(p) {}
    bool valid() const { return ptr != 0; }

    // ── HP read via PropertyData ──────────────────────────
    int GetHP() const {
        auto propdata = Il2Cpp::ReadPtr(ptr, OFF_PLAYER_PROPDATA);
        if (!propdata) return 0;
        // PropertyData holds a backing Il2Cpp array at offset 0x20 (typical)
        // Read field 0 of PropertyData object = m_propArr pointer
        auto arr = Il2Cpp::ReadPtr(propdata, 0x20);
        if (!arr) return 0;
        auto* pd = reinterpret_cast<PropertyDataArray*>(arr);
        if (EPRI_CUR_HP >= (int)pd->arr_len) return 0;
        return pd->values[EPRI_CUR_HP];
    }

    int GetMaxHP() const {
        auto propdata = Il2Cpp::ReadPtr(ptr, OFF_PLAYER_PROPDATA);
        if (!propdata) return 200;
        auto arr = Il2Cpp::ReadPtr(propdata, 0x20);
        if (!arr) return 200;
        auto* pd = reinterpret_cast<PropertyDataArray*>(arr);
        if (EPRI_MAX_HP >= (int)pd->arr_len) return 200;
        return pd->values[EPRI_MAX_HP];
    }

    // ── Team ──────────────────────────────────────────────
    uint32_t GetTeamID() const {
        return *(uint32_t*)(ptr + OFF_PLAYER_TEAMMODEID);
    }

    // ── Dead check ────────────────────────────────────────
    // get_IsDead VA: 0x53799CC
    using fn_IsDead = bool (*)(uintptr_t, void**);
    bool IsDead() const {
        static fn_IsDead f = nullptr;
        if (!f) f = (fn_IsDead)(Il2Cpp::GetGameBase() + 0x53799CC);
        void* exc = nullptr;
        return f(ptr, &exc);
    }

    // ── Bot check ─────────────────────────────────────────
    bool IsBot() const {
        return *(bool*)(ptr + OFF_PLAYER_ISCLIENTBOT);
    }

    // ── Name ─────────────────────────────────────────────
    std::string GetName() const {
        auto sptr = Il2Cpp::ReadPtr(ptr, OFF_PLAYER_ORIGINALNAME);
        if (!sptr) return "???";
        return Il2Cpp::Il2CppStringToStd((Il2CppString*)sptr);
    }

    // ── Bone world position ───────────────────────────────
    bool GetBonePosition(BoneTarget bone, Vector3& out) const {
        if (!g_GetBoneTransform || !g_GetPosition) return false;

        const char* boneName = kBoneNames[(int)bone];
        auto* il2str = Il2Cpp::API::NewString(boneName);
        if (!il2str) return false;

        void* exc = nullptr;
        uintptr_t boneNode = g_GetBoneTransform(ptr, il2str, &exc);
        if (!boneNode || exc) return false;

        // DAPOIBANHEH: Transform* at offset 0x10
        uintptr_t transform = Il2Cpp::ReadPtr(boneNode, OFF_BONENODE_TRANSFORM);
        if (!transform) return false;

        g_GetPosition(transform, &out);
        return true;
    }

    // ── Best aimed bone position based on BoneTarget ─────
    bool GetTargetPos(BoneTarget bone, Vector3& out) const {
        return GetBonePosition(bone, out);
    }

    // ── Rough center position (pelvis fallback) ───────────
    Vector3 GetCenter() const {
        Vector3 v = {0,0,0};
        GetBonePosition(BoneTarget::Body, v);
        return v;
    }
};

// ── World to Screen ───────────────────────────────────────
struct ScreenPt { float x, y; bool onScreen; };

static ScreenPt W2S(uintptr_t camera, const Vector3& world) {
    ScreenPt out = {0,0,false};
    if (!g_W2S || !camera) return out;

    Vector3 wc = world;
    Vector2 screen = {0,0};
    g_W2S(camera, &wc, 0, &screen);

    int sw = g_ScreenW ? g_ScreenW() : 1080;
    int sh = g_ScreenH ? g_ScreenH() : 1920;

    // Unity screen Y is bottom-up; flip for UI top-down
    float fx = screen.x;
    float fy = (float)sh - screen.y;

    out.onScreen = (fx > 0 && fx < sw && fy > 0 && fy < sh);
    out.x = fx;
    out.y = fy;
    return out;
}

} // namespace ME
