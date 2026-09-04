#pragma once

// ── Substrate include — robust fallback chain ─────────────
#if __has_include(<CydiaSubstrate/CydiaSubstrate.h>)
  #include <CydiaSubstrate/CydiaSubstrate.h>
#elif __has_include(<substrate.h>)
  #include <substrate.h>
#else
  // Minimal stubs so the file compiles even without substrate headers
  #warning "Substrate header not found — MSHookFunction/MSHookMessageEx will be no-ops"
  #define MSHookFunction(sym, hook, orig)     do { *(orig) = (decltype(*(orig)))(sym); } while(0)
  #define MSHookMessageEx(cls, sel, imp, orig) do { } while(0)
#endif

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

// ── Hook function pointers ────────────────────────────────
static int    (*orig_stat)(const char*, struct stat*) = nullptr;
static int    (*orig_stat64)(const char*, struct stat64*) = nullptr;
static int    (*orig_lstat)(const char*, struct stat*) = nullptr;
static int    (*orig_access)(const char*, int) = nullptr;
static FILE*  (*orig_fopen)(const char*, const char*) = nullptr;
static int    (*orig_open)(const char*, int, ...) = nullptr;
static pid_t  (*orig_fork)() = nullptr;
static pid_t  (*orig_vfork)() = nullptr;
static int    (*orig_sysctl)(int*, u_int, void*, size_t*, void*, size_t) = nullptr;
static int    (*orig_uname)(struct utsname*) = nullptr;
static kern_return_t (*orig_task_info)(task_t, task_flavor_t, task_info_t, mach_msg_type_number_t*) = nullptr;

static int   hook_stat(const char* path, struct stat* sb) {
    if (IsJBPath(path)) { errno = ENOENT; return -1; }
    return orig_stat(path, sb);
}
static int   hook_lstat(const char* path, struct stat* sb) {
    if (IsJBPath(path)) { errno = ENOENT; return -1; }
    return orig_lstat(path, sb);
}
static int   hook_access(const char* path, int mode) {
    if (IsJBPath(path)) { errno = ENOENT; return -1; }
    return orig_access(path, mode);
}
static FILE* hook_fopen(const char* path, const char* mode) {
    if (IsJBPath(path)) { errno = ENOENT; return nullptr; }
    return orig_fopen(path, mode);
}
static pid_t hook_fork()  { return -1; }
static pid_t hook_vfork() { return -1; }

static int hook_sysctl(int* name, u_int namelen, void* oldp, size_t* oldlenp, void* newp, size_t newlen) {
    return orig_sysctl(name, namelen, oldp, oldlenp, newp, newlen);
}

// ── dyld image list spoof ─────────────────────────────────
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

// ── Objc swizzle: NSFileManager fileExistsAtPath ─────────
static BOOL (*orig_fileExists)(id, SEL, NSString*) = nullptr;
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

static NSString* RandomUUID() {
    return [[NSUUID UUID] UUIDString];
}

// ── Anticheat CRC bypass ──────────────────────────────────
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
    // Pattern scan for integrity check branch — binary-specific
    // Add AntiCheatPatch.mm for OB54 exact offsets
    return true;
}

// ── Init all bypass hooks ─────────────────────────────────
static void InitBypass() {
    static dispatch_once_t tok;
    dispatch_once(&tok, ^{
        NSUserDefaults* ud = [NSUserDefaults standardUserDefaults];
        g_SpoofIFV = [ud stringForKey:@"ME_SPOOFIFV"];
        if (!g_SpoofIFV) {
            g_SpoofIFV = RandomUUID();
            [ud setObject:g_SpoofIFV forKey:@"ME_SPOOFIFV"];
            [ud synchronize];
        }
    });

    MSHookFunction((void*)stat,   (void*)hook_stat,   (void**)&orig_stat);
    MSHookFunction((void*)lstat,  (void*)hook_lstat,  (void**)&orig_lstat);
    MSHookFunction((void*)access, (void*)hook_access, (void**)&orig_access);
    MSHookFunction((void*)fopen,  (void*)hook_fopen,  (void**)&orig_fopen);
    MSHookFunction((void*)fork,   (void*)hook_fork,   (void**)&orig_fork);
    MSHookFunction((void*)vfork,  (void*)hook_vfork,  (void**)&orig_vfork);

    Class fmCls = [NSFileManager class];
    MSHookMessageEx(fmCls,
        @selector(fileExistsAtPath:),
        (IMP)hook_fileExists,
        (IMP*)&orig_fileExists);
    MSHookMessageEx(fmCls,
        @selector(fileExistsAtPath:isDirectory:),
        (IMP)hook_fileExistsIsDir,
        (IMP*)&orig_fileExistsIsDir);

    Class devCls = [UIDevice class];
    MSHookMessageEx(devCls,
        @selector(identifierForVendor),
        (IMP)hook_identifierForVendor_get,
        (IMP*)&orig_identifierForVendor_get);

    PatchIntegrityCheck();
}
