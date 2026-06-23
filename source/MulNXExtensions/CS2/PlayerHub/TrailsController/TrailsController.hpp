#pragma once
#include <MulNXExtensions/CS2/PlayerHub/CSViewPlayerModuleBase.hpp>

class ParticleManager;
class TrailsController final :public CSViewPlayerModuleBase {
    struct ParticleColor { float r, g, b; };
    struct ParticleProp { float lifetime, width, alpha; };

    std::unordered_map<Steam64UID, ParticleColor>playerColors{};
    ParticleColor TColor{ 224, 175, 86 };
    ParticleColor CTColor{ 114, 155, 221 };
    ParticleProp prop{ 4.0f, 1.0f, 1.0f };

    ParticleManager* pParticleMgr = nullptr;
    WrapHook hkFunc_BaseCSGrenadeProjectile_DrawStuff;
    MulNX::Hook::Then HandleOnCreate(CS2::C_BaseCSGrenadeProjectile* pProjectile);
    MulNX::Hook::Then HandleOnUpdate(CS2::C_BaseCSGrenadeProjectile* pProjectile);
    bool Init()override;
    void HubPlayer(MulNX::UINode* node)override;
    void HubTeam(MulNX::UINode* node)override;
    void Menu(MulNX::UINode* node);
    void ProcessMsg(MulNX::Message& msg)override;
};