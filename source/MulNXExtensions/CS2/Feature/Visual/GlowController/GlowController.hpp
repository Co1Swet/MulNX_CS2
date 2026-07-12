#pragma once
#include <Buildup/PlayerHub/CSViewPlayerModuleBase.hpp>

class GlowController final :public CSViewPlayerModuleBase {
    std::unique_ptr<MulNX::Hook> hkSetGlowColor = nullptr;
    std::unordered_map<Steam64UID, uint32_t>playerColors;
    std::map<CS2::ui8TeamNum, uint32_t>teamColors;
    std::atomic<bool>disableGlow = true;
    
    bool Init()override;
    void ProcessMsg(MulNX::Message& Msg)override;
    void HubPlayer()override;
    void HubTeam()override;

    void MySetGlowColor(CS2::CGlowProperty* pGlowProperty, uint32_t* color);
};