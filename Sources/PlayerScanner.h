#pragma once
#include "Il2CppHelper.h"
#include "PlayerEntity.h"
#include <vector>
#include <algorithm>

// ═══════════════════════════════════════
//  PlayerScanner
//  Gets Player list via Object.FindObjectsOfType
//  (fastest; no scene graph walk needed)
// ═══════════════════════════════════════

// il2cpp icall: UnityEngine.Object::FindObjectsOfType
using fn_FindObjectsOfType = Il2CppArray* (*)(Il2CppClass* type, void** exc);

namespace ME {

static fn_FindObjectsOfType g_FindObjects = nullptr;
static Il2CppClass*          g_PlayerClass  = nullptr;
static uintptr_t             g_LocalCamera  = 0;
static uint32_t              g_LocalTeamID  = 0xFFFFFFFF;
static uintptr_t             g_LocalPlayer  = 0;

static bool InitScanner() {
    // Resolve Object::FindObjectsOfType icall
    g_FindObjects = (fn_FindObjectsOfType)
        Il2Cpp::API::resolve_icall("UnityEngine.Object::FindObjectsOfType");

    // Get Player class
    g_PlayerClass = Il2Cpp::API::FindClass("COW.GamePlay", "Player");
    if (!g_PlayerClass) {
        // Try without namespace (obfuscated build)
        g_PlayerClass = Il2Cpp::API::FindClass("", "Player");
    }

    return g_FindObjects && g_PlayerClass;
}

// ── Refresh camera via MainCamera ────────────────────────
static void RefreshCamera() {
    // Camera::get_main icall
    using fn_GetMain = uintptr_t (*)(void**);
    static fn_GetMain f = nullptr;
    if (!f) f = (fn_GetMain)
        Il2Cpp::API::resolve_icall("UnityEngine.Camera::get_main");
    if (!f) return;
    void* exc = nullptr;
    g_LocalCamera = f(&exc);
}

// ── Get all players ───────────────────────────────────────
static std::vector<PlayerEntity> GetAllPlayers() {
    std::vector<PlayerEntity> result;
    if (!g_FindObjects || !g_PlayerClass) return result;

    void* exc = nullptr;
    Il2CppArray* arr = g_FindObjects(g_PlayerClass, &exc);
    if (!arr || exc) return result;

    uint32_t count = arr->max_length;
    for (uint32_t i = 0; i < count && i < 64; i++) {
        uintptr_t pptr = (uintptr_t)arr->vector[i];
        if (!pptr) continue;
        PlayerEntity pe(pptr);
        if (pe.valid() && !pe.IsDead()) {
            result.push_back(pe);
        }
    }
    return result;
}

// ── Find local player (same team, not dead, first match) ─
static PlayerEntity GetLocalPlayer(const std::vector<PlayerEntity>& players) {
    // If we already know the local pointer
    if (g_LocalPlayer) return PlayerEntity(g_LocalPlayer);

    // Fallback: IsClientBot==false + smallest HP difference scan is unreliable.
    // Better: use KillCamManager::GetSelfPlayerID (resolve method)
    // For now return first non-bot player
    for (auto& p : players) {
        if (!p.IsBot()) {
            g_LocalPlayer = p.ptr;
            g_LocalTeamID = p.GetTeamID();
            return p;
        }
    }
    return PlayerEntity(0);
}

// ── Get enemies (different team from local) ───────────────
static std::vector<PlayerEntity> GetEnemies(const std::vector<PlayerEntity>& all, uint32_t localTeam) {
    std::vector<PlayerEntity> out;
    for (auto& p : all) {
        if (p.GetTeamID() != localTeam && !p.IsDead()) {
            out.push_back(p);
        }
    }
    return out;
}

} // namespace ME
