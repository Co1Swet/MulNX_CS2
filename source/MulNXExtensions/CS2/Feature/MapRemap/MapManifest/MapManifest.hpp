#pragma once
#include <Intro/CSModuleBase.hpp>
#include <unordered_set>

class MapManifest final :public CSModuleBase {
    std::unordered_set<std::string> errorLoadings{};
    std::unique_ptr<MulNX::Hook> hkPos_Manifest_AddFullPath = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_Log_Failedloading = nullptr;
    bool Init()override;
};