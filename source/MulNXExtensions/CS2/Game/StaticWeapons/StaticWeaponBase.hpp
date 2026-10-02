#pragma once
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace CS2 {
    enum DamageTypes_t : uint32_t {
        DMG_GENERIC = 0x0,
        DMG_CRUSH = 0x1,
        DMG_BULLET = 0x2,
        DMG_SLASH = 0x4,
        DMG_BURN = 0x8,
        DMG_VEHICLE = 0x10,
        DMG_FALL = 0x20,
        DMG_BLAST = 0x40,
        DMG_CLUB = 0x80,
        DMG_SHOCK = 0x100,
        DMG_SONIC = 0x200,
        DMG_ENERGYBEAM = 0x400,
        DMG_BUCKSHOT = 0x800,
        DMG_BLAST_SURFACE = 0x1000,
        DMG_DISSOLVE = 0x2000,
        DMG_DROWN = 0x4000,
        DMG_POISON = 0x8000,
        DMG_RADIATION = 0x10000,
        DMG_DROWNRECOVER = 0x20000,
        DMG_ACID = 0x40000,
        DMG_LASTGENERICFLAG = 0x40000,
        DMG_HEADSHOT = 0x80000
    };
    enum class EKillTypes_t : uint8_t {
        KILL_NONE = 0x0,
        KILL_DEFAULT = 0x1,
        KILL_HEADSHOT = 0x2,
        KILL_BLAST = 0x3,
        KILL_BURN = 0x4,
        KILL_SLASH = 0x5,
        KILL_SHOCK = 0x6,
        KILLTYPE_COUNT = 0x7
    };

    // 优先级：BLAST > BURN > SLASH > HEADSHOT > SHOCK > DEFAULT，来自server.dll
    constexpr EKillTypes_t TranslateKillType(DamageTypes_t dt) noexcept {
        if (dt & DamageTypes_t::DMG_BLAST)    return EKillTypes_t::KILL_BLAST;
        if (dt & DamageTypes_t::DMG_BURN)     return EKillTypes_t::KILL_BURN;
        if (dt & DamageTypes_t::DMG_SLASH)    return EKillTypes_t::KILL_SLASH;
        if (dt & DamageTypes_t::DMG_HEADSHOT) return EKillTypes_t::KILL_HEADSHOT;
        if (dt & DamageTypes_t::DMG_SHOCK)    return EKillTypes_t::KILL_SHOCK;
        return EKillTypes_t::KILL_DEFAULT;
    }

    // 分类
    enum class WeaponClass : uint8_t {
        Rifle, SMG, Pistol, Sniper, Shotgun, MG,
        Knife, Grenade, Taser, C4, Special,
    };
    enum class GrenadeKind : uint8_t {
        None, Smoke, Flash, HE, Molotov, Incendiary, Decoy,
    };

    // 描述符：运行时查找用的 POD 载体
    struct WeaponDescriptor {
        std::string_view canonical;
        DamageTypes_t damageType;
        WeaponClass cls;
        GrenadeKind grenade;
        int baseReward;
    };

    // CRTP 基类：所有通用函数在此实现一次
    template<typename Derived>
    struct Weapon {
        static constexpr std::string_view canonical()  noexcept { return Derived::kCanonical; }
        static constexpr DamageTypes_t    damageType() noexcept { return static_cast<DamageTypes_t>(Derived::kDamageType); }
        static constexpr WeaponClass      cls()        noexcept { return Derived::kClass; }
        static constexpr GrenadeKind      grenade()    noexcept { return Derived::kGrenade; }
        static constexpr int              baseReward() noexcept { return Derived::kBaseReward; }

        // 导出为运行时描述符
        static constexpr WeaponDescriptor descriptor() noexcept {
            return WeaponDescriptor{ canonical(), damageType(), cls(), grenade(), baseReward() };
        }
    };
}