#include "ParticleSystemMgr.hpp"

bool ParticleSystemMgr::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/particles.dll", [this](auto&&...) {
        this->particles = MulNX::Memory::DllModule(L"particles.dll");
        this->pGameParticleSystemMgr = (void*)(this->particles.GetBaseAddress() +
            cs2_dumper::interfaces::particles_dll::ParticleSystemMgr003);
        });

    return true;
}

int64_t ParticleSystemMgr::CreateTrailHandle(int64_t* outHandle, void* nameString) { // CUtlString*
    auto pF = (CreateTrailHandle_t)IVClass::Assume(this->pGameParticleSystemMgr)->GetVFuncPtr(328 / 8);
    return pF(this->pGameParticleSystemMgr, outHandle, nameString);
}

void ParticleSystemMgr::UpdateTrail(int64_t handle, int pointCount, void** controlPointData) {
    auto pF = (UpdateTrail_t)IVClass::Assume(this->pGameParticleSystemMgr)->GetVFuncPtr(336 / 8);
    return pF(this->pGameParticleSystemMgr, handle, pointCount, controlPointData);
}