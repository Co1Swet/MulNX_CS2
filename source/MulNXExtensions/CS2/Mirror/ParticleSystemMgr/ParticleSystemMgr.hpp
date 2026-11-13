#pragma once
#include <Intro/Intros.hpp>

class ParticleSystemMgr final :public CSModule<ParticleSystemMgr> {
    using CreateTrailHandle_t = int64_t(*)(void* sysMgr, int64_t* outHandle, void* nameString); // CUtlString*
    using UpdateTrail_t = void(*)(void* sysMgr, int64_t handle, int pointCount, void** controlPointData);

    void* pGameParticleSystemMgr = nullptr;
    MulNX::Memory::DllModule particles{};
    bool Init()override;
public:
    int64_t CreateTrailHandle(int64_t* outHandle, void* nameString);// CUtlString*
    void UpdateTrail(int64_t handle, int pointCount, void** controlPointData);
};