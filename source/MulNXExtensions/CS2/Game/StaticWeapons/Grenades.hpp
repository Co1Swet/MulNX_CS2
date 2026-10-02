#pragma once
#include "StaticWeaponBase.hpp"

namespace CS2 {
    namespace StaticWeapons {
        struct HEGrenade : Weapon<HEGrenade> {
            static constexpr std::string_view kCanonical = "hegrenade";
            static constexpr uint32_t kDamageType = DMG_BLAST;
            static constexpr WeaponClass kClass = WeaponClass::Grenade;
            static constexpr GrenadeKind kGrenade = GrenadeKind::HE;
            static constexpr int kBaseReward = 300;
        };
        struct Molotov : Weapon<Molotov> {
            static constexpr std::string_view kCanonical = "molotov";
            static constexpr uint32_t kDamageType = DMG_BURN;
            static constexpr WeaponClass kClass = WeaponClass::Grenade;
            static constexpr GrenadeKind kGrenade = GrenadeKind::Molotov;
            static constexpr int kBaseReward = 300;
        };
        struct IncGrenade : Weapon<IncGrenade> {
            static constexpr std::string_view kCanonical = "incgrenade";
            static constexpr uint32_t kDamageType = DMG_BURN;
            static constexpr WeaponClass kClass = WeaponClass::Grenade;
            static constexpr GrenadeKind kGrenade = GrenadeKind::Incendiary;
            static constexpr int kBaseReward = 300;
        };
        struct Flashbang : Weapon<Flashbang> {
            static constexpr std::string_view kCanonical = "flashbang";
            static constexpr uint32_t kDamageType = DMG_BLAST;
            static constexpr WeaponClass kClass = WeaponClass::Grenade;
            static constexpr GrenadeKind kGrenade = GrenadeKind::Flash;
            static constexpr int kBaseReward = 300;
        };
        struct SmokeGrenade : Weapon<SmokeGrenade> {
            static constexpr std::string_view kCanonical = "smokegrenade";
            static constexpr uint32_t kDamageType = DMG_BLAST;
            static constexpr WeaponClass kClass = WeaponClass::Grenade;
            static constexpr GrenadeKind kGrenade = GrenadeKind::Smoke;
            static constexpr int kBaseReward = 300;
        };
        struct Decoy : Weapon<Decoy> {
            static constexpr std::string_view kCanonical = "decoy";
            static constexpr uint32_t kDamageType = DMG_BLAST;
            static constexpr WeaponClass kClass = WeaponClass::Grenade;
            static constexpr GrenadeKind kGrenade = GrenadeKind::Decoy;
            static constexpr int kBaseReward = 300;
        };
    }
}