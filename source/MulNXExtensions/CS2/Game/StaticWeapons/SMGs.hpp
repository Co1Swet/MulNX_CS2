#pragma once
#include "StaticWeaponBase.hpp"

namespace CS2 {
    namespace StaticWeapons {
        struct MP9 : Weapon<MP9> {
            static constexpr std::string_view kCanonical = "mp9";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::SMG;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 600;
        };
        struct MAC10 : Weapon<MAC10> {
            static constexpr std::string_view kCanonical = "mac10";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::SMG;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 600;
        };
        struct MP7 : Weapon<MP7> {
            static constexpr std::string_view kCanonical = "mp7";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::SMG;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 600;
        };
        struct MP5SD : Weapon<MP5SD> {
            static constexpr std::string_view kCanonical = "mp5sd";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::SMG;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 600;
        };
        struct UMP45 : Weapon<UMP45> {
            static constexpr std::string_view kCanonical = "ump45";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::SMG;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 600;
        };
        struct Bizon : Weapon<Bizon> {
            static constexpr std::string_view kCanonical = "bizon";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::SMG;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 600;
        };
        struct P90 : Weapon<P90> {
            static constexpr std::string_view kCanonical = "p90";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::SMG;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
    }
}