#pragma once

#include <MulNX/MulNX.hpp>

class AudioCapturer final :public MulNX::ModuleBase {
public:
    bool Init()override;
};