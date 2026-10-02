#pragma once
#include "StaticWeaponBase.hpp"

namespace CS2 {
    namespace StaticWeapons {
        struct Nova : Weapon<Nova> {
            static constexpr std::string_view kCanonical = "nova";
            static constexpr uint32_t kDamageType = DMG_BUCKSHOT;
            static constexpr WeaponClass kClass = WeaponClass::Shotgun;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 900;
        };
        struct MAG7 : Weapon<MAG7> {
            static constexpr std::string_view kCanonical = "mag7";
            static constexpr uint32_t kDamageType = DMG_BUCKSHOT;
            static constexpr WeaponClass kClass = WeaponClass::Shotgun;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 900;
        };
        struct SawedOff : Weapon<SawedOff> {
            static constexpr std::string_view kCanonical = "sawedoff";
            static constexpr uint32_t kDamageType = DMG_BUCKSHOT;
            static constexpr WeaponClass kClass = WeaponClass::Shotgun;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 900;
        };
        struct XM1014 : Weapon<XM1014> {
            static constexpr std::string_view kCanonical = "xm1014";
            static constexpr uint32_t kDamageType = DMG_BUCKSHOT;
            static constexpr WeaponClass kClass = WeaponClass::Shotgun;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 600;
        };
    }
}