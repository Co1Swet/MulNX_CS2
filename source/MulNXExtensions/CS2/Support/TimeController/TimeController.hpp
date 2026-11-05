#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/TimeLiner/TimeMixin.hpp>

class TimeController final :public CSModuleBase, public TimeAdaptMixin<TimeController> {
    void* pDemoPlayer = nullptr;
    class CSDemoController* pCSDemoController = nullptr;
    float GetMinTime()override;
    float GetMaxTime()override;
    void TimeRend(TimeLiner* timeline, ImDrawList* dl)override;

    bool Init()override;
public:
    VExecutor<int()> GetDemoTick{};
    VExecutor<bool()> IsPlayingDemo{};
    VExecutor<bool()> IsDemoPaused{};

    int GetDemoLastTick()const;
    float GetTime()override;
    bool SetTime(float time)override;
    bool JumpRealRel(float time);
};