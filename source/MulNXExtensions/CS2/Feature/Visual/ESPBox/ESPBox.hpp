#pragma once
#include <Intro/Intros.hpp>

class ESPBox final :public CSModule<ESPBox> {
    MulReflBool(enable, false);
    void Draw();
    bool Init()override;
};