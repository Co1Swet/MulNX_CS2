#include "RewardFix.hpp"
#include <Game/tree/Econ.hpp>
#include <Mirror/CS2Hash/CS2Hash.hpp>

void RewardFix::Menu() {
    MulNX::UI::Checkbox("启用击杀奖励提示", this->enable);
}

bool RewardFix::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {
        CS2::C_EconItemView::pGetFromItemID = (CS2::C_EconItemView::GetFromItemID_t)this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Econ::CEconItemView::GetEconItemViewFromItemID).Data();
        CS2::C_EconItemView::pGetVData = (CS2::C_EconItemView::GetVData_t)this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Econ::CEconItemView::GetVData).Data();
        this->pFunc_EmitChatMsgParams = (Func_EmitChatMsgParams_t)this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Hud::Chat::Func_EmitChatMsgParams).Data();
        });

    this->SubscribeSync("Hook/FireEventClientSide/player_death", [this](MulNX::Message& msg) {
        try {
            if (!this->enable.load(std::memory_order_acquire))return;
            auto&& [pEvent] = msg.Access<CS2::CGameEvent*>();

            static CS2::CKV3MemberName attacker{ this->CS2Hashs->attacker, -1, nullptr };
            static CS2::CKV3MemberName kItemId{ this->CS2Hashs->weapon_itemid,     -1, nullptr };
            static CS2::CKV3MemberName kFaux{ this->CS2Hashs->weapon_fauxitemid, -1, nullptr };

            auto pOBing = this->CS2Entitys->TryGetObservingPawn();
            if (!pOBing)return;
            auto pAttacker = pEvent->GetPlayerPawn(attacker);
            if (pAttacker != pOBing)return;

            const char* s = pEvent->GetString(kItemId);
            uint64_t id = s ? strtoull(s, nullptr, 10) : 0;
            if (!id) {
                const char* f = pEvent->GetString(kFaux);
                id = f ? strtoull(f, nullptr, 10) : 0;
            }
            if (!id)return;

            auto view = CS2::C_EconItemView::pGetFromItemID(id);
            if (!view || !MulNX::MRead(view->m_bInitialized()))return;
            auto vdata = CS2::C_EconItemView::pGetVData(view);
            if (!vdata)return;

            auto award = MulNX::MRead(vdata->m_nKillAward());

            if (award == 300) {
                textmsg::OwnedArgs o{};
                textmsg::Build(o, "#Player_Cash_Award_Killed_Enemy_Generic",
                    "300", nullptr, 3);
                return this->pFunc_EmitChatMsgParams(nullptr, &o);
            }

            auto def = view->GetEconItemDefinition();
            if (!def)return;
            auto name = def->GetUIToken();
            if (!name)return;
            textmsg::OwnedArgs o{};
            textmsg::Build(o, "#Player_Cash_Award_Killed_Enemy",
                std::to_string(award).c_str(), name, 3);
            this->pFunc_EmitChatMsgParams(nullptr, &o);
        }
        catch (const MulNX::Exception& e) {
            this->LogError(e);
        }
        });

    return true;
}