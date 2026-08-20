#include "HitSoundFix.hpp"
#include <Intro/HookGameEvents/HookGameEvents.hpp>
#include <Intro/HookView/HookView.hpp>
#include <Buildup/CS2Hash/CS2Hash.hpp>

bool HitSoundFix::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto pFunc = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Sound::EmitHurtFeedbackSound);
        if (!pFunc.IsValid())MulNX::ErrorTerminate("无法找到 EmitHurtFeedbackSound 函数的签名！");
        this->EmitHurtFeedbackSound = std::bit_cast<EmitHurtFeedbackSound_t>(pFunc.Data());
        });

    this->SubscribeSync("Hook/FireEventClientSide/player_hurt", [this](MulNX::Message& msg) {
        auto&& [pEvent] = msg.Access<CS2::CGameEvent*>();
        try {
            this->HandleOnPlayerHurt(pEvent);
        }
        catch (MulNX::Exception& e) {
            this->LogError(e);
        }
        });

    this->SubscribeSync("Hook/FireEventClientSide/player_death", [this](MulNX::Message& msg) {
        auto&& [pEvent] = msg.Access<CS2::CGameEvent*>();
        try {
            this->HandleOnPlayerDeath(pEvent);
        }
        catch (MulNX::Exception& e) {
            this->LogError(e);
        }
        });

    return true;
}

void HitSoundFix::HandleOnPlayerHurt(CS2::CGameEvent* event) {
    if (this->CS2View->GetCameraLeavePlayerState())return;
    static CS2::CKV3MemberName attacker{ this->CS2Hashs->attacker, -1, nullptr };
    static CS2::CKV3MemberName userid{ this->CS2Hashs->userid, -1, nullptr };
    static CS2::CKV3MemberName hitgroup{ this->CS2Hashs->hitgroup, -1, nullptr };
    static CS2::CKV3MemberName health{ this->CS2Hashs->health, -1, nullptr };

    auto pOBingPawn = this->CS2->client.TryGetObservingPawn();
    if (!pOBingPawn)return;

    auto pAttackerPawn = event->GetPlayerPawn(attacker);
    if (pOBingPawn != pAttackerPawn)return;

    auto pVictimPawn = event->GetPlayerPawn(userid);
    auto pCCSPlayer_ItemServices = static_cast<CS2::CCSPlayer_ItemServices*>(MulNX::MRead(pVictimPawn->pItemServices()));

    auto hitgroupValue = event->GetInt(hitgroup);
    bool hasHelmet = MulNX::MRead(pVictimPawn->m_bPrevHelmet());
    auto armorValue = MulNX::MRead(pVictimPawn->m_ArmorValue());

    if (event->GetInt(health) == 0)return;
    
    auto PraseSoundName = [&]()->const char* {
        if (hitgroupValue == 0)return nullptr;
        if (hitgroupValue == 1) {  // 头部
            if (hasHelmet) {
                return "Player.DamageHeadShotArmor.AttackerFeedback";
            }
            else {
                return "Player.DamageHeadShot.AttackerFeedback";
            }
        }
        if (armorValue > 0) {
            return "Player.DamageBodyArmor.AttackerFeedback";
        }
        else {
            return "Player.DamageBody.AttackerFeedback";
        }
        };

    auto soundName = PraseSoundName();

    if (soundName) {
        this->EmitHurtFeedbackSound(pVictimPawn, nullptr, soundName);
    }

    return;
}

void HitSoundFix::HandleOnPlayerDeath(CS2::CGameEvent* event) {
    if (this->CS2View->GetCameraLeavePlayerState())return;
    static CS2::CKV3MemberName attacker{ this->CS2Hashs->attacker, -1, nullptr };
    static CS2::CKV3MemberName userid{ this->CS2Hashs->userid, -1, nullptr };
    static CS2::CKV3MemberName hitgroup{ this->CS2Hashs->hitgroup, -1, nullptr };

    auto pOBingPawn = this->CS2->client.TryGetObservingPawn();
    if (!pOBingPawn)return;

    auto pAttackerPawn = event->GetPlayerPawn(attacker);
    if (pOBingPawn != pAttackerPawn)return;

    auto pVictimPawn = event->GetPlayerPawn(userid);
    auto pCCSPlayer_ItemServices = static_cast<CS2::CCSPlayer_ItemServices*>(MulNX::MRead(pVictimPawn->pItemServices()));

    auto hitgroupValue = event->GetInt(hitgroup);
    bool hasHelmet = MulNX::MRead(pVictimPawn->m_bPrevHelmet());
    auto armorValue = MulNX::MRead(pVictimPawn->m_ArmorValue());

    auto PraseSoundName = [&]()->const char* {
        if (hitgroupValue == 0)return nullptr;
        if (hitgroupValue == 1) {  // 头部
            if (hasHelmet) {
                return "Player.DeathHeadShotArmor.AttackerFeedback";
            }
            else {
                return "Player.DeathHeadShot.AttackerFeedback";
            }
        }
        if (armorValue > 0) {
            return "Player.DeathBodyArmor.AttackerFeedback";
        }
        else {
            return "Player.DeathBody.AttackerFeedback";
        }
        };

    auto soundName = PraseSoundName();

    if (soundName) {
        this->EmitHurtFeedbackSound(pVictimPawn, nullptr, soundName);
    }
    this->EmitHurtFeedbackSound(pVictimPawn, nullptr, "UI.KillCard.1");
}