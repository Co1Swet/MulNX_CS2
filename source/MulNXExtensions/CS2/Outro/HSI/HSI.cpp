#include "HSI.hpp"
#include <Intro/HookGameEvents/HookGameEvents.hpp>
#include <Buildup/CS2Hash/CS2Hash.hpp>
#include <nlohmann/json.hpp>

bool HSI::Init() {
    this->SubscribeSync("Hook/FireEventClientSide/player_death", [this](MulNX::Message& msg) {
        auto&& [pEvent] = msg.Access<CS2::CGameEvent*>();
        this->OnDeathEvent(pEvent);
        });

    this->SendTask("Main", "CSControl", [this]() {
        this->Main();
        return true;
        });

    return true;
}

void HSI::OnDeathEvent(CS2::CGameEvent* event) {
    static CS2::CKV3MemberName attacker{ this->CS2Hashs->attacker, -1, nullptr };
    static CS2::CKV3MemberName userid{ this->CS2Hashs->userid, -1, nullptr };
    static CS2::CKV3MemberName assister{ this->CS2Hashs->assister, -1, nullptr };

    static CS2::CKV3MemberName assistedflash{ CS2Hashs->assistedflash, -1, nullptr };
    static CS2::CKV3MemberName weapon{ CS2Hashs->weapon, -1, nullptr };
    static CS2::CKV3MemberName headshot{ CS2Hashs->headshot, -1, nullptr };
    static CS2::CKV3MemberName penetrated{ CS2Hashs->penetrated, -1, nullptr };
    static CS2::CKV3MemberName noscope{ CS2Hashs->noscope, -1, nullptr };
    static CS2::CKV3MemberName thrusmoke{ CS2Hashs->thrusmoke, -1, nullptr };
    static CS2::CKV3MemberName attackerblind{ CS2Hashs->attackerblind, -1, nullptr };
    static CS2::CKV3MemberName attackerinair{ CS2Hashs->attackerinair, -1, nullptr };

    // 读取并填充结构体
    KillEvent kEvent;
    kEvent.assistedflash = event->GetInt(assistedflash);
    kEvent.headshot = event->GetInt(headshot);
    kEvent.penetrated = event->GetInt(penetrated);
    kEvent.noscope = event->GetInt(noscope);
    kEvent.thrusmoke = event->GetInt(thrusmoke);
    kEvent.attackerblind = event->GetInt(attackerblind);
    kEvent.attackerinair = event->GetInt(attackerinair);
    const char* weaponStr = event->GetString(weapon);
    kEvent.weapon = weaponStr ? weaponStr : "";

    try {
        auto pAttacker = event->GetPlayerController(attacker);
        auto pVictim = event->GetPlayerController(userid);
        auto pAssister = event->GetPlayerController(assister);

        kEvent.attacker = MulNX::MRead(pAttacker->m_steamID());
        kEvent.victim = MulNX::MRead(pVictim->m_steamID());
        if (pAssister)kEvent.assister = MulNX::MRead(pAssister->m_steamID());
        else kEvent.assister = 0;
    }
    catch (MulNX::Exception& e) {
        this->LogError(e);
        this->LogError("丢失了一个击杀事件");
        return;
    }

    this->buffer.enqueue(std::move(kEvent));
}

void HSI::Main() {
    KillEvent kEvent;
    if (!this->buffer.try_dequeue(kEvent))return;

    nlohmann::json j;
    j["assistedflash"] = kEvent.assistedflash;
    j["weapon"] = kEvent.weapon;
    j["headshot"] = kEvent.headshot;
    j["penetrated"] = kEvent.penetrated;
    j["noscope"] = kEvent.noscope;
    j["thrusmoke"] = kEvent.thrusmoke;
    j["attackerblind"] = kEvent.attackerblind;
    j["attackerinair"] = kEvent.attackerinair;

    // SteamID 转为字符串，与原始 JSON 格式一致
    j["attackerSteamId"] = std::to_string(kEvent.attacker);
    j["victimSteamId"] = std::to_string(kEvent.victim);
    j["assistSteamId"] = std::to_string(kEvent.assister);

    this->WebPost(j.dump());
}