#pragma once
#include <MulNXExtensions/CS2/PlayerHub/CSViewPlayerModuleBase.hpp>

class ParticleManager;
class TrailsController final :public CSViewPlayerModuleBase {
    ParticleManager* pParticleMgr = nullptr;
    WrapHook hkFunc_BaseCSGrenadeProjectile_DrawStuff;
public:
    bool Init()override;
};