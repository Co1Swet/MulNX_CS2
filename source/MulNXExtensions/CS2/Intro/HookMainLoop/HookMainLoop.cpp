#include "HookMainLoop.hpp"
#include <Intro/HookConsole/HookConsole.hpp>
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

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {
        auto t = this->CS2->client.GetTextRegion().FindRegion(CS2::Signatures::Physics::Pos_PhysicsCreated).Data();
        this->hkPos_PhysicsCreated = MulNX::Hook::Create(t, [this](MulNX::Hook* hk, RegContext* ctx) {
            this->PublishSync("Hook/Physics/Created"_hash);
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_PhysicsCreated, "Pos_PhysicsCreated");
        });

    this->SubscribeSync("Hook/RegisterConCommand", [this](MulNX::Message& msg) {
        auto&& [pCmd] = msg.Access<CCmd*>();
        std::string_view name = pCmd->m_pszName;
        if (name == "quit") {
            this->hkQuit = MulNX::Hook::Create((uint8_t*)pCmd->m_pCommandCallback, [this](MulNX::Hook* hk, RegContext* ctx) {
                int result = MessageBoxW(NULL,
                    L"要关闭游戏前，必须先卸载 MulNX。\n点击“确定”卸载 MulNX，之后可再次尝试关闭游戏。",
                    L"MulNX 警告",
                    MB_OKCANCEL | MB_ICONWARNING | MB_TOPMOST
                    );
                if (result == IDOK) {
                    this->LogWarning(I18n("sys.shutdown_warning"));
                    this->Core->Driver()->smutex.unlock();
                }
                ctx->rax = 0;
                return MulNX::Hook::Then::Return;
                }).value();
            this->RegisterAttachHook(this->hkQuit, "Quit");
        }
        });

    return true;
}