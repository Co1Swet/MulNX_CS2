#pragma once
#include <Intro/Intros.hpp>
#include <unordered_set>

class MapManifest final :public CSModule<MapManifest> {
    std::unordered_set<std::string> errorLoadings{};
    std::unique_ptr<MulNX::Hook> hkPos_Manifest_AddFullPath = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_Log_Failedloading = nullptr;
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
};