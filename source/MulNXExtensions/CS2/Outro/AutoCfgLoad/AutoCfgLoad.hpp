#pragma once
#include <Intro/CSModuleBase.hpp>

class AutoCfgLoad final :public CSModuleBase {
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    void FireAsyncCfg(const std::filesystem::path& dir)const;
};