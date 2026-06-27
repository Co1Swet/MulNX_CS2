#pragma once
#include <Intro/CSModuleBase.hpp>

class ESPBox final :public CSModuleBase {
    bool Draw(MulNX::UINode* node);
public:
    bool Init()override;
};