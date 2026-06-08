#pragma once
#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class EntityListScanner :public CSModuleBase {
    void Window(MulNX::UINode* node);
public:
    bool Init();
};