#pragma once

#include <MulNX/MulNX.hpp>

class MediaResourceManager final :public MulNX::ModuleBase {
public:
    bool Init()override;
};