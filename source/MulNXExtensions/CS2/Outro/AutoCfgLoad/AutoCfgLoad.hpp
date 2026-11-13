#pragma once
#include <Intro/Intros.hpp>

class AutoCfgLoad final :public CSModule<AutoCfgLoad> {
    std::atomic<int>lastDemoTick = false;
    std::atomic<bool>nextNewRoundIsNewDemo = false;
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    void FireAsyncCfg(const fs::path& dir)const;
    void Main();
};