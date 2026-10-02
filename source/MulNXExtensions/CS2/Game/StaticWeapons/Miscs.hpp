#pragma once
#include "StaticWeaponBase.hpp"

namespace CS2 {
    namespace StaticWeapons {
        // 机枪
        struct M249 : Weapon<M249> {
            static constexpr std::string_view kCanonical = "m249";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::MG;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        struct Negev : Weapon<Negev> {
            static constexpr std::string_view kCanonical = "negev";
            static constexpr uint32_t kDamageType = DMG_BULLET;
            static constexpr WeaponClass kClass = WeaponClass::MG;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 300;
        };
        // 刀
        struct Knife : Weapon<Knife> {
            static constexpr std::string_view kCanonical = "knife";
            static constexpr uint32_t kDamageType = DMG_SLASH;
            static constexpr WeaponClass kClass = WeaponClass::Knife;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 1500;
        };
        struct Bayonet : Weapon<Bayonet> {
            static constexpr std::string_view kCanonical = "bayonet";
            static constexpr uint32_t kDamageType = DMG_SLASH;
            static constexpr WeaponClass kClass = WeaponClass::Knife;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 1500;
        };
        // 电击枪
        struct Taser : Weapon<Taser> {
            static constexpr std::string_view kCanonical = "taser";
            static constexpr uint32_t         kDamageType = DMG_SHOCK;
            static constexpr WeaponClass      kClass = WeaponClass::Taser;
            static constexpr GrenadeKind      kGrenade = GrenadeKind::None;
            static constexpr int              kBaseReward = 0;
        };
        // C4
        struct C4 : Weapon<C4> {
            static constexpr std::string_view kCanonical = "c4";
            static constexpr uint32_t         kDamageType = DMG_BLAST;
            static constexpr WeaponClass      kClass = WeaponClass::C4;
            static constexpr GrenadeKind      kGrenade = GrenadeKind::None;
            static constexpr int              kBaseReward = 300;
        };
        // 已安放的 C4（特殊来源，无 weapon_ 前缀）
        struct PlantedC4 : Weapon<PlantedC4> {
            static constexpr std::string_view kCanonical = "planted_c4";
            static constexpr uint32_t         kDamageType = DMG_BLAST;
            static constexpr WeaponClass      kClass = WeaponClass::C4;
            static constexpr GrenadeKind      kGrenade = GrenadeKind::None;
            static constexpr int              kBaseReward = 300;
        };
        // 特殊伤害来源
        struct World : Weapon<World> {
            static constexpr std::string_view kCanonical = "world";
            static constexpr uint32_t kDamageType = DMG_GENERIC;
            static constexpr WeaponClass kClass = WeaponClass::Special;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 0;
        };
        struct WorldSpawn : Weapon<WorldSpawn> {
            static constexpr std::string_view kCanonical = "worldspawn";
            static constexpr uint32_t kDamageType = DMG_GENERIC;
            static constexpr WeaponClass kClass = WeaponClass::Special;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 0;
        };
        struct Inferno : Weapon<Inferno> {
            static constexpr std::string_view kCanonical = "inferno";
            static constexpr uint32_t kDamageType = DMG_BURN;
            static constexpr WeaponClass kClass = WeaponClass::Special;
            static constexpr GrenadeKind kGrenade = GrenadeKind::None;
            static constexpr int kBaseReward = 0;
        };
    }
}