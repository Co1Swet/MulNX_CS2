#include "HookMainLoop.hpp"
#include <Support/TimeController/TimeController.hpp>

namespace {
    struct DemoRoundInterval {
        int nTickStart;
        int nTickEnd;
    };

    struct DemoRoundInfo {
        int  currentRoundNumber = 0;
        int  currentRoundStart = 0;
        int  currentRoundEnd = 0;
        int  elapsedTicks = 0;
        int  totalRounds = 0;
        bool valid = false;
        bool inRound = false;
    };

    DemoRoundInfo g_demoRound;

    constexpr uintptr_t RVA_GetDemoController = 0xD35410;
    constexpr uintptr_t OFFSET_ROUND_COUNT = 0x40;
    constexpr uintptr_t OFFSET_ROUND_ARRAY = 0x48;

    using FnGetDemoController = void*** (__fastcall*)();

    void UpdateDemoRoundInfo(uintptr_t clientBase, int currentTick) {
        g_demoRound = {};

        if (!clientBase) {
            return;
        }

        static FnGetDemoController GetDemoController = nullptr;
        if (!GetDemoController) {
            void* fnAddr = reinterpret_cast<void*>(clientBase + RVA_GetDemoController);
            GetDemoController = reinterpret_cast<FnGetDemoController>(fnAddr);
        }
        if (!GetDemoController) {
            return;
        }

        void*** pDemoController = GetDemoController();
        if (!pDemoController) {
            return;
        }

        const uintptr_t base = reinterpret_cast<uintptr_t>(pDemoController);

        const int roundCount = *reinterpret_cast<int*>(base + OFFSET_ROUND_COUNT);
        auto rounds = *reinterpret_cast<DemoRoundInterval**>(base + OFFSET_ROUND_ARRAY);

        if (roundCount <= 0 || !rounds) {
            return;
        }

        g_demoRound.valid = true;
        g_demoRound.totalRounds = roundCount;

        for (int i = 0; i < roundCount; ++i) {
            const int start = rounds[i].nTickStart;
            const int end = rounds[i].nTickEnd;

            if (currentTick >= start && currentTick < end) {
                g_demoRound.currentRoundNumber = i + 1;
                g_demoRound.currentRoundStart = start;
                g_demoRound.currentRoundEnd = end;
                g_demoRound.elapsedTicks = currentTick - start;
                g_demoRound.inRound = true;
                return;
            }
        }
    }
}

bool HookMainLoop::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {
        auto Pos_Call_CInputService_ProcessCommands = this->CS2->engine2.GetTextRegion().FindRegion(CS2::Signatures::Utils::Pos_Call_CInputService_ProcessCommands);
        this->hkPos_Call_CInputService_ProcessCommands = MulNX::Hook::Create(Pos_Call_CInputService_ProcessCommands.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->pGlobalVars->SystemReady.load(std::memory_order_relaxed))return MulNX::Hook::Then::Continue;
            this->PublishSync("Hook/CSMainLoop"_hash);

            auto clientBase = this->CS2->client.GetBaseAddress();

            if (this->pInputSystem->CheckComboClick('V', 2)) {

            }

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

            //UpdateDemoRoundInfo(clientBase, now);

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

    return true;
}