#pragma once
#include <Feature/MapRemap/MapState/MapState.hpp>

class MapProto final :public CSModuleBase, public RemapMixin<MapProto> {
    std::unique_ptr<MulNX::Hook> hkCDemoFileHeader_PraseString = nullptr;
    std::unique_ptr<MulNX::Hook> hkCNETMsg_SpawnGroup_Load_PraseString = nullptr;
    std::unique_ptr<MulNX::Hook> hkCSVCMsg_ServerInfo_PraseString = nullptr;
    std::unique_ptr<MulNX::Hook> hkCSVCMsg_ClearAllStringTables_PraseString = nullptr;
    std::unique_ptr<MulNX::Hook> hkCSVCMsg_GameSessionConfiguration_PraseString = nullptr;
    bool Init()override;
    void OnEngine2Load(MulNX::Memory::Region& textRegion);
    bool CheckName(std::string_view sv);
};