#include "SpotController.hpp"
#include <MulNX/Base/UI/UI.hpp>

enum TeamSytle :uint64_t {
    CT = 9,
    T = 13,
    Enemy = 17
};

bool SpotController::Init() {

    this->ISys().SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        // 修改绘制状态
        auto Pos_Spot_CmpToSetShow = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Pos_CmpToSetShow).Data();
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

        // 修改小地图上玩家图标的绘制样式
        auto Pos_Spot_WriteMaybeEnumToChangeRadarPlayerDraw = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Pos_WriteMaybeEnumToChangeRadarPlayerDraw);
        this->hkPos_Spot_WriteMaybeEnumToChangeRadarPlayerDraw = MulNX::Hook::Create(Pos_Spot_WriteMaybeEnumToChangeRadarPlayerDraw.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            uint64_t Enum = ctx->rbx;
            auto pOBing = this->CS2->client.TryGetObservingPawn();
            if (!pOBing)return MulNX::Hook::Then::Continue;
            try {
                auto OBingTeam = MulNX::MRead(pOBing->iTeamNum());
                if (OBingTeam == CS2::ui8TeamNum::CT && Enum == T) {
                    ctx->rbx = Enemy;
                }
                else if (OBingTeam == CS2::ui8TeamNum::T && Enum == CT) {
                    ctx->rbx = Enemy;
                }
            }
            catch (...) {
                
            }
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkPos_Spot_WriteMaybeEnumToChangeRadarPlayerDraw->Attach();
        this->ISys().LogSucc(I18n("hook.attached", "Pos_Spot_WriteMaybeEnumToChangeRadarPlayerDraw"));

        // 修改玩家图标的具体绘制组件可见性
        auto pFunc_FinallyUpdatePlayerState = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Func_FinallyUpdatePlayerState).FindFuncStart();
        this->hkFunc_FinallyUpdatePlayerState = MulNX::Hook::Create(pFunc_FinallyUpdatePlayerState.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            ctx->rdx &= ~1ULL;  // 或 ctx->edx &= ~1;
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkFunc_FinallyUpdatePlayerState->Attach();
        this->ISys().LogSucc(I18n("hook.attached", "Func_FinallyUpdatePlayerState"));

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