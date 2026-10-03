#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Game/StaticWeapons/StaticWeaponBase.hpp>
#include <Intro/HookGameEvents/HookGameEvents.hpp>

class OneHitInfo {
public:
    int lastHealth;
    int damage;
    Steam64UID attacker;
    Steam64UID victim;
    CS2::DamageTypes_t damageType;
    
    bool isHeadshot;
    bool isKill;

    bool operator==(const OneHitInfo&) const noexcept = default;

    bool operator<(const OneHitInfo& other) const {
        if (lastHealth != other.lastHealth)return lastHealth < other.lastHealth;
        if (attacker != other.attacker)return attacker < other.attacker;
        if (victim != other.victim)return victim < other.victim;
        if (damage != other.damage)return damage < other.damage;
        if (isHeadshot != other.isHeadshot)return isHeadshot < other.isHeadshot;
        if (isKill != other.isKill)return isKill < other.isKill;
        return damageType < other.damageType;
    }
};

class RoundDamageInfo {
public:
    std::set<OneHitInfo> Hits;
};

class DamageInfo {
public:
    std::map<int, RoundDamageInfo> RoundDamageMap;
};

class DamageRecorder final : public CSModuleBase {
    DamageInfo info{};
    bool Init() override;
    void HandleOnPlayerHurt(MulNX::Message& msg);
public:
    std::vector<OneHitInfo> GetPlayerGiveDamageInfo(int round, Steam64UID player) const;
    std::vector<OneHitInfo> GetPlayerTakeDamageInfo(int round, Steam64UID player) const;
};