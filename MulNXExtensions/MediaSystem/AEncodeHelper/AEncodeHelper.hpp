#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class AEncodeHelper final :public MediaModuleBase {
public:
    bool Init()override;
};