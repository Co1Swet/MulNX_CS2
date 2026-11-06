#pragma once
#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/TimeLiner/TimeMixin.hpp>

class FlowClock final : public MulNX::Module<FlowClock>, public TimeAdaptMixin<FlowClock> {
    bool Init() override;

    double GetMinTime() override;
    double GetMaxTime() override;
    double GetTime() override;
    bool SetTime(double time) override;

    void OnSetOn(double time) override;
    void OnSetOff() override;

    std::atomic<double> m_currentTime = 0.0f;
    bool  m_isActive = false;
    std::chrono::steady_clock::time_point m_lastUpdateTime;
};