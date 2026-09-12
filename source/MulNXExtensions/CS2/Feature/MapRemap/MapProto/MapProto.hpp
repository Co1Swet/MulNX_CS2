#pragma once
#include <Intro/CSModuleBase.hpp>

class MapProto final :public CSModuleBase {
    std::atomic<bool>enable = false;
    std::unique_ptr<MulNX::Hook> hkCDemoFileHeader_PraseString = nullptr;
    std::unique_ptr<MulNX::Hook> hkCNETMsg_SpawnGroup_Load_PraseString = nullptr;
    bool Init()override;
    void OnEngine2Load(MulNX::Memory::Region& textRegion);
};