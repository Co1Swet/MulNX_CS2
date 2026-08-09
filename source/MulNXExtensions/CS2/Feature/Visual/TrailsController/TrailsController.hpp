#pragma once
#include <Intro/CSModuleBase.hpp>

class ParticleManager;
class TrailsController final :public CSModuleBase {
    struct ParticleColor { float r, g, b; };
    struct ParticleProp { float lifetime, width, alpha; };

    constexpr static ParticleColor TRaw{ 224.0f, 175.0f, 86.0f };
    constexpr static ParticleColor CTRaw{ 114.0f, 155.0f, 221.0f };

    std::atomic<bool> trailsEnable = true;
    bool* sv_grenade_trajectory_prac_pipreview = nullptr;      // 开启投掷前预览(小窗户)
    float* sv_grenade_trajectory_time_spectator = nullptr;     // 生存期

    ParticleColor TColor = this->TRaw;
    ParticleColor CTColor = this->CTRaw;
    ParticleProp prop{ 4.0f, 1.0f, 1.0f };

    std::unordered_map<Steam64UID, ParticleColor>playerColors{};

    ParticleManager* pParticleMgr = nullptr;
    std::unique_ptr<MulNX::Hook> hkFunc_BaseCSGrenadeProjectile_DrawStuff;

    std::optional<ParticleColor> FindColor(CS2::C_CSPlayerPawn* pPawn, CS2::CCSPlayerController* pController);
    MulNX::Hook::Then HandleOnCreate(CS2::C_BaseCSGrenadeProjectile* pProjectile);
    MulNX::Hook::Then HandleOnUpdate(CS2::C_BaseCSGrenadeProjectile* pProjectile);
    bool Init()override;
    void HubPlayer(MulNX::Message* umsg);
    void HubTeam(MulNX::Message* umsg);
    void Menu();
    void ProcessMsg(MulNX::Message& msg)override;
};