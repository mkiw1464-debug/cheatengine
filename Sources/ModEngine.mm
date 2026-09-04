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

    RefreshCamera();

    auto allPlayers = GetAllPlayers();
    if (allPlayers.empty()) return;

    auto localPE    = GetLocalPlayer(allPlayers);
    uintptr_t localPtr = localPE.ptr;

    auto enemies = GetEnemies(allPlayers, g_LocalTeamID);

    AimbotTick(localPtr, enemies, g_LocalCamera, g_AimbotCfg);
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

// ── Async init ────────────────────────────────────────────
static void InitModEngine() {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3.0 * NSEC_PER_SEC)),
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{

        InitBypass();

        if (!Il2Cpp::API::Init()) {
            NSLog(@"[ModEngine] il2cpp API init failed");
            return;
        }

        if (!ME::InitFunctions()) {
            NSLog(@"[ModEngine] InitFunctions failed");
            return;
        }

        if (!ME::InitScanner()) {
            NSLog(@"[ModEngine] InitScanner failed — will retry");
        }

        dispatch_async(dispatch_get_main_queue(), ^{
            ME_SetupGesture();
        });

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
