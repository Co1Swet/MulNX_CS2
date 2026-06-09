#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <mutex>

class MediaProcesser final :public MediaModuleBase {
    void ProcessMsg(MulNX::Message& msg)override;

    bool concatActive = false;
    std::filesystem::path concatTarget;
    std::vector<std::filesystem::path> concatInputs;
public:
    bool Init()override;
    void BeginConcat(const std::filesystem::path& target);
    void AddConcat(const std::filesystem::path& add);
    void EndConcat();
};
