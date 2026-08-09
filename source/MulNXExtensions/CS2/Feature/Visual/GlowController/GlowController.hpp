#pragma once
#include <Intro/CSModuleBase.hpp>

class GlowController final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkSetGlowColor = nullptr;
    std::unordered_map<Steam64UID, uint32_t>playerColors;
    std::atomic<std::optional<uint32_t>>TColor;
    std::atomic<std::optional<uint32_t>>CTColor;

    static_assert(std::atomic<std::optional<uint32_t>>::is_always_lock_free, "This type is not always lock-free");

    std::atomic<bool>disableGlow = true;
    
    bool Init()override;
    void ProcessMsg(MulNX::Message& Msg)override;
    void HubPlayer(MulNX::Message* umsg);
    void HubTeam(MulNX::Message* umsg);

    void HandleSetGlowColor(CS2::CGlowProperty* pGlowProperty, uint32_t* color);
};