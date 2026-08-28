#include "ParticleSystemMgr.hpp"

bool ParticleSystemMgr::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/particles.dll", [this](auto&&...) {
        this->particles = MulNX::Memory::DllModule(L"particles.dll");
        this->pGameParticleSystemMgr = (void*)(this->particles.GetBaseAddress() +
            cs2_dumper::interfaces::particles_dll::ParticleSystemMgr003);
        });

    return true;
}