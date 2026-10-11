#pragma once
#include <Intro/CSModuleBase.hpp>

class AdTimeFix final :public CSModuleBase {
    inline static AdTimeFix* pThis = nullptr;

    using Plat_FloatTime_t = double(*)(void);
    MulNX::Memory::IatHook hkPanorama_Plat_FloatTime{};
    MulNX::Memory::IatHook hkScenesystem_Plat_FloatTime{};
    Plat_FloatTime_t pRawPlat_FloatTime = nullptr;

    double OnGetTime(void);
    bool Init()override;
};