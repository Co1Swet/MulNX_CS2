#pragma once
#include <Intro/CSModuleBase.hpp>

class ResourceSystem final :public CSModuleBase {
    MulNX::Memory::DllModule resourcesystem{};
    void** ppGameResourcesystem = nullptr;
    bool Init()override;
};