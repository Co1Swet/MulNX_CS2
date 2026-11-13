#pragma once
#include <Intro/Intros.hpp>

class MapResource final :public CSModule<MapResource> {
    std::unique_ptr<MulNX::Hook> hkFunc_RequestResourceByHash = nullptr;
    using FindResourceByHash_t = uint64_t(*)(uint64_t, uint64_t);
    FindResourceByHash_t pFindByHash = nullptr;
    bool Init()override;
};