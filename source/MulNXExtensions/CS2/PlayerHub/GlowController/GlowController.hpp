#pragma once
#include <MulNXExtensions/CS2/PlayerHub/CSViewPlayerModuleBase.hpp>

class GlowController final :public CSViewPlayerModuleBase {
    std::unique_ptr<MulNX::Hook> hkSetGlowColor = nullptr;
    std::unordered_map<Steam64UID, uint32_t>playerColors;
    std::map<CS2::ui8TeamNum, uint32_t>teamColors;
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& Msg)override;
    void Player(MulNX::UINode* node)override;
    void Team(MulNX::UINode* node)override;

    void MySetGlowColor(CS2::CGlowProperty* pGlowProperty, uint32_t* color);
};