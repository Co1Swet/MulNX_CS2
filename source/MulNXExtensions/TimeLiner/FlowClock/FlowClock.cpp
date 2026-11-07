#include "FlowClock.hpp"

bool FlowClock::Init() {
    this->pTimeline->pTimeAdapter2 = this;
    // 初始化上次更新时间点
    m_lastUpdateTime = std::chrono::steady_clock::now();
    return true;
}

double FlowClock::GetMinTime() {
    return this->m_currentTime - 50.0f;
}

double FlowClock::GetMaxTime() {
    return this->m_currentTime + 50.0f;
}

double FlowClock::GetTime() {
    if (m_isActive) {
        auto now = std::chrono::steady_clock::now();
        double delta = std::chrono::duration<double>(now - m_lastUpdateTime).count();
        m_currentTime += delta;
        m_lastUpdateTime = now;
    }
    return m_currentTime;
}

bool FlowClock::SetTime(double time) {
    m_currentTime = time;
    // 重置时间基准，避免累计之前未更新的增量
    m_lastUpdateTime = std::chrono::steady_clock::now();
    return true;
}

void FlowClock::OnSetOn(double time) {
    m_isActive = true;
    m_currentTime = time;           // 继承主时钟的当前时间
    m_lastUpdateTime = std::chrono::steady_clock::now(); // 重置时间基准
}

void FlowClock::OnSetOff() {
    m_isActive = false;
    // 可以保留 m_currentTime，以便下次激活时继续
}