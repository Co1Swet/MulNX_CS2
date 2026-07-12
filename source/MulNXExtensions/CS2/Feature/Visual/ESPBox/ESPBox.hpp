#pragma once
#include <Intro/CSModuleBase.hpp>

class ESPBox final :public CSModuleBase {
    bool Draw();
    bool Init()override;
};