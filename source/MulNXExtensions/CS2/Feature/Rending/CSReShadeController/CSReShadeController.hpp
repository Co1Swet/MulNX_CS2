#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/MulNXReShadeBridge/IMulNXReShadeBridge.hpp>

class CSReShadeController final :public CSModuleBase {
    IMulNXReShadeBridge* pMulNXReShadeBridge = nullptr;

    bool Init()override;
};