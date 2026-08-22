#pragma once
#include <Feature/DemoSystem/DemBase/DemModuleBase.hpp>
#include <MulNXExtensions/TimeLiner/ITimeAdapter.hpp>

class DemoPlaying final : public CSModuleBase, public ITimeAdapter {
    void* pDemoPlayer = nullptr;
    bool Init() override;

    float GetMinTime()override;
    float GetMaxTime()override;

    float GetTime()override;
    bool SetTime(float time)override;

    VExecutor<int()>GetDemoTick{};
    VExecutor<bool()>IsPlayingDemo{};
    VExecutor<bool()>IsDemoPaused{};
};