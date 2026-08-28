#pragma once
#include <Intro/CSModuleBase.hpp>

class ParticleSystemMgr final :public CSModuleBase {
    void* pGameParticleSystemMgr = nullptr;
    MulNX::Memory::DllModule particles{};
    bool Init()override;
};