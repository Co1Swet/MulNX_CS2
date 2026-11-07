#pragma once
#include <Intro/Intros.hpp>
#include <MulNXExtensions/TimeLiner/TimeMixin.hpp>

class TimeController final :public CSModuleBase, public TimeAdaptMixin<TimeController> {
    void* pDemoPlayer = nullptr;
    class CSDemoController* pCSDemoController = nullptr;
    double GetMinTime()override;
    double GetMaxTime()override;
    void TimeRend(TimeLiner* timeline, ImDrawList* dl)override;

    bool Init()override;
public:
    VExecutor<int()> GetDemoTick{};
    VExecutor<bool()> IsPlayingDemo{};
    VExecutor<bool()> IsDemoPaused{};

    int GetDemoLastTick()const;
    double GetTime()override;
    bool SetTime(double time)override;
    bool JumpRealRel(double time);
};