#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class MediaSystem final :public MulNX::Module<MediaSystem> {
    std::filesystem::path dirVideos;
    std::string outputFile = "record";
    class MediaParamManager* pMediaParamManager = nullptr;
    bool Window(MulNX::UINode* node);
    void RecordParamsUI();
public:
    bool Init()override;
};
