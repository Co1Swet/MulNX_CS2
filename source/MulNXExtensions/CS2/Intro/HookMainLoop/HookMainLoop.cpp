#include "HookMainLoop.hpp"
#include <Support/TimeController/TimeController.hpp>

bool HookMainLoop::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {
        auto Pos_Call_CInputService_ProcessCommands = this->CS2->engine2.GetTextRegion().FindRegion(CS2::Signatures::Utils::Pos_Call_CInputService_ProcessCommands);
        this->hkPos_Call_CInputService_ProcessCommands = MulNX::Hook::Create(Pos_Call_CInputService_ProcessCommands.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->pGlobalVars->SystemReady.load(std::memory_order_relaxed))return MulNX::Hook::Then::Continue;
            this->PublishSync("Hook/CSMainLoop"_hash);

            auto old = this->lastUpdateTick.load(std::memory_order_acquire);
            if (old == 0) {
                this->lastUpdateTick.store(this->CS2Time->GetDemoTick(), std::memory_order_release);
                return MulNX::Hook::Then::Continue;
            }
            auto now = this->CS2Time->GetDemoTick();
            if (std::abs(old - now) > 5) {
                this->PublishSync("Hook/MainLoop/TickJumpDetected"_hash);
            }
            this->lastUpdateTick.store(now, std::memory_order_release);

            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_Call_CInputService_ProcessCommands, "Pos_Call_CInputService_ProcessCommands");
        });

    return true;
}