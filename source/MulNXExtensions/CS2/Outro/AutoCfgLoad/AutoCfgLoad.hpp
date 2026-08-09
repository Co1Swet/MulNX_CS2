#pragma once
#include <Intro/CSModuleBase.hpp>

class AutoCfgLoad final :public CSModuleBase {
    std::atomic<int>lastDemoTick = false;
    std::atomic<bool>nextNewRoundIsNewDemo = false;
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    void FireAsyncCfg(const std::filesystem::path& dir)const;
    void Main();
};