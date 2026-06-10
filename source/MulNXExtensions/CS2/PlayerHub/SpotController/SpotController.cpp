#include "SpotController.hpp"

bool SpotController::Init() {

    this->ISys().SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto Pos_Spot_GetpEntForCheck = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Pos_Spot_GetpEntForCheck);
        this->hkPos_Spot_GetpEntForCheck = MulNX::Hook::Create(Pos_Spot_GetpEntForCheck.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pEntity = (CS2::CCSPlayerController*)ctx->rax;
            try {
                auto name = pEntity->GetName();
                auto pOBing = this->CS2->client.TryGetObservingPawn();
                if (!pOBing)return MulNX::Hook::Then::Continue;
                if (MulNX::MRead(pOBing->iTeamNum()) != MulNX::MRead(pEntity->iTeamNum())) {
                    ctx->rax = 0;
                    return MulNX::Hook::Then::Continue;
                }
            }
            catch (...) {

            }
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkPos_Spot_GetpEntForCheck->Attach();

        auto Pos_Spot_WriteBombState = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Pos_Spot_WriteBombState);
        this->hkPos_Spot_WriteBombState = MulNX::Hook::Create(Pos_Spot_WriteBombState.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {

            return MulNX::Hook::Then::Continue;
            }).value();
        //this->hkPos_Spot_WriteBombState->Attach();
        });

    return true;
}