#pragma once
#include <Feature/MapRemap/MapState/MapState.hpp>

class MapKeyValues final :public CSModule<MapKeyValues>, public RemapMixin<MapKeyValues> {
    std::unique_ptr<MulNX::Hook> hkPos_MapKVReaded = nullptr;
    bool Init()override;
    void OnClientLoad(MulNX::Memory::Region& textRegion);
};