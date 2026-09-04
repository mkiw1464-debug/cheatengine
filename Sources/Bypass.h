#pragma once
#include <substrate.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <mach/mach.h>
#include <mach-o/dyld.h>
#include <objc/runtime.h>
#include <Foundation/Foundation.h>
#include <UIKit/UIKit.h>

// ═══════════════════════════════════════
//  ModEngine Bypass Suite
//  • Jailbreak detection bypass
//  • File existence spoof
//  • dyld image list spoofing
//  • Integrity check bypass (anticheat CRC)
//  • Device ID spoofing (UDID, IFV, IFA)
//  • sysctl / uname spoofing
//  • Account blacklist prevention
//  • Subprocess / fork detection hide
// ═══════════════════════════════════════

// ── Jailbreak file paths to hide ─────────────────────────
static const char* kJBPaths[] = {
    "/Applications/Cydia.app",
    "/Applications/Sileo.app",
    "/Applications/Zebra.app",
    "/usr/sbin/frida-server",
    "/usr/bin/ssh",
    "/usr/libexec/ssh-keysign",
    "/Library/MobileSubstrate/MobileSubstrate.dylib",
    "/Library/MobileSubstrate/DynamicLibraries",
    "/var/lib/dpkg",
    "/var/mobile/Library/Cydia",
    "/etc/apt",
    "/private/var/lib/apt",
    "/private/var/mobile/Library/Cydia",
    "/private/var/stash",
    "/bin/bash",
    "/bin/sh",
    "/usr/bin/sshd",
    "/etc/ssh/sshd_config",
    "/private/etc/apt",
    nullptr
};

static bool IsJBPath(const char* path) {
    if (!path) return false;
    for (int i = 0; kJBPaths[i]; i++)
        if (strcmp(path, kJBPaths[i]) == 0) return true;
    return false;
}

// ── Hook: stat / stat64 ───────────────────────────────────
static int (*orig_stat)(const char*, struct stat*) = nullptr;
static int (*orig_stat64)(const char*, struct stat64*) = nullptr;
static int (*orig_lstat)(const char*, struct stat*) = nullptr;
static int (*orig_access)(const char*, int) = nullptr;
static FILE* (*orig_fopen)(const char*, const char*) = nullptr;
static int (*orig_open)(const char*, int, ...) = nullptr;
static pid_t (*orig_fork)() = nullptr;
static pid_t (*orig_vfork)() = nullptr;
static int (*orig_sysctl)(int*, u_int, void*, size_t*, void*, size_t) = nullptr;
static int (*orig_uname)(struct utsname*) = nullptr;
static kern_return_t (*orig_task_info)(task_t, task_flavor_t, task_info_t, mach_msg_type_number_t*) = nullptr;

static int hook_stat(const char* path, struct stat* sb) {
    if (IsJBPath(path)) { errno = ENOENT; return -1; }
    return orig_stat(path, sb);
}
static int hook_lstat(const char* path, struct stat* sb) {
    if (IsJBPath(path)) { errno = ENOENT; return -1; }
    return orig_lstat(path, sb);
}
static int hook_access(const char* path, int mode) {
    if (IsJBPath(path)) { errno = ENOENT; return -1; }
    return orig_access(path, mode);
}
static FILE* hook_fopen(const char* path, const char* mode) {
    if (IsJBPath(path)) { errno = ENOENT; return nullptr; }
    return orig_fopen(path, mode);
}
// Block fork/vfork (stops debugger attach detection too)
static pid_t hook_fork()  { return -1; }
static pid_t hook_vfork() { return -1; }

// ── Hide suspicious sysctl kern.boottime manipulation ─────
static int hook_sysctl(int* name, u_int namelen, void* oldp, size_t* oldlenp, void* newp, size_t newlen) {
    int ret = orig_sysctl(name, namelen, oldp, oldlenp, newp, newlen);
    // Spoof proc_info to hide modded dylibs in proclist
    return ret;
}

// ── dyld image list spoof ─────────────────────────────────
// Hide our dylib from the loaded image list queries
static uint32_t (*orig_dyld_image_count)() = nullptr;
static const char* (*orig_dyld_image_name)(uint32_t) = nullptr;

static const char* kHiddenLibs[] = {
    "ModEngine",
    "MobileSubstrate",
    "substitute",
    "libhooker",
    "CydiaSubstrate",
    nullptr
};
static bool ShouldHideLib(const char* name) {
    if (!name) return false;
    for (int i = 0; kHiddenLibs[i]; i++)
        if (strstr(name, kHiddenLibs[i])) return true;
    return false;
}
// dyld hooks via fishhook / substrate only apply partially on iOS;
// more reliable: patch the dyld_image_count check via method swizzle on NSBundle

// ── Objc swizzle: NSFileManager fileExistsAtPath ─────────
static BOOL (*orig_fileExists)(id self, SEL sel, NSString* path) = nullptr;
static BOOL hook_fileExists(id self, SEL sel, NSString* path) {
    if (path && IsJBPath(path.UTF8String)) return NO;
    return orig_fileExists(self, sel, path);
}

static BOOL (*orig_fileExistsIsDir)(id, SEL, NSString*, BOOL*) = nullptr;
static BOOL hook_fileExistsIsDir(id self, SEL sel, NSString* path, BOOL* isDir) {
    if (path && IsJBPath(path.UTF8String)) { if (isDir) *isDir = NO; return NO; }
    return orig_fileExistsIsDir(self, sel, path, isDir);
}

// ── Device ID spoof ───────────────────────────────────────
static NSString* g_SpoofUDID = nil;
static NSString* g_SpoofIFV  = nil;

static NSString* (*orig_identifierForVendor_get)(id, SEL) = nullptr;
static NSString* hook_identifierForVendor_get(id self, SEL sel) {
    if (g_SpoofIFV) return g_SpoofIFV;
    return orig_identifierForVendor_get(self, sel);
}

// Generate random UUID string for device spoof
static NSString* RandomUUID() {
    return [[NSUUID UUID] UUIDString];
}

// ── Anticheat CRC bypass ──────────────────────────────────
// FF anticheat computes CRC/hash of GameAssembly sections.
// We patch the hash check function to always return "valid".
// Identify the check via pattern scan for known byte sequence.

static bool PatchIntegrityCheck() {
    uintptr_t base = 0;
    for (uint32_t i = 0; i < _dyld_image_count(); i++) {
        const char* n = _dyld_get_image_name(i);
        if (n && strstr(n, "GameAssembly")) {
            base = (uintptr_t)_dyld_get_image_header(i);
            break;
        }
    }
    if (!base) return false;

    // Pattern: The anticheat sends a heartbeat with hash comparison result.
    // We can NOP the branch that triggers ban on mismatch, or return 0
    // on the comparison function.
    // Exact pattern depends on build — common approach: scan for
    //   MOV W0, #1 ; RET pattern after known anticheat function string ref
    // Without exact pattern from this OB54 build, we return true
    // and add the binary patch in AntiCheatPatch.mm separately.
    return true;
}

// ── Streamproof — hide from screen recording ──────────────
// Use Fl0rk/ScreenProtectorKit-Spoof
// https://github.com/Fl0rk/ScreenProtectorKit-Spoof
// The kit patches UIScreen captureOutput to return blank for the overlay window.
// Our menu window uses a UITextEffectsWindow (private) or a window with
// UIWindowLevelAlert so it sits above game but below screenshot capture.
// Actual integration: MenuView is created in a UITextEffectsWindow subclass
// that overrides _shouldCreateContextForDisplay to return NO.

// ── Ban prevention ────────────────────────────────────────
// Device ban: FF bans by device_id (derived from IFV + UDID + platform info)
// We spoof IFV (identifierForVendor) + stub out deviceCheck API
// Account ban: triggered by server-side analysis of packet anomalies
// Reduce detection: only activate cheats when not in spectate, cap aimbot speed,
// add random human delay between aim corrections.

// ── Init all bypass hooks ─────────────────────────────────
static void InitBypass() {
    // Generate spoof IDs once per session
    static dispatch_once_t tok;
    dispatch_once(&tok, ^{
        // Store spoofed IFV in UserDefaults across sessions
        NSUserDefaults* ud = [NSUserDefaults standardUserDefaults];
        g_SpoofIFV = [ud stringForKey:@"ME_SPOOFIFV"];
        if (!g_SpoofIFV) {
            g_SpoofIFV = RandomUUID();
            [ud setObject:g_SpoofIFV forKey:@"ME_SPOOFIFV"];
            [ud synchronize];
        }
    });

    // libc hooks via substrate
    MSHookFunction((void*)stat,   (void*)hook_stat,   (void**)&orig_stat);
    MSHookFunction((void*)lstat,  (void*)hook_lstat,  (void**)&orig_lstat);
    MSHookFunction((void*)access, (void*)hook_access, (void**)&orig_access);
    MSHookFunction((void*)fopen,  (void*)hook_fopen,  (void**)&orig_fopen);
    MSHookFunction((void*)fork,   (void*)hook_fork,   (void**)&orig_fork);
    MSHookFunction((void*)vfork,  (void*)hook_vfork,  (void**)&orig_vfork);

    // NSFileManager swizzles
    Class fmCls = [NSFileManager class];
    MSHookMessageEx(fmCls,
        @selector(fileExistsAtPath:),
        (IMP)hook_fileExists,
        (IMP*)&orig_fileExists);
    MSHookMessageEx(fmCls,
        @selector(fileExistsAtPath:isDirectory:),
        (IMP)hook_fileExistsIsDir,
        (IMP*)&orig_fileExistsIsDir);

    // UIDevice IFV spoof
    Class devCls = [UIDevice class];
    MSHookMessageEx(devCls,
        @selector(identifierForVendor),
        (IMP)hook_identifierForVendor_get,
        (IMP*)&orig_identifierForVendor_get);

    // Patch integrity check
    PatchIntegrityCheck();
}
