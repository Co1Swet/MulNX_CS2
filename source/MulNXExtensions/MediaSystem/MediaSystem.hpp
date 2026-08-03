#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class MediaSystem final :public MulNX::Module<MediaSystem> {
    void Window(MulNX::UICoordinator* uico);
    bool Init()override;
};