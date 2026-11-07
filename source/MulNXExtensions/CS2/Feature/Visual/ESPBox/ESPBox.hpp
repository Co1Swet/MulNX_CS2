#pragma once
#include <Intro/Intros.hpp>

class ESPBox final :public CSModuleBase {
    MulReflBool(enable, false);
    void Draw();
    bool Init()override;
};