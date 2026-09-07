#pragma once
#include <Intro/CSModuleBase.hpp>

class TimeController final :public CSModuleBase {
public:
    bool Init()override;

    VExecutor<int()>GetDemoTick{};
    VExecutor<bool()>IsPlayingDemo{};
    VExecutor<bool()>IsDemoPaused{};

    float GetReal();
    bool JumpReal(const float time);
    bool JumpRealRel(float time);
};