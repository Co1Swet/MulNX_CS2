#include "TeamCounterController.hpp"
#include <MulNX/Base/UI/UI.hpp>

void TeamCounterController::HubWindow(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("TeamCounter");
    MulNX::UI::Checkbox("是否隐藏敌方血条", this->runFlag1);
}

bool TeamCounterController::Init() {
    this->runFlag1.store(true);
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::PosTeamCounterWriteHP).Data();
        this->hkTeamCounterWriteHP = MulNX::Hook::Create(target, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->runFlag1.load())return MulNX::Hook::Then::Continue;
            uint8_t teamByte = *(uint8_t*)((char*)ctx->rbx + 0x10);   // 0 = T, 非0 = CT
            try {
                auto pOBPawn = this->CS2->client.TryGetObservingPawn();
                if (!pOBPawn)return MulNX::Hook::Then::Continue;
                auto team = MulNX::MRead(pOBPawn->iTeamNum());
                uint8_t byteTeam = 0;
                if (team == CS2::ui8TeamNum::CT)byteTeam = 1;
                if (byteTeam == teamByte)return MulNX::Hook::Then::Continue;
            }
            catch (...) {
                
            }
            return MulNX::Hook::Then::JmpUserSettedTarget;
            }, false, false, (uintptr_t)target + 11).value();
        this->hkTeamCounterWriteHP->Attach();

        });

    return true;
}