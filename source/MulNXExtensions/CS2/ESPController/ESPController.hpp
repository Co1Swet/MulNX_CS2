#pragma once
#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class ESPController final :public CSModuleBase {
    bool Draw(MulNX::UINode* node);
public:
    bool Init()override;
};