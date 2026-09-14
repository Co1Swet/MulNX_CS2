#pragma once
#include <Intro/CSModuleBase.hpp>

class MapKeyValues final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkPos_MapKVReaded = nullptr;
    bool Init()override;
    void OnClientLoad(MulNX::Memory::Region& textRegion);
};