#pragma once
#include <MulNX/Base/NewestBuffer/NewestBuffer.hpp>
#include <Intro/HookView/CSViewControlModuleBase.hpp>

class ProjectileTracker final : public CSModuleBase, public CSViewControlMixin<ProjectileTracker> {
    std::set<CS2::C_BaseCSGrenadeProjectile*> bufferProjectiles;
    std::atomic<CS2::C_BaseCSGrenadeProjectile*> pTargetWatchProjectile = nullptr;

    // void HandleGrenadeAdd(CS2::C_BaseCSGrenade* pGrenade, std::string&& name);
    // void HandleGrenadeRemove(CS2::C_BaseCSGrenade* pGrenade);

    bool HandleProjectileAdd(CS2::C_BaseCSGrenadeProjectile* pProjectile);

    void Menu();
    void Main();
    MulNX::NewestBuffer<MulNX::Math::View> currentView{};
    void OnEntityAdd(MulNX::Message& msg);
    void OnEntityRemove(MulNX::Message& msg);

    bool Init()override;
    bool HandleUpdate(CS2::CViewSetup* viewSetup, const int& num)override;
};