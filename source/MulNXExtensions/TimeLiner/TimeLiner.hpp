#pragma once
#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>

class ITimeLineModule;
class ITimeAdapter;

class TimeLiner :public MulNX::Module<TimeLiner> {
    float currentLeftX = 0.0f;
    float currentRightX = 1.0f;
    float currentBaseY = 0.0f;

    float m_clickRatio = 0.0f;
    void Menu();
    bool Init()override;
    void UpdateTime();
    void UpdatePos();
    ITimeAdapter* pActiveTimeAdapter = nullptr;
public:
    float minTime = 0;
    float maxTime = 0;
    float curTime = 0;

    ITimeAdapter* pTimeAdapter1 = nullptr;
    ITimeAdapter* pTimeAdapter2 = nullptr;

    std::vector<ITimeLineModule*>timeLineModules{};
    ImVec2 Map(float time, int layer)const;

    float GetTime();
};