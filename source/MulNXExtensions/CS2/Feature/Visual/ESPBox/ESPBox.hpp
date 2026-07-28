#pragma once
#include <Intro/CSModuleBase.hpp>

class ESPBox final :public CSModuleBase {
    std::atomic<bool> enable = false;
    void Draw();
    bool Init()override;
};