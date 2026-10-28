#pragma once
#include <Intro/CSModuleBase.hpp>

class ESPBox final :public CSModuleBase {
    MulReflBool(enable, false);
    void Draw();
    bool Init()override;
};