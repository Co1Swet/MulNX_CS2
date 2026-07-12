#pragma once
#include <Buildup/PlayerHub/CSViewPlayerModuleBase.hpp>

class ParticleManager;
class TrailsController final :public CSViewPlayerModuleBase {
    struct ParticleColor { float r, g, b; };
    struct ParticleProp { float lifetime, width, alpha; };

    bool* sv_grenade_trajectory_prac_pipreview = nullptr;      // 开启投掷前预览
    float* sv_grenade_trajectory_time_spectator = nullptr;        // 观战队友时也显示5秒的轨迹

    std::unordered_map<Steam64UID, ParticleColor>playerColors{};
    ParticleColor TColor{ 224, 175, 86 };
    ParticleColor CTColor{ 114, 155, 221 };
    ParticleProp prop{ 4.0f, 1.0f, 1.0f };

    ParticleManager* pParticleMgr = nullptr;
    WrapHook hkFunc_BaseCSGrenadeProjectile_DrawStuff;
    std::optional<ParticleColor> FindColor(CS2::C_CSPlayerPawn* pPawn, CS2::CCSPlayerController* pController);
    MulNX::Hook::Then HandleOnCreate(CS2::C_BaseCSGrenadeProjectile* pProjectile);
    MulNX::Hook::Then HandleOnUpdate(CS2::C_BaseCSGrenadeProjectile* pProjectile);
    bool Init()override;
    void HubPlayer()override;
    void HubTeam()override;
    void Menu();
    void ProcessMsg(MulNX::Message& msg)override;
};