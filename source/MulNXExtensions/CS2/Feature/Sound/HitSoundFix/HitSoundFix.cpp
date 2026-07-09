#include "HitSoundFix.hpp"
#include <Intro/HookGameEvents/HookGameEvents.hpp>
#include <Buildup/CS2Hash/CS2Hash.hpp>

bool HitSoundFix::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto pFunc = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Sound::EmitHurtFeedbackSound);
        if (!pFunc.IsValid())MulNX::ErrorTerminate("无法找到 EmitHurtFeedbackSound 函数的签名！");
        this->EmitHurtFeedbackSound = std::bit_cast<EmitHurtFeedbackSound_t>(pFunc.Data());
        });

    this->SubscribeSync("Hook/FireEventClientSide", [this](MulNX::Message& msg) {
        auto&& [pEvent] = msg.Access<CS2::CGameEvent*>();
        if (!pEvent)return;
        auto name = std::string_view(pEvent->GetEventName());
        if (name != "player_hurt") return;
       
        this->HandleOnPlayerHurt(pEvent);
        });
    return true;
}

void HitSoundFix::HandleOnPlayerHurt(CS2::CGameEvent* event) {
    CS2::CKV3MemberName attacker{ this->CS2Hashs->attacker, -1, nullptr };
    CS2::CKV3MemberName userid{ this->CS2Hashs->userid, -1, nullptr };
    CS2::CKV3MemberName hitgroup{ this->CS2Hashs->hitgroup, -1, nullptr };

    try {
        auto pOBingPawn = this->CS2->client.TryGetObservingPawn();
        if (!pOBingPawn)return;

        auto pAttackerPawn = event->GetPlayerPawn(attacker);
        if (pOBingPawn != pAttackerPawn)return;

        auto pVictimPawn = event->GetPlayerPawn(userid);
        auto pCCSPlayer_ItemServices = static_cast<CS2::CCSPlayer_ItemServices*>(MulNX::MRead(pVictimPawn->pItemServices()));

        auto hitgroupValue = event->GetInt(hitgroup);
        bool hasHelmet = MulNX::MRead(&pCCSPlayer_ItemServices->m_bHasHelmet);
        auto armorValue = MulNX::MRead(pVictimPawn->m_ArmorValue());

        const char* soundName = nullptr;
        if (hitgroupValue == 1) {  // 头部
            soundName = hasHelmet ?
                "Player.DamageHeadShotArmor.AttackerFeedback" :
                "Player.DamageHeadShot.AttackerFeedback";
        }
        else {  // 身体/其他部位
            if (armorValue > 0) {
                soundName = "Player.DamageBodyArmor.AttackerFeedback";
            }
            else {
                soundName = "Player.DamageBody.AttackerFeedback";
            }
            // 将来可以细化 hitgroup 区分四肢、腹部，或根据伤害类型（刀、燃烧）选择
        }

        if (soundName) {
            // 参数顺序：声源 = 受害者，过滤实体 = 观战者（即攻击者）
            this->EmitHurtFeedbackSound(pVictimPawn, nullptr, soundName);
        }
    }
    catch(MulNX::Exception& e) {
        this->LogError(e);
    }
    return;
}