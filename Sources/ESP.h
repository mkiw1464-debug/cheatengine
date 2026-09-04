#pragma once
#include "PlayerEntity.h"
#include "PlayerScanner.h"

// ═══════════════════════════════════════
//  ESP — wraps Fl0rk/External_ESP_Free_Fire
//  https://github.com/Fl0rk/External_ESP_Free_Fire
//  Renders via Metal overlay on top of game view
// ═══════════════════════════════════════
// The Fl0rk ESP source provides:
//   - ESPFF::DrawBox(float x, float y, float w, float h, UIColor*)
//   - ESPFF::DrawLine(float x1,y1,x2,y2, UIColor*)
//   - ESPFF::DrawString(float x,y, NSString*, UIColor*)
//   - ESPFF::DrawCircle(float cx,cy,r, UIColor*)
// We call these from our tick.

// Forward declarations matching Fl0rk ESPFF API
#ifdef __OBJC__
#import <UIKit/UIKit.h>
#import <CoreGraphics/CoreGraphics.h>

// Include Fl0rk ESP source (copied into Sources/ from the repo)
// See Sources/ESPFF/ directory
extern "C" {
    void ESPFF_DrawBox(float x, float y, float w, float h, float r, float g, float b, float a, float thickness);
    void ESPFF_DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a, float thickness);
    void ESPFF_DrawFilledRect(float x, float y, float w, float h, float r, float g, float b, float a);
    void ESPFF_DrawString(float x, float y, const char* text, float r, float g, float b, float a, float fontSize);
    void ESPFF_DrawCircle(float cx, float cy, float radius, float r, float g, float b, float a, float thickness);
}
#endif

namespace ME {

struct ESPConfig {
    bool enabled       = false;
    bool showBox       = true;
    bool showSkeleton  = true;
    bool showHP        = true;
    bool showName      = true;
    bool showDistance  = true;
    bool showBotLabel  = false;
    float maxDistance  = 300.0f; // meters
};

// ── Color helpers ─────────────────────────────────────────
struct Color4 { float r,g,b,a; };
static const Color4 kRed    = {1.0f, 0.0f, 0.0f, 1.0f};
static const Color4 kGreen  = {0.0f, 1.0f, 0.0f, 1.0f};
static const Color4 kWhite  = {1.0f, 1.0f, 1.0f, 1.0f};
static const Color4 kYellow = {1.0f, 1.0f, 0.0f, 1.0f};
static const Color4 kCyan   = {0.0f, 1.0f, 1.0f, 1.0f};
static const Color4 kOrange = {1.0f, 0.5f, 0.0f, 1.0f};

// Color by HP ratio
static Color4 HPColor(int hp, int maxhp) {
    if (maxhp <= 0) return kWhite;
    float r = (float)hp / (float)maxhp;
    if (r > 0.6f) return kGreen;
    if (r > 0.3f) return kYellow;
    return kRed;
}

// ── Draw single enemy ESP ─────────────────────────────────
static void DrawEnemy(
    const PlayerEntity& enemy,
    uintptr_t camera,
    const ESPConfig& cfg,
    uintptr_t localPlayer
) {
    // Get head and feet bone positions
    Vector3 headPos, feetPos;
    bool hasHead = enemy.GetBonePosition(BoneTarget::Head,   headPos);
    bool hasFeet = enemy.GetBonePosition(BoneTarget::Pelvis, feetPos);

    if (!hasHead && !hasFeet) return;

    // Fall back to center if partial
    if (!hasHead)  headPos  = enemy.GetCenter();
    if (!hasFeet)  feetPos  = headPos;
    feetPos.y -= 0.3f; // approximate foot offset from pelvis

    auto screenHead = W2S(camera, headPos);
    auto screenFeet = W2S(camera, feetPos);
    if (!screenHead.onScreen && !screenFeet.onScreen) return;

    float sx = screenHead.x, sy = screenHead.y;
    float fx = screenFeet.x, fy = screenFeet.y;

    // Box dimensions from head/feet
    float height = fabsf(fy - sy);
    if (height < 10.0f) height = 40.0f;
    float width  = height * 0.4f;
    float bx     = sx - width * 0.5f;
    float by      = sy;

    int hp    = enemy.GetHP();
    int maxhp = enemy.GetMaxHP();
    auto hpCol = HPColor(hp, maxhp);

    if (cfg.showBox) {
#ifdef __OBJC__
        ESPFF_DrawBox(bx, by, width, height, 1.0f, 0.3f, 0.3f, 1.0f, 1.5f);
#endif
    }

    if (cfg.showHP && maxhp > 0) {
        // HP bar to the left of box
        float barH = height;
        float barX = bx - 6.0f;
        float barY = by;
        float fillH = barH * ((float)hp / (float)maxhp);
#ifdef __OBJC__
        // Background
        ESPFF_DrawFilledRect(barX, barY, 4.0f, barH, 0.1f,0.1f,0.1f,0.7f);
        // Fill
        ESPFF_DrawFilledRect(barX, barY + (barH - fillH), 4.0f, fillH,
                             hpCol.r, hpCol.g, hpCol.b, hpCol.a);
#endif
    }

    if (cfg.showName) {
        auto name = enemy.GetName();
#ifdef __OBJC__
        ESPFF_DrawString(sx, sy - 16.0f, name.c_str(), 1.0f,1.0f,1.0f,1.0f, 12.0f);
#endif
    }

    if (cfg.showSkeleton) {
        // Draw skeleton: head → neck → spine → pelvis, L/R legs
        static const BoneTarget chain[] = {
            BoneTarget::Head, BoneTarget::Neck, BoneTarget::Body, BoneTarget::Pelvis,
            BoneTarget::LLeg, BoneTarget::RLeg
        };
        Vector3 prev; bool hasPrev = false;
        for (int i = 0; i < 4; i++) {
            Vector3 cur;
            if (!enemy.GetBonePosition(chain[i], cur)) { hasPrev=false; continue; }
            auto sc = W2S(camera, cur);
            if (!sc.onScreen) { hasPrev=false; continue; }
            if (hasPrev) {
                auto sp = W2S(camera, prev);
#ifdef __OBJC__
                ESPFF_DrawLine(sp.x, sp.y, sc.x, sc.y, 0.0f,1.0f,1.0f,0.8f, 1.0f);
#endif
            }
            prev = cur; hasPrev = true;
        }
        // Legs from pelvis
        Vector3 pelvis, lleg, rleg;
        auto sp = W2S(camera, pelvis);
        if (enemy.GetBonePosition(BoneTarget::Pelvis, pelvis) &&
            enemy.GetBonePosition(BoneTarget::LLeg, lleg)) {
            auto sl = W2S(camera, lleg);
#ifdef __OBJC__
            ESPFF_DrawLine(sp.x,sp.y, sl.x,sl.y, 0.0f,1.0f,1.0f,0.8f, 1.0f);
#endif
        }
        if (enemy.GetBonePosition(BoneTarget::Pelvis, pelvis) &&
            enemy.GetBonePosition(BoneTarget::RLeg, rleg)) {
            auto sr = W2S(camera, rleg);
#ifdef __OBJC__
            ESPFF_DrawLine(sp.x,sp.y, sr.x,sr.y, 0.0f,1.0f,1.0f,0.8f, 1.0f);
#endif
        }
    }

    if (cfg.showDistance) {
        // Get local player center for distance
        Vector3 mypos = {0,0,0};
        if (localPlayer) PlayerEntity(localPlayer).GetBonePosition(BoneTarget::Body, mypos);
        Vector3 epos = enemy.GetCenter();
        float dist = Distance3D(mypos, epos);

        char buf[32];
        snprintf(buf, sizeof(buf), "%.0fm", dist);
#ifdef __OBJC__
        ESPFF_DrawString(sx, fy + 4.0f, buf, 0.9f,0.9f,0.0f,1.0f, 11.0f);
#endif
    }
}

// ── ESP tick ──────────────────────────────────────────────
static void ESPTick(
    const std::vector<PlayerEntity>& enemies,
    uintptr_t camera,
    const ESPConfig& cfg,
    uintptr_t localPlayer
) {
    if (!cfg.enabled) return;
    for (auto& e : enemies) {
        DrawEnemy(e, camera, cfg, localPlayer);
    }
}

} // namespace ME
