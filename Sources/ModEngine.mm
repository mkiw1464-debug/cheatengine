// ═══════════════════════════════════════
//  ModEngine.mm — dylib constructor + main tick
//  Free Fire iOS OB54
//  Inject via: GBox / AppSigner / Sideload IPA
// ═══════════════════════════════════════

#include "Il2CppHelper.h"
#include "PlayerScanner.h"
#include "Aimbot.h"
#include "ESP.h"
#include "Bypass.h"

extern "C" {
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
}

extern void ME_SetupGesture();
extern void ME_SetStreamProof(bool);

// ── Global configs ────────────────────────────────────────
ME::AimbotConfig g_AimbotCfg;
ME::ESPConfig    g_ESPCfg;
bool             g_StreamProof = false;

// ── Game tick ─────────────────────────────────────────────
static void GameTick() {
    using namespace ME;

    // Refresh camera once per tick
    RefreshCamera();

    // Get all players
    auto allPlayers = GetAllPlayers();
    if (allPlayers.empty()) return;

    // Identify local player
    auto localPE = GetLocalPlayer(allPlayers);
    uintptr_t localPtr = localPE.ptr;

    // Get enemies
    auto enemies = GetEnemies(allPlayers, g_LocalTeamID);

    // Aimbot
    AimbotTick(localPtr, enemies, g_LocalCamera, g_AimbotCfg);

    // ESP
    ESPTick(enemies, g_LocalCamera, g_ESPCfg, localPtr);
}

// ── CADisplayLink tick (main thread, 60fps) ───────────────
@interface MEDisplayLinkTarget : NSObject
+ (void)start;
@end
@implementation MEDisplayLinkTarget
+ (void)start {
    CADisplayLink* link = [CADisplayLink
        displayLinkWithTarget:self selector:@selector(tick:)];
    link.preferredFramesPerSecond = 60;
    [link addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}
+ (void)tick:(CADisplayLink*)dl {
    GameTick();
}
@end

// ── Async init (avoid blocking injector) ─────────────────
static void InitModEngine() {
    // Small delay to let game fully load il2cpp
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3.0 * NSEC_PER_SEC)),
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{

        // 1. Bypass first — before anything else
        InitBypass();

        // 2. Init il2cpp API
        if (!Il2Cpp::API::Init()) {
            NSLog(@"[ModEngine] il2cpp API init failed");
            return;
        }

        // 3. Init native function pointers
        if (!ME::InitFunctions()) {
            NSLog(@"[ModEngine] InitFunctions failed");
            return;
        }

        // 4. Init player scanner
        if (!ME::InitScanner()) {
            NSLog(@"[ModEngine] InitScanner failed — will retry");
            // Scanner may fail before game scene loads, retry handled in tick
        }

        // 5. Setup menu triple-tap gesture
        dispatch_async(dispatch_get_main_queue(), ^{
            ME_SetupGesture();
        });

        // 6. Start game tick on main thread
        dispatch_async(dispatch_get_main_queue(), ^{
            [MEDisplayLinkTarget start];
        });

        NSLog(@"[ModEngine] OB54 initialized — ModEngine live");
    });
}

// ── Dylib constructor ─────────────────────────────────────
__attribute__((constructor))
static void ModEngineConstructor() {
    InitModEngine();
}

// ── Dylib destructor ──────────────────────────────────────
__attribute__((destructor))
static void ModEngineDestructor() {
    NSLog(@"[ModEngine] unloaded");
}
