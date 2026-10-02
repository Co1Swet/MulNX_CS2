#pragma once
#include "StaticWeaponBase.hpp"

namespace CS2 {
    namespace StaticWeapons {
        struct Glock : Weapon<Glock> {
            static constexpr std::string_view kCanonical = "glock";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Pistol;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct USPSilencer : Weapon<USPSilencer> {
            static constexpr std::string_view kCanonical = "usp_silencer";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Pistol;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct HKP2000 : Weapon<HKP2000> {
            static constexpr std::string_view kCanonical = "hkp2000";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Pistol;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct P250 : Weapon<P250> {
            static constexpr std::string_view kCanonical = "p250";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Pistol;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct FiveSeven : Weapon<FiveSeven> {
            static constexpr std::string_view kCanonical = "fiveseven";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Pistol;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct Deagle : Weapon<Deagle> {
            static constexpr std::string_view kCanonical = "deagle";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Pistol;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct Revolver : Weapon<Revolver> {
            static constexpr std::string_view kCanonical = "revolver";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Pistol;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct Elite : Weapon<Elite> {
            static constexpr std::string_view kCanonical = "elite";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Pistol;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct Tec9 : Weapon<Tec9> {
            static constexpr std::string_view kCanonical = "tec9";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Pistol;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct CZ75A : Weapon<CZ75A> {
            static constexpr std::string_view kCanonical = "cz75a";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::Pistol;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 100;
        };
    }
}