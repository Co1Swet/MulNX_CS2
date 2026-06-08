#pragma once
#include <MulNXExtensions/CS2/PlayerHub/CSViewPlayerModuleBase.hpp>

class SmokeController final : public CSViewPlayerModuleBase {
    std::unique_ptr<MulNX::Hook> hkSetSmokeProps = nullptr;
    std::unordered_map<Steam64UID, uint32_t> playerColors;   // 玩家自定义烟雾颜色
    std::map<CS2::ui8TeamNum, uint32_t> teamColors;          // 队伍自定义烟雾颜色

public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& Msg)override;

    void HubPlayer(MulNX::UINode* node)override;
    void HubTeam(MulNX::UINode* node)override;

    void MySetSmokeProps(CS2::C_SmokeGrenadeProjectile* pSmoke);
};