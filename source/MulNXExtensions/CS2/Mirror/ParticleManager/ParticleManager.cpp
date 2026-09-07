#include "ParticleManager.hpp"

bool ParticleManager::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        this->pFuncGet = (GetParticleManager_t)this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Particle::Func_ParticleManager_Get).Data();
        this->pFuncCreateParticle = (CreateParticle_t)this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Particle::Func_ParticleManager_CreateParticle).Data();
        this->pFuncUpdateParticle = (UpdateParticle_t)this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Particle::Func_ParticleManager_UpdateParticle).Data();
        this->pFuncBindTrail = (BindTrail_t)this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Particle::Func_BindTrail).Data();
        });

    return true;
}

int ParticleManager::CreateParticle(int* pOutIndex, const char* pszParticlePath, int arg4, int64_t arg5, int64_t arg6, int64_t arg7, int64_t arg8) {
    auto* pRaw = this->pFuncGet();
    if (!pRaw)return -2;
    return this->pFuncCreateParticle(pRaw, pOutIndex, pszParticlePath, arg4, arg5, arg6, arg7, arg8);
}
void ParticleManager::UpdateParticle(int idx, int command, void* pData, int arg5) {
    auto* pRaw = this->pFuncGet();
    if (!pRaw)return;
    return this->pFuncUpdateParticle(pRaw, idx, command, pData, arg5);
}
bool ParticleManager::BindTrail(int idx, unsigned int command, int64_t handle) {
    auto* pRaw = this->pFuncGet();
    if (!pRaw)return false;
    return this->pFuncBindTrail(pRaw, idx, command, handle);
}