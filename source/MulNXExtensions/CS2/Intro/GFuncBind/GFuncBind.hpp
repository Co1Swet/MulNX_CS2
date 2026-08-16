#pragma once
#include <Intro/CSModuleBase.hpp>

class GFuncBind final :public CSModuleBase {
    MulNX::Memory::DllModule* pTier0 = nullptr;

    bool Init()override;
    void HandleOnTier0Load();

    template<typename F>
    F FindTier0Func(std::string procName);
};