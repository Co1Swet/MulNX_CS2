#include "DamageRecorder.hpp"
#include <Mirror/CS2Hash/CS2Hash.hpp>
#include <Support/TimeController/TimeController.hpp>
#include <Game/StaticWeapons/StaticWeapons.hpp>

bool DamageRecorder::Init() {
    this->SubscribeSync("Hook/FireEventClientSide/player_hurt", [this](MulNX::Message& msg) {
        try {
            this->HandleOnPlayerHurt(msg);
        }
        catch (const MulNX::Exception& e) {
            this->LogError(e);
        }
        });

    this->SubscribeSync("Hook/Physics/Created", [this](auto&&...) {
        std::unique_lock lock(this->smutex);
        this->info.RoundDamageMap.clear();
        this->LogInfo("伤害记录已清空");
        });

    return true;
}

void DamageRecorder::HandleOnPlayerHurt(MulNX::Message& msg) {
    auto&& [pEvent] = msg.Access<CS2::CGameEvent*>();

    static CS2::CKV3MemberName attacker{ this->CS2Hashs->attacker, -1, nullptr };
    static CS2::CKV3MemberName userid{ this->CS2Hashs->userid, -1, nullptr };
    static CS2::CKV3MemberName hitgroup{ this->CS2Hashs->hitgroup, -1, nullptr };
    static CS2::CKV3MemberName health{ this->CS2Hashs->health, -1, nullptr };
    static CS2::CKV3MemberName damage{ this->CS2Hashs->dmg_health, -1, nullptr };
    static CS2::CKV3MemberName weapon{ this->CS2Hashs->weapon, -1, nullptr };

    auto pGameRules = this->CS2->client.dwGameRules();
    auto round = pGameRules->m_nRoundStartCount;

    auto pAttackerController = pEvent->GetPlayerController(attacker);
    if (!pAttackerController) return;

    auto pVictimController = pEvent->GetPlayerController(userid);

    auto attackerId = MulNX::MRead(pAttackerController->m_steamID());
    auto oID = this->CS2Entitys->TryGetObservingSteam64UID();
    auto victimId = MulNX::MRead(pVictimController->m_steamID());

    auto hitgroupValue = pEvent->GetInt(hitgroup);
    auto healthValue = pEvent->GetInt(health);
    auto pVictimPawn = pEvent->GetPlayerPawn(userid);
    auto preHP = MulNX::MRead(pVictimPawn->m_iOldHealth());
    auto reallyDamage = preHP - healthValue;
    auto damageValue = pEvent->GetInt(damage);
    auto pWeaponName = pEvent->GetString(weapon);

    if (!pWeaponName|| pWeaponName[0] == '\0') {
        this->LogWarning("无武器的player_hurt事件，已丢弃");
        return;
    }

    if (reallyDamage != damageValue) {
        //this->LogWarning(std::format("玩家 {} 对玩家 {} 造成的伤害不一致，实际伤害: {}, 事件伤害: {}", attackerId, victimId, reallyDamage, damageValue));
    }

    CS2::DamageTypes_t damageType = CS2::FindWeapon(pWeaponName)->damageType;

    OneHitInfo hit{
        .lastHealth = healthValue,
        .damage = reallyDamage,
        .attacker = attackerId,
        .victim = victimId,
        .damageType = damageType,
        .isHeadshot = (hitgroupValue == 1),   // HITGROUP_HEAD
        .isKill = (healthValue == 0),
    };

    std::unique_lock lock(this->smutex);
    this->info.RoundDamageMap[round].Hits.insert(std::move(hit));
}

std::vector<OneHitInfo> DamageRecorder::GetPlayerGiveDamageInfo(int round, Steam64UID player) const {
    std::vector<OneHitInfo> result;
    std::shared_lock lock(this->smutex);
    auto it = this->info.RoundDamageMap.find(round);
    if (it == this->info.RoundDamageMap.end()) return result;
    for (const auto& hit : it->second.Hits) {
        if (hit.attacker == player) result.push_back(hit);
    }
    return result;
}

std::vector<OneHitInfo> DamageRecorder::GetPlayerTakeDamageInfo(int round, Steam64UID player) const {
    std::vector<OneHitInfo> result;
    std::shared_lock lock(this->smutex);
    auto it = this->info.RoundDamageMap.find(round);
    if (it == this->info.RoundDamageMap.end()) return result;
    for (const auto& hit : it->second.Hits) {
        if (hit.victim == player) result.push_back(hit);
    }
    return result;
}