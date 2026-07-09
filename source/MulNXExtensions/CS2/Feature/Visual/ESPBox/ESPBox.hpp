#pragma once
#include <Intro/CSModuleBase.hpp>

class ESPBox final :public CSModuleBase {
    bool Draw(MulNX::UINode* node);
    bool Init()override;
};