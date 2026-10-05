#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/TimeLiner/ITimeAdapter.hpp>

class TimeController final :public CSModuleBase, public ITimeAdapter {
    void* pDemoPlayer = nullptr;
    float GetMinTime()override;
    float GetMaxTime()override;
   
    bool Init()override;
public:
    VExecutor<int()> GetDemoTick{};
    VExecutor<bool()> IsPlayingDemo{};
    VExecutor<bool()> IsDemoPaused{};

    float GetTime()override;
    bool SetTime(float time)override;
    bool JumpRealRel(float time);
};