#pragma once
#include "StaticWeaponBase.hpp"

namespace CS2 {
    namespace StaticWeapons {
        struct AK47 : Weapon<AK47> {
            static constexpr std::string_view kCanonical = "ak47";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Rifle;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct AUG : Weapon<AUG> {
            static constexpr std::string_view kCanonical = "aug";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Rifle;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct FAMAS : Weapon<FAMAS> {
            static constexpr std::string_view kCanonical = "famas";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Rifle;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct GalilAR : Weapon<GalilAR> {
            static constexpr std::string_view kCanonical = "galilar";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Rifle;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct M4A1 : Weapon<M4A1> {
            static constexpr std::string_view kCanonical = "m4a1";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Rifle;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct M4A1Silencer : Weapon<M4A1Silencer> {
            static constexpr std::string_view kCanonical = "m4a1_silencer";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Rifle;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct SG556 : Weapon<SG556> {
            static constexpr std::string_view kCanonical = "sg556";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Rifle;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
    }
}