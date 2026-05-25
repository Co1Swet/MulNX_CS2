#pragma once

#include <MulNX/MulNX.hpp>

class MediaProcesser final :public MulNX::ModuleBase {
public:
    bool Init()override;
};