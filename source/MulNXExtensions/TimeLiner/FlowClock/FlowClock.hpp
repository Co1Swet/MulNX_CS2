#pragma once
#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/TimeLiner/ITimeAdapter.hpp>

class FlowClock final : public MulNX::Module<FlowClock>, public ITimeAdapter {
public:
    bool Init() override;

    float GetMinTime() override;
    float GetMaxTime() override;
    float GetTime() override;
    bool SetTime(float time) override;

    void OnSetOn(float time) override;
    void OnSetOff() override;

private:
    float m_currentTime = 0.0f;
    bool  m_isActive = false;
    std::chrono::steady_clock::time_point m_lastUpdateTime;
};