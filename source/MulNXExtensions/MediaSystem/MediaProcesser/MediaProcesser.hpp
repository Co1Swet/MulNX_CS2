#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <string>

class MediaProcesser final :public MediaModule<MediaProcesser> {
    void ProcessMsg(MulNX::Message& msg)override;

    bool concatActive = false;
    fs::path concatTarget;
    std::vector<fs::path> concatInputs;

    bool Init()override;
    void BeginConcat(const fs::path& target);
    void AddConcat(const fs::path& add);
    void EndConcat();

public:
    void DoConcat(const std::vector<fs::path>& inputs, const fs::path& output,
        const std::string& audioEncoderName = "libmp3lame");
};
