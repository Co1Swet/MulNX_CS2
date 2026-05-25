#pragma once

#include <MulNX/MulNX.hpp>

class DLLInjector final :public MulNX::ModuleBase {
public:
    bool Init()override;
};