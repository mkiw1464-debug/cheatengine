#pragma once
#include "PlayerEntity.h"
#include "PlayerScanner.h"
#include <cmath>
#include <optional>
#include <vector>

// ═══════════════════════════════════════
//  Aimbot — silent + legit, adjustable FOV, bone select
// ═══════════════════════════════════════

namespace ME {

struct AimbotConfig {
    bool       enabled     = false;
    bool       silentAim   = false;   // bullet redirect
    bool       legitAim    = false;   // smooth humanlike
    float      fov         = 90.0f;   // degrees, 0–360
    float      smoothing   = 5.0f;    // legit: higher = slower
    BoneTarget boneTarget  = BoneTarget::Head;
    bool       aimHead     = true;
    bool       aimNeck     = false;
    bool       aimBody     = false;
    bool       aimLeg      = false;
};

// ── Hook: SetAimRotation for silent aim ───────────────────
using fn_SetAimRot = void (*)(uintptr_t player, float yaw, float pitch, void** exc);
static fn_SetAimRot g_SetAimRot = nullptr;

static bool InitAimbot() {
    auto* cls = Il2Cpp::API::FindClass("COW.GamePlay", "Player");
    if (!cls) return false;
    // SetAimRotation — RVA fallback via substrate at runtime
    return true;
}

// ── Math helpers ──────────────────────────────────────────
struct AimAngles { float yaw, pitch; };

static float Distance2D(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2, dy = y1 - y2;
    return sqrtf(dx * dx + dy * dy);
}

static float Distance3D(const Vector3& a, const Vector3& b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

static float AngularDist(float sx, float sy, float cx, float cy) {
    return Distance2D(sx, sy, cx, cy);
}

// ── Find best target within FOV ───────────────────────────
struct AimTarget {
    PlayerEntity player;
    Vector3      worldPos;
    ScreenPt     screenPos;
    float        dist;
};

static std::optional<AimTarget> FindBestTarget(
    const std::vector<PlayerEntity>& enemies,
    uintptr_t camera,
    const AimbotConfig& cfg
) {
    if (!camera || enemies.empty()) return std::nullopt;

    int sw = g_ScreenW ? g_ScreenW() : 1080;
    int sh = g_ScreenH ? g_ScreenH() : 1920;
    float cx = sw * 0.5f, cy = sh * 0.5f;

    float maxPx = (sqrtf((float)(sw * sw + sh * sh)) * 0.5f) * (cfg.fov / 360.0f);

    std::optional<AimTarget> best;
    float bestDist = 1e9f;

    for (const auto& enemy : enemies) {
        if (enemy.IsDead()) continue;

        Vector3 wpos;
        if (!enemy.GetBonePosition(cfg.boneTarget, wpos)) continue;

        auto sp = W2S(camera, wpos);
        if (!sp.onScreen) continue;

        float d = AngularDist(sp.x, sp.y, cx, cy);
        if (d > maxPx) continue;

        if (d < bestDist) {
            bestDist = d;
            best = AimTarget{ enemy, wpos, sp, Distance3D(wpos, {0, 0, 0}) };
        }
    }
    return best;
}

// ── Apply silent aim ──────────────────────────────────────
static Vector3 g_SilentTarget    = { 0, 0, 0 };
static bool    g_HasSilentTarget = false;

static void ApplySilentAim(const AimTarget& target) {
    g_SilentTarget    = target.worldPos;
    g_HasSilentTarget = true;
    // Actual redirection in fire hook (Hooks.mm)
}

// ── Apply legit aim (smooth) ──────────────────────────────
static float g_SmoothYaw   = 0;
static float g_SmoothPitch = 0;

static void ApplyLegitAim(uintptr_t localPlayer, const AimTarget& target, const AimbotConfig& cfg) {
    if (!localPlayer || !g_SetAimRot) return;

    int sw = g_ScreenW ? g_ScreenW() : 1080;
    int sh = g_ScreenH ? g_ScreenH() : 1920;
    float cx = sw * 0.5f, cy = sh * 0.5f;

    float dx = target.screenPos.x - cx;
    float dy = target.screenPos.y - cy;

    float yawDelta   =  (dx / (float)sw) * cfg.fov;
    float pitchDelta = -(dy / (float)sh) * cfg.fov;

    float s = 1.0f / cfg.smoothing;
    g_SmoothYaw   += (yawDelta   - g_SmoothYaw)   * s;
    g_SmoothPitch += (pitchDelta - g_SmoothPitch) * s;

    void* exc = nullptr;
    g_SetAimRot(localPlayer, g_SmoothYaw, g_SmoothPitch, &exc);
}

// ── Main aimbot tick (call each frame) ───────────────────
static void AimbotTick(
    uintptr_t localPlayer,
    const std::vector<PlayerEntity>& enemies,
    uintptr_t camera,
    AimbotConfig& cfg
) {
    if (!cfg.enabled) return;

    auto target = FindBestTarget(enemies, camera, cfg);
    if (!target) { g_HasSilentTarget = false; return; }

    if (cfg.silentAim) {
        ApplySilentAim(*target);
    } else if (cfg.legitAim) {
        ApplyLegitAim(localPlayer, *target, cfg);
    } else {
        if (g_SetAimRot) {
            int sw = g_ScreenW ? g_ScreenW() : 1080;
            int sh = g_ScreenH ? g_ScreenH() : 1920;
            float cx = sw * 0.5f, cy = sh * 0.5f;
            float dx = target->screenPos.x - cx;
            float dy = target->screenPos.y - cy;
            float yaw   =  (dx / (float)sw) * cfg.fov;
            float pitch = -(dy / (float)sh) * cfg.fov;
            void* exc = nullptr;
            g_SetAimRot(localPlayer, yaw, pitch, &exc);
        }
    }
}

} // namespace ME
