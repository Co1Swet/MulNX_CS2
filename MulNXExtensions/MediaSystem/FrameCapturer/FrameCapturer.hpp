#pragma once

#include <MulNX/MulNX.hpp>

class FrameCapturer final :public MulNX::ModuleBase {
public:
    bool Init()override;
};