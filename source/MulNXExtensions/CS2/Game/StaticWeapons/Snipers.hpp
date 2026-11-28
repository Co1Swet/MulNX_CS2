#pragma once
#include "StaticWeaponBase.hpp"

namespace CS2 {
    namespace StaticWeapons {
        struct AWP : Weapon<AWP> {
            static constexpr std::string_view kCanonical = "awp";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Sniper;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
        };
        struct SSG08 : Weapon<SSG08> {
            static constexpr std::string_view kCanonical = "ssg08";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Sniper;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
        };
        struct SCAR20 : Weapon<SCAR20> {
            static constexpr std::string_view kCanonical = "scar20";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Sniper;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
        };
        struct G3SG1 : Weapon<G3SG1> {
            static constexpr std::string_view kCanonical = "g3sg1";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Sniper;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
        };
    }
}