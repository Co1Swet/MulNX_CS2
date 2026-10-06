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
        char pad_0000[0x40]; //0x0000
        size_t m_nRoundCount; //0x0040
        DemoRoundInterval* m_pRoundIntervals; //0x0048
    };
#pragma pack(pop)
}

class CSDemoController final :public CSModuleBase {
    using GetCCSDemoController_t = CS2::CCSDemoController* (*)(void);
    GetCCSDemoController_t pGetCCSDemoController = nullptr;

    std::atomic<int> currentRound = 0;

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    void UpdateRound();
    void GotoRound(int round);
public:
    inline int GetCurrentRound()const { return this->currentRound.load(std::memory_order_acquire); }
};