#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class MediaSystem final :public MulNX::ModuleBase {
    bool Window(MulNX::UINode* node);
public:
    bool Init()override;
};