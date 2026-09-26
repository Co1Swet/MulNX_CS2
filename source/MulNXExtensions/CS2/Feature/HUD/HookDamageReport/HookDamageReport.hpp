#pragma once
#include <Intro/CSModuleBase.hpp>

class HookDamageReport final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkPos_Check_m_nSendUpdate = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_GettedController = nullptr;
    std::unique_ptr<MulNX::Hook> hkFunc_UpdateDamageReport = nullptr;
    bool Init()override;
    void RefreshPool(CS2::CCSPlayerController* pObservedController, CS2::CHandleBase hObserved);
};