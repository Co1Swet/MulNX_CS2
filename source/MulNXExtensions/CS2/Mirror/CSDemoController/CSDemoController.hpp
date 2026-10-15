#pragma once
#include <Intro/CSModuleBase.hpp>

namespace CS2 {
#pragma pack(push, 1)
    struct DemoRoundInterval {
        int nTickStart;
        int nTickEnd;
    };
    class CCSDemoController {
    public:
        char pad_0000[0x50];
        size_t m_nRoundCount;
        DemoRoundInterval* m_pRoundIntervals;
    };
#pragma pack(pop)
}

class CSDemoController final :public CSModuleBase {
    using GetCCSDemoController_t = CS2::CCSDemoController* (*)(void);
    GetCCSDemoController_t pGetCCSDemoController = nullptr;

    std::atomic<int> currentRound = 0;
    std::atomic<int> totalRounds = 0;

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    void UpdateRound();
    void GotoRound(int round);
public:
    inline int GetCurrentRound()const { return this->currentRound.load(std::memory_order_acquire); }
    inline int GetTotalRound()const { return this->totalRounds.load(std::memory_order_acquire); }
};