#include "CSDemoController.hpp"
#include <Support/TimeController/TimeController.hpp>

bool CSDemoController::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {
        this->pGetCCSDemoController = (GetCCSDemoController_t)this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Demo::Pos_Call_GetCCSDemoController).TryGetCallTarget();
        if (!this->pGetCCSDemoController)MulNX::ErrorTerminate("找不到 pGetCCSDemoController");
        });

    this->SubscribeSync("Hook/CSMainLoop", [this](auto&&...) {
        try {
            this->UpdateRound();
            this->Update();
        }
        catch (const MulNX::Exception& e) {
            this->LogError(e);
        }
        });

    (*this)
        .SubscribeAsync<void>("Demo/PrevRound")
        .SubscribeAsync<void>("Demo/NextRound")
        .SubscribeAsync<int>("Demo/GotoRound")
        ;

    return true;
}

void CSDemoController::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/PrevRound"_hash: {
        const int round = this->currentRound.load(std::memory_order_acquire);
        if (round > 1) {
            this->GotoRound(round - 1);
        }
        break;
    }
    case "Demo/NextRound"_hash: {
        const int round = this->currentRound.load(std::memory_order_acquire);
        if (round > 0) {
            this->GotoRound(round + 1);
        }
        break;
    }
    case "Demo/GotoRound"_hash: {
        auto&& [round] = msg.Access<int>();
        this->GotoRound(round);
        break;
    }
    }
}

void CSDemoController::UpdateRound() {
    auto* pCtrler = this->pGetCCSDemoController();

    const auto count = MulNX::MRead(&pCtrler->m_nRoundCount);
    auto* pRounds = MulNX::MRead(&pCtrler->m_pRoundIntervals);
    if (count == 0 || !pRounds)return;

    const int currentTick = this->CS2Time->GetDemoTick();

    for (size_t i = 0; i < count; ++i) {
        const int start = MulNX::MRead(&pRounds[i].nTickStart);
        const int end = MulNX::MRead(&pRounds[i].nTickEnd);
        if (currentTick >= start && currentTick < end) {
            this->currentRound.store(static_cast<int>(i) + 1, std::memory_order_release);
            return;
        }
    }
}

void CSDemoController::GotoRound(int round) {
    auto* pCtrler = this->pGetCCSDemoController();

    const auto count = MulNX::MRead(&pCtrler->m_nRoundCount);
    auto* pRounds = MulNX::MRead(&pCtrler->m_pRoundIntervals);
    if (count == 0 || !pRounds) return;

    const int idx = round - 1;
    if (idx < 0 || idx >= static_cast<int>(count)) {
        this->LogError(std::format("无法跳跃至异常回合索引：{}", idx));
        return;
    }

    const int targetTick = MulNX::MRead(&pRounds[idx].nTickStart);

    this->AsyncCommand(std::format("demo_gototick {}", targetTick));
}