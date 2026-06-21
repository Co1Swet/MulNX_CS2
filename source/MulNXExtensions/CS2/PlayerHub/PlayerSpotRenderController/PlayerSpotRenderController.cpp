#include "PlayerSpotRenderController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/PlayerHub/PlayerSpotColorController/PlayerSpotColorController.hpp>

enum TeamSytle :uint64_t {
    CT = 9,
    T = 13,
    Enemy = 17
};

void PlayerSpotRenderController::HubWindow(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("雷达玩家标记控制");
    MulNX::UI::Checkbox("隐藏数字显示", this->hideNumLabel);
    MulNX::UI::Checkbox("强制敌人渲染为红色", this->forceEnemyRed);
    MulNX::UI::Checkbox("强制队友显示", this->forceTeammateDraw);
    this->pColorController->IDraw(node);
}

bool PlayerSpotRenderController::Init() {
    this->pColorController = this->FindModule<PlayerSpotColorController>("PlayerSpotColorController");
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        // 修改绘制状态
        auto Pos_Spot_CmpToSetShow = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Pos_CmpToSetShow).Data();
        this->hkPos_Spot_CmpToSetShow = MulNX::Hook::Create(Pos_Spot_CmpToSetShow, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->forceTeammateDraw.load(std::memory_order_acquire))return MulNX::Hook::Then::Continue;
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
        this->LogSucc(I18n("hook.attached", "Pos_Spot_CmpToSetShow where r15 is C_CSPlayerPawn*"));

        // 修改小地图上玩家图标的绘制样式
        auto Pos_Spot_WriteMaybeEnumToChangeRadarPlayerDraw = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Pos_WriteMaybeEnumToChangeRadarPlayerDraw);
        this->hkPos_Spot_WriteMaybeEnumToChangeRadarPlayerDraw = MulNX::Hook::Create(Pos_Spot_WriteMaybeEnumToChangeRadarPlayerDraw.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->forceEnemyRed.load(std::memory_order_acquire))return MulNX::Hook::Then::Continue;
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
        this->LogSucc(I18n("hook.attached", "Pos_Spot_WriteMaybeEnumToChangeRadarPlayerDraw"));

        // 修改玩家图标的具体绘制组件可见性
        auto pFunc_FinallyUpdatePlayerState = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Func_FinallyUpdatePlayerState).FindFuncStart();
        this->hkFunc_FinallyUpdatePlayerState = MulNX::Hook::Create(pFunc_FinallyUpdatePlayerState.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->hideNumLabel.load(std::memory_order_acquire))return MulNX::Hook::Then::Continue;
            ctx->rdx &= ~1ULL;
            if (ctx->rdx & (1ULL << 27)) {          // 如果 bit 27 是 1
                ctx->rdx &= ~(1ULL << 27);          // 清零 bit 27
                ctx->rdx |= (1ULL << 28);          // 置位 bit 28
            }
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkFunc_FinallyUpdatePlayerState->Attach();
        this->LogSucc(I18n("hook.attached", "Func_FinallyUpdatePlayerState"));
        });

    return true;
}