#pragma once
#include <Intro/CSModuleBase.hpp>

class DemState final :public CSModuleBase {
    bool Init()override;
public:
    std::atomic<std::shared_ptr<const std::string>>currentOperatingDemoName = nullptr;
    std::atomic<std::shared_ptr<const std::string>>currentPlayingDemoName = nullptr;
};