#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class MediaProcesser final :public MediaModule<MediaProcesser> {
    struct ConcatInfo {
        struct Segment {
            fs::path v;
            fs::path overrideA;

        };

        fs::path concatTarget;
        std::vector<Segment> segments{};
    };

    std::unordered_map<std::string, ConcatInfo> infos{};

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    void AddConcat(const fs::path& concatTarget, const fs::path& add);
    void EndConcat(const fs::path& concatTarget);

    void DoConcat(const std::vector<fs::path>& inputs, const fs::path& output,
        const std::string& audioEncoderName = "libmp3lame");
};
