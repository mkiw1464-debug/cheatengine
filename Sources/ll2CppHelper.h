#pragma once
#include <stdint.h>
#include <dlfcn.h>
#include <string>
#include <vector>
#include <mach-o/dyld.h>
#include <objc/runtime.h>

// ═══════════════════════════════════════
//  Il2CppHelper — runtime type resolution
// ═══════════════════════════════════════

// ── Il2Cpp runtime types ─────────────────────────────────
struct Il2CppDomain;
struct Il2CppAssembly;
struct Il2CppImage;
struct Il2CppClass;
struct Il2CppObject { Il2CppClass* klass; void* monitor; };
struct Il2CppString { Il2CppObject obj; int32_t length; uint16_t chars[1]; };
struct Il2CppArray  { Il2CppObject obj; void* bounds; uint32_t max_length; void* vector[1]; };

// ── Il2Cpp exports ───────────────────────────────────────
typedef Il2CppDomain*   (*t_il2cpp_domain_get)();
typedef Il2CppAssembly* (*t_il2cpp_domain_assembly_open)(Il2CppDomain*, const char*);
typedef Il2CppImage*    (*t_il2cpp_assembly_get_image)(Il2CppAssembly*);
typedef Il2CppClass*    (*t_il2cpp_class_from_name)(Il2CppImage*, const char*, const char*);
typedef void*           (*t_il2cpp_class_get_method_from_name)(Il2CppClass*, const char*, int);
typedef void*           (*t_il2cpp_resolve_icall)(const char*);
typedef Il2CppString*   (*t_il2cpp_string_new)(const char*);
typedef void*           (*t_il2cpp_runtime_invoke)(void*, Il2CppObject*, void**, void**);
typedef void*           (*t_il2cpp_object_new)(Il2CppClass*);
typedef Il2CppClass*    (*t_il2cpp_object_get_class)(Il2CppObject*);

// ── Singleton function exports ────────────────────────────
namespace Il2Cpp {

static uintptr_t s_GameBase = 0;

static uintptr_t GetGameBase() {
    if (s_GameBase) return s_GameBase;
    for (uint32_t i = 0; i < _dyld_image_count(); i++) {
        const char* name = _dyld_get_image_name(i);
        if (name && strstr(name, "GameAssembly")) {
            s_GameBase = (uintptr_t)_dyld_get_image_header(i);
            return s_GameBase;
        }
    }
    return 0;
}

// Read pointer at (base + rva)
template<typename T>
static T Read(uintptr_t addr) {
    return *reinterpret_cast<T*>(addr);
}

// Resolve il2cpp export by name from libil2cpp / GameAssembly
template<typename Fn>
static Fn Resolve(const char* sym) {
    static void* handle = nullptr;
    if (!handle) handle = dlopen("@rpath/GameAssembly.dylib", RTLD_NOLOAD);
    if (!handle) handle = dlopen(nullptr, RTLD_NOLOAD);
    return (Fn)dlsym(handle, sym);
}

// RVA → VA
static uintptr_t RVA(uintptr_t rva) { return GetGameBase() + rva; }

// Read absolute VA for a method — dump gives VA directly, subtract base then add actual base
static uintptr_t VA(uintptr_t dumpVA) {
    // dump was produced on a device; VAs = base + RVA
    // We need to subtract the dump base. In practice VA == offset for ASLR patched
    // For most FF ios dumps, VA == Offset. Use Offset field.
    return RVA(dumpVA);
}

// ── Il2Cpp API wrappers ──────────────────────────────────
namespace API {
    static t_il2cpp_domain_get               domain_get;
    static t_il2cpp_domain_assembly_open     domain_assembly_open;
    static t_il2cpp_assembly_get_image       assembly_get_image;
    static t_il2cpp_class_from_name          class_from_name;
    static t_il2cpp_class_get_method_from_name class_get_method_from_name;
    static t_il2cpp_resolve_icall            resolve_icall;
    static t_il2cpp_string_new               string_new;
    static t_il2cpp_runtime_invoke           runtime_invoke;

    static bool Init() {
        domain_get               = Resolve<t_il2cpp_domain_get>("il2cpp_domain_get");
        domain_assembly_open     = Resolve<t_il2cpp_domain_assembly_open>("il2cpp_domain_assembly_open");
        assembly_get_image       = Resolve<t_il2cpp_assembly_get_image>("il2cpp_assembly_get_image");
        class_from_name          = Resolve<t_il2cpp_class_from_name>("il2cpp_class_from_name");
        class_get_method_from_name = Resolve<t_il2cpp_class_get_method_from_name>("il2cpp_class_get_method_from_name");
        resolve_icall            = Resolve<t_il2cpp_resolve_icall>("il2cpp_resolve_icall");
        string_new               = Resolve<t_il2cpp_string_new>("il2cpp_string_new");
        runtime_invoke           = Resolve<t_il2cpp_runtime_invoke>("il2cpp_runtime_invoke");
        return domain_get != nullptr;
    }

    static Il2CppImage* GetAssemblyCSharp() {
        static Il2CppImage* img = nullptr;
        if (img) return img;
        auto* dom = domain_get();
        if (!dom) return nullptr;
        auto* asm_ = domain_assembly_open(dom, "Assembly-CSharp");
        if (!asm_) return nullptr;
        img = assembly_get_image(asm_);
        return img;
    }

    static Il2CppClass* FindClass(const char* ns, const char* name) {
        auto* img = GetAssemblyCSharp();
        if (!img) return nullptr;
        return class_from_name(img, ns, name);
    }

    static Il2CppString* NewString(const char* s) {
        return string_new(s);
    }
}

// ── Memory read helpers ───────────────────────────────────
static uintptr_t ReadPtr(uintptr_t base, ptrdiff_t off) {
    if (!base) return 0;
    return *(uintptr_t*)(base + off);
}

static std::string Il2CppStringToStd(Il2CppString* s) {
    if (!s) return "";
    std::string out;
    for (int i = 0; i < s->length; i++)
        if (s->chars[i] < 128) out += (char)s->chars[i];
    return out;
}

} // namespace Il2Cpp
