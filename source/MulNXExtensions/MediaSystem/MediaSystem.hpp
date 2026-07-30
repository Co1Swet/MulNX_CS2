#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class MediaSystem final :public MulNX::Module<MediaSystem> {
    std::filesystem::path dirVideos;
    std::string outputFile = "record";

    void Window(MulNX::UICoordinator* uico);
    bool Init()override;
};
