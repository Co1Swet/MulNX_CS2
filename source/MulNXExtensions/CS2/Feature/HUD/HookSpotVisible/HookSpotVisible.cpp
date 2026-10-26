#include "HookSpotVisible.hpp"
#include <MulNX/Base/UI/UI.hpp>

void HookSpotVisible::Menu() {
    MulNX::UI::Checkbox("抑制全局显示", this->noAllVisible);
    MulNX::UI::Checkbox("强制雷达队友显示", this->forceTeammateDraw);
}

bool HookSpotVisible::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        // 修改绘制状态
        auto tPos_Spot_CmpToSetShow = this->CS2->client.GetTextRegion().FindRegion(CS2::Signatures::Spot::Pos_CmpToSetShow).Data();
        this->hkPos_Spot_CmpToSetShow = MulNX::Hook::Create(tPos_Spot_CmpToSetShow, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->forceTeammateDraw.load(std::memory_order_acquire))return MulNX::Hook::Then::Continue;
            auto pPawn = (CS2::C_CSPlayerPawn*)ctx->r15;
            try {
                auto pOBing = this->CS2Entitys->TryGetObservingPawn();
                if (!pOBing)return MulNX::Hook::Then::Continue;
                auto teamOBing = MulNX::MRead(pOBing->iTeamNum());
                auto team = MulNX::MRead(pPawn->iTeamNum());
                if (teamOBing == team)return MulNX::Hook::Then::JmpUserSettedTarget;
            }
            catch (MulNX::Exception& e) {
                this->LogError(e);
            }
            return MulNX::Hook::Then::Continue;
            }, true, false, (uintptr_t)(tPos_Spot_CmpToSetShow + 10)).value();
        this->RegisterAttachHook(this->hkPos_Spot_CmpToSetShow, "Pos_Spot_CmpToSetShow where r15 is C_CSPlayerPawn*");
        // 修改全局可见性
        auto tPos_SetAllVisible = this->CS2->client.GetTextRegion().FindRegion(CS2::Signatures::Spot::Pos_SetAllVisible).Data();
        this->hkPos_SetAllVisible = MulNX::Hook::Create(tPos_SetAllVisible, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (this->noAllVisible.load(std::memory_order_acquire))return MulNX::Hook::Then::SkipAllAndContinue;
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_SetAllVisible, "Pos_SetAllVisible");
        });

    this->UIRegisterCallback("UI.2DVision", [this](auto&&...) {return this->Menu();});

    return true;
}