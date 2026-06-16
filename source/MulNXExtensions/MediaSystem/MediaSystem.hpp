#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class MediaSystem final :public MulNX::Module<MediaSystem> {
    std::filesystem::path dirVedios;
    bool Window(MulNX::UINode* node);
public:
    bool Init()override;
};