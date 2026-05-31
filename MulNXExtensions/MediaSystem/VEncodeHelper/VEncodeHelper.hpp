#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class VEncodeHelper final :public MediaModuleBase {
public:
    bool Init()override;
};