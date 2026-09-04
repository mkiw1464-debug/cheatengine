#pragma once
#include <stdint.h>

// ═══════════════════════════════════════
//  ModEngine — OB54 Offsets (iOS arm64)
//  Extracted from dump.cs
// ═══════════════════════════════════════

// ── Unity mono/il2cpp base ──────────────────────────────
// il2cpp base is resolved at runtime via dyld

// ── Player class (COW.GamePlay.Player) ──────────────────
// TypeDefIndex: 30884 — Player extends AttackableEntity
#define OFF_PLAYER_ISKNOCKEDDOWN        0xA8   // bool IsFrozenKnockDown
#define OFF_PLAYER_SPEED                0x4F8  // float Speed
#define OFF_PLAYER_TEAMMODEID           0x3CC  // uint TeamModeID
#define OFF_PLAYER_ISLOCAL              0x3D0  // bool IsShowEquip (proxy — use GetBoneTransform)
#define OFF_PLAYER_ISINVEHICLE          0x490  // bool m_GetInVehicle
#define OFF_PLAYER_ORIGINALNAME         0x430  // string OriginalNickName
#define OFF_PLAYER_ISCLIENTBOT          0x438  // bool IsClientBot
#define OFF_PLAYER_MAINCAMERATRANS      0x380  // Transform MainCameraTransform
#define OFF_PLAYER_KNOCKDOWNDMGPERSEC   0x344  // uint m_KnockDownDamagePerSec
// PropertyData (for HP read via EPRI_PLAYER_CUR_HP=0, EPRI_PLAYER_MAX_HP=1)
#define OFF_PLAYER_PROPDATA             0x720  // PropertyData AJBLKLFDNHC

// ── AttackableEntity ─────────────────────────────────────
// TypeDefIndex: 30639
// IsDead via get_IsDead() RVA: 0x53799CC

// ── PropertyData (HP system) ─────────────────────────────
// EPRI_PLAYER_CUR_HP  = 0
// EPRI_PLAYER_MAX_HP  = 1
// EPRI_PLAYER_STATUS  = 11
// Access via PropertyData array, index * 4 bytes (int array base)
// PropertyData::m_Props array — read int at index
#define EPRI_CUR_HP    0
#define EPRI_MAX_HP    1
#define EPRI_STATUS    11

// ── PlayerAttributes ─────────────────────────────────────
// TypeDefIndex: 33340
// Attached component on Player @ OFF_PLAYER_KDJHNBAECLM
#define OFF_PLAYER_PLAYERATTR           0x700  // PlayerAttributes KDJHNBAECLM

// ── Bone names used with GetBoneTransform(string) ────────
// RVA: 0x594D978 — Player::GetBoneTransform
#define BONE_NAME_HEAD     "Bip001 Head"
#define BONE_NAME_NECK     "Bip001 Neck"
#define BONE_NAME_SPINE    "Bip001 Spine2"
#define BONE_NAME_PELVIS   "Bip001 Pelvis"
#define BONE_NAME_LTHIGH   "Bip001 L Thigh"
#define BONE_NAME_RTHIGH   "Bip001 R Thigh"
#define BONE_NAME_LCALF    "Bip001 L Calf"
#define BONE_NAME_RCALF    "Bip001 R Calf"

// ── DAPOIBANHEH (bone transform node) ────────────────────
// TypeDefIndex: 30773
#define OFF_BONENODE_TRANSFORM          0x10   // Transform <IKLHANPPCGM>k__BackingField

// ── KillCamManager (singleton, has GetSelfPlayerID) ──────
// TypeDefIndex: 12167 / SingletonModule<KillCamManager>
// GetSelfPlayerID() RVA: (resolve via idmap 19927)
// Use to check which Player* is local self

// ── Unity Engine offsets (arm64 iOS, Unity 2021.x) ───────
#define OFF_TRANSFORM_POS               0x90   // UnityEngine.Transform localPosition
#define OFF_TRANSFORM_WORLDPOS          0x90   // via get_position()
#define OFF_GAMEOBJ_TRANSFORM           0x10   // GameObject->Transform

// ── Camera WorldToScreen ──────────────────────────────────
// Use Camera::WorldToScreenPoint native binding
// il2cpp_resolve_icall("UnityEngine.Camera::WorldToScreenPoint_Injected")

// ── Screen dimensions ─────────────────────────────────────
// Screen::get_width / Screen::get_height via icall

// ── Il2Cpp Class/Method resolution helpers ───────────────
// Resolved at runtime via il2cpp_class_find / il2cpp_method_get_from_name
