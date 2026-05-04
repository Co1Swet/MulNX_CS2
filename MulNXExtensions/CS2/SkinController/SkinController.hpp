#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class SkinController final :public CSModuleBase {
    void Main();
    using rebuild = void(*)(void*);
    rebuild re = nullptr;
public:
    bool Init()override;
};