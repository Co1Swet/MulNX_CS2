#pragma once
#include <Intro/CSModuleBase.hpp>

class AdTimeFix final : public CSModuleBase {
    inline static AdTimeFix* pThis = nullptr;

    using Plat_FloatTime_t = double(*)(void);
    MulNX::Memory::IatHook hkPanorama_Plat_FloatTime{};
    MulNX::Memory::IatHook hkScenesystem_Plat_FloatTime{};
    Plat_FloatTime_t pRawPlat_FloatTime = nullptr;

    double OnGetTime(void);
    bool   Init() override;

    std::mutex m_mutex;

    bool   m_initialized = false;
    double m_lastResult = 0.0;   // 虚拟时间累积器
    double m_lastTime = 0.0;   // 上次基准 raw
    int    m_lastFrameCount = 0;     // 上次帧号
    double m_frameTime = 0.0;   // 当前帧剩余目标时间

    static constexpr double kGolden = 0.6180340;
};