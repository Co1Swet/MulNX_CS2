#include "BombSpotController.hpp"
#include <MulNX/Base/UI/UI.hpp>

bool BombSpotController::Init() {

    this->ISys().SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        // 修改雷包颜色
        auto Pos_Spot_WriteBombState = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Pos_WriteBombState);
        this->hkPos_Spot_WriteBombState = MulNX::Hook::Create(Pos_Spot_WriteBombState.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pOBing = this->CS2->client.TryGetObservingPawn();
            if (!pOBing)return MulNX::Hook::Then::Continue;
            try {
                if (MulNX::MRead(pOBing->iTeamNum()) == CS2::ui8TeamNum::CT) {
                    *(int*)ctx->rdx = IM_COL32(255, 0, 0, 255);// -16776961 red
                }
                // ToDo: 在雷包掉落且正在观战T方成员时修改为白色
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