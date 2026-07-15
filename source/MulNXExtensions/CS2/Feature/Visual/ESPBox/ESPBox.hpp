#pragma once
#include <Intro/CSModuleBase.hpp>

class ESPBox final :public CSModuleBase {
    void Draw();
    bool Init()override;
};