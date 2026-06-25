#pragma once
#include <Intro/CSModuleBase.hpp>

class EntityListScanner :public CSModuleBase {
    void Window(MulNX::UINode* node);
public:
    bool Init();
};