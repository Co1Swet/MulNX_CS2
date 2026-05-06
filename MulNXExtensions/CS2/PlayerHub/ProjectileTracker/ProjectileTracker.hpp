#pragma once

#include <MulNX/Base/NewestBuffer/NewestBuffer.hpp>
#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class ProjectileTracker final : public CSModuleBase {
    std::atomic<bool>Enable = false;
    std::atomic<CS2::C_BaseCSGrenadeProjectile*> pTargetWatchProjectile = nullptr;

    // void HandleGrenadeAdd(CS2::C_BaseCSGrenade* pGrenade, std::string&& name);
    // void HandleGrenadeRemove(CS2::C_BaseCSGrenade* pGrenade);

    void HandleProjectileAdd(CS2::C_BaseCSGrenadeProjectile* pProjectile, std::string&& name);

    void ProcessMsg(MulNX::Message& msg)override;
    void Menu(MulNX::UINode* node);
    void Update();
    MulNX::NewestBuffer<MulNX::Math::View> currentView{};
public:
    bool Init()override;
    std::optional<MulNX::Math::View> GetView();
};