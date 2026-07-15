#include "HitSoundFix.hpp"
#include <Intro/HookGameEvents/HookGameEvents.hpp>
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
    CS2::CKV3MemberName attacker{ this->CS2Hashs->attacker, -1, nullptr };
    CS2::CKV3MemberName userid{ this->CS2Hashs->userid, -1, nullptr };
    CS2::CKV3MemberName hitgroup{ this->CS2Hashs->hitgroup, -1, nullptr };

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
        if (hitgroupValue == 1) {  // 头部
            if (hasHelmet) {
                return "Player.DamageHeadShotArmor.AttackerFeedback";
            }
            else {
                return "Player.DamageHeadShot.AttackerFeedback";
            }
        }
        else {  // 身体/其他部位
            if (armorValue > 0) {
                return "Player.DamageBodyArmor.AttackerFeedback";
            }
            else {
                return "Player.DamageBody.AttackerFeedback";
            }
            // 将来可以细化 hitgroup 区分四肢、腹部，或根据伤害类型（刀、燃烧）选择
        }
        };

    auto soundName = PraseSoundName();

    if (soundName) {
        // 参数顺序：声源 = 受害者，过滤实体 = 观战者（即攻击者）
        this->EmitHurtFeedbackSound(pVictimPawn, nullptr, soundName);
    }

    return;
}

void HitSoundFix::HandleOnPlayerDeath(CS2::CGameEvent* event) {
    CS2::CKV3MemberName attacker{ this->CS2Hashs->attacker, -1, nullptr };
    CS2::CKV3MemberName userid{ this->CS2Hashs->userid, -1, nullptr };
    CS2::CKV3MemberName hitgroup{ this->CS2Hashs->hitgroup, -1, nullptr };

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
        if (hitgroupValue == 1) {  // 头部
            if (hasHelmet) {
                return "Player.DeathHeadShotArmor.AttackerFeedback";
            }
            else {
                return "Player.DeathHeadShot.AttackerFeedback";
            }
        }
        else {  // 身体/其他部位
            if (armorValue > 0) {
                return "Player.DeathBodyArmor.AttackerFeedback";
            }
            else {
                return "Player.DeathBody.AttackerFeedback";
            }
            // 将来可以细化 hitgroup 区分四肢、腹部，或根据伤害类型（刀、燃烧）选择
        }
        };

    auto soundName = PraseSoundName();

    if (soundName) {
        // 参数顺序：声源 = 受害者，过滤实体 = 观战者（即攻击者）
        this->EmitHurtFeedbackSound(pVictimPawn, nullptr, soundName);
    }

    return;
}