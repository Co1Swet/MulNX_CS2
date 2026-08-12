#pragma once
#include <Intro/CSModuleBase.hpp>

class HookGSI final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook>hkPos_GSI_ForSpecTarget_call_GetOBingPawn = nullptr;
    bool Init()override;
};