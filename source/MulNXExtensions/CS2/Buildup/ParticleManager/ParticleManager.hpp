#pragma once
#include <Intro/CSModuleBase.hpp>

class ParticleManager final :public CSModuleBase {
    using GetParticleManager_t = void* (*)();
    using CreateParticle_t = int (*)(void*, int*, const char*, int, int64_t, int64_t, int64_t, int64_t);
    using UpdateParticle_t = void (*)(void*, int, int, void*, int);
    GetParticleManager_t pFuncGet = nullptr;
    CreateParticle_t pFuncCreateParticle = nullptr;
    UpdateParticle_t pFuncUpdateParticle = nullptr;
    bool Init()override;
public:
    int CreateParticle(int* pOutIndex, const char* pszParticlePath, int arg4, int64_t arg5, int64_t arg6, int64_t arg7, int64_t arg8);
    void UpdateParticle(int idx, int command, void* pData, int arg5);
};