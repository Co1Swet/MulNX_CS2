#include "BombSpotController.hpp"
#include <MulNX/Base/UI/UI.hpp>

void BombSpotController::Menu() {
    MulNX::UI::Checkbox("当观战CT时强制渲染C4为红色", this->forceBombRedWhenSpecCT);
}

bool BombSpotController::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        // 修改雷包颜色
        auto Pos_Spot_WriteBombState = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Pos_WriteBombState);
        this->hkPos_Spot_WriteBombState = MulNX::Hook::Create(Pos_Spot_WriteBombState.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->forceBombRedWhenSpecCT.load(std::memory_order_acquire))return MulNX::Hook::Then::Continue;
            auto pOBing = this->CS2->client.TryGetObservingPawn();
            if (!pOBing)return MulNX::Hook::Then::Continue;
            try {
                if (MulNX::MRead(pOBing->iTeamNum()) == CS2::ui8TeamNum::CT) {
                    *(int*)ctx->rdx = IM_COL32(255, 0, 0, 255);// -16776961 red
                }
            }
            catch (...) {

            }
            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkPos_Spot_WriteBombState, "Pos_Spot_WriteBombState where rdx is BombColor*");

        auto Pos_CallGetPawnMaybeSetAllHUD = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Pos_CallGetPawnMaybeSetAllHUD).Data();
        this->hkPos_CallGetPawnMaybeSetAllHUD = MulNX::Hook::Create(Pos_CallGetPawnMaybeSetAllHUD + 14, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pOBing = this->CS2->client.TryGetObservingPawn();
            auto pRet = (CS2::C_BaseEntity*)ctx->rax;
            if (pOBing)ctx->rax = (uint64_t)pOBing;
            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkPos_CallGetPawnMaybeSetAllHUD, "Pos_CallGetPawnMaybeSetAllHUD");
        });
        
    this->UIRegisterCallback("UI.2DVision", [this](auto&&...) {return this->Menu();});

    return true;
}