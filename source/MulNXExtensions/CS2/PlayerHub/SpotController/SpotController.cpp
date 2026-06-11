#include "SpotController.hpp"
#include <MulNX/Base/UI/UI.hpp>

bool SpotController::Init() {

    this->ISys().SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto Pos_Spot_CmpToSetShow = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Pos_Spot_CmpToSetShow).Data();
        this->hkPos_Spot_CmpToSetShow = MulNX::Hook::Create(Pos_Spot_CmpToSetShow, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pPawn = (CS2::C_CSPlayerPawn*)ctx->r15;
            try {
                auto pOBing = this->CS2->client.TryGetObservingPawn();
                if (!pOBing)return MulNX::Hook::Then::Continue;
                auto teamOBing = MulNX::MRead(pOBing->iTeamNum());
                auto team = MulNX::MRead(pPawn->iTeamNum());
                if (teamOBing == team)return MulNX::Hook::Then::JmpUserSettedTarget;
            }
            catch (...) {

            }
            return MulNX::Hook::Then::Continue;
            }, false, false, (uintptr_t)(Pos_Spot_CmpToSetShow + 10)).value();
        this->hkPos_Spot_CmpToSetShow->Attach();
        this->ISys().LogSucc(I18n("hook.attached", "Pos_Spot_CmpToSetShow where r15 is C_CSPlayerPawn*"));

        auto Pos_Spot_WriteBombState = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Pos_Spot_WriteBombState);
        this->hkPos_Spot_WriteBombState = MulNX::Hook::Create(Pos_Spot_WriteBombState.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pOBing = this->CS2->client.TryGetObservingPawn();
            if (!pOBing)return MulNX::Hook::Then::Continue;
            try {
                if (MulNX::MRead(pOBing->iTeamNum()) == CS2::ui8TeamNum::CT) {
                    *(int*)ctx->rdx = IM_COL32(255, 0, 0, 255);// -16776961;
                }
            }
            catch (...) {
                
            }
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkPos_Spot_WriteBombState->Attach();
        this->ISys().LogSucc(I18n("hook.attached", "Pos_Spot_WriteBombState where rdx is BombColor*"));
        });

    return true;
}