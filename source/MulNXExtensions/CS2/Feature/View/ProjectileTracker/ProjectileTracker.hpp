#pragma once
#include <MulNX/Base/NewestBuffer/NewestBuffer.hpp>
#include <Buildup/PlayerHub/CSViewPlayerModuleBase.hpp>

class ProjectileTracker final : public CSViewPlayerModuleBase {
    std::atomic<bool>Enable = false;
    std::set<CS2::C_BaseCSGrenadeProjectile*> bufferProjectiles;
    std::atomic<CS2::C_BaseCSGrenadeProjectile*> pTargetWatchProjectile = nullptr;

    // void HandleGrenadeAdd(CS2::C_BaseCSGrenade* pGrenade, std::string&& name);
    // void HandleGrenadeRemove(CS2::C_BaseCSGrenade* pGrenade);

    bool HandleProjectileAdd(CS2::C_BaseCSGrenadeProjectile* pProjectile);

    void HubWindow(MulNX::UINode* node)override;
    void HubTeam(MulNX::UINode* node)override {};
    void HubPlayer(MulNX::UINode* node)override {};
    void Main();
    MulNX::NewestBuffer<MulNX::Math::View> currentView{};
    void OnEntityAdd(MulNX::Message& msg);
    void OnEntityRemove(MulNX::Message& msg);
public:
    bool Init()override;
    std::optional<MulNX::Math::View> GetView();
};