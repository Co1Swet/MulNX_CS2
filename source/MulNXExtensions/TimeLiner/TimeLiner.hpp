#pragma once
#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>

class ITimeAdapter;

class TimeLiner :public MulNX::Module<TimeLiner> {
    std::atomic<double> curTime = 0;
    std::atomic<double> minTime = 0;
    std::atomic<double> maxTime = 0;

    std::atomic<float> currentLeftX = 0.0f;
    std::atomic<float> currentRightX = 1.0f;
    std::atomic<float> currentBaseY = 0.0f;

    std::atomic<float> m_clickRatio = 0.0f;
    void Menu();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    void UpdateTime();
    void UpdatePos();
    ITimeAdapter* pActiveTimeAdapter = nullptr;
public:
    ITimeAdapter* pTimeAdapter1 = nullptr;
    ITimeAdapter* pTimeAdapter2 = nullptr;
    std::vector<class ITimeRend*> timeLineModules{};

    ImVec2 Map(double time, int layer)const;

    double GetTime()const;
    inline double GetMinTime()const { return this->minTime.load(std::memory_order_acquire); }
    inline double GetMaxTime()const { return this->maxTime.load(std::memory_order_acquire); }
    void SetTime(double time)const;
};