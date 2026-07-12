#include "TeamIDRenderController.hpp"
#include <MulNX/Base/UI/UI.hpp>

void TeamIDRenderController::Menu() {
    MulNX::UI::Checkbox("Team ID隐藏敌方", this->runFlag1);
}

bool TeamIDRenderController::Init() {
    this->runFlag1.store(true);
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Hud::PosTeamID_CmpForHide);
        auto jmp = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Hud::PosTeamID_xxIt);
        this->hkPosTeamID_CmpForHide = MulNX::Hook::Create(target.Data() + 3, [this](MulNX::Hook* hk, RegContext* ctx) {
            return this->HandleForShowTeamID((CS2::C_CSPlayerPawn*)ctx->rbx);
            }, false, false, (uintptr_t)jmp.Begin()).value();
        this->hkPosTeamID_CmpForHide->Attach();
        this->LogSucc(I18n("hook.attached", "cl_teamid_overhead_maxdist_spec is read here for the comparison to decide Team ID display where rbx is C_CSPlayerPawn*"));
        });

    this->SendUINode(this->GetName(), [this](auto&&...) {return this->Menu();});

    return true;
}

MulNX::Hook::Then TeamIDRenderController::HandleForShowTeamID(CS2::C_CSPlayerPawn* pCSPlayerPawn) {
    if (!this->runFlag1.load(std::memory_order_acquire))return MulNX::Hook::Then::Continue;
    try {
        auto pOBPawn = this->CS2->client.TryGetObservingPawn();
        if (!pOBPawn)return MulNX::Hook::Then::Continue;
        auto OBTeam = MulNX::MRead(pOBPawn->iTeamNum());

        auto team = MulNX::MRead(pCSPlayerPawn->iTeamNum());
        if (team == OBTeam) {
            return MulNX::Hook::Then::Continue; // 继续按照旧有距离规则进行判断
        }
        // 跳转到使得迭代器更新的循环尾，继续下一个对象
        // 注意到，这样只是跳过了
        // comiss xmm7,xmm6
        // jb client + rel32
        // 寄存器没有变化，标记位的变化应该也是安全的
        return MulNX::Hook::Then::JmpUserSettedTarget;
    }
    catch (const std::exception& e) {
        this->LogError(std::format("在控制Team ID显示时遇到错误：{}", e.what()));
    }
    return MulNX::Hook::Then::Continue;
}