#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <mutex>

class MediaProcesser final :public MediaModuleBase {
    void ProcessMsg(MulNX::Message& msg)override;

    bool concatActive = false;
    fs::path concatTarget;
    std::vector<fs::path> concatInputs;
public:
    bool Init()override;
    void BeginConcat(const fs::path& target);
    void AddConcat(const fs::path& add);
    void EndConcat();
};
