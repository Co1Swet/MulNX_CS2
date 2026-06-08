#pragma once
#include <MulNXExtensions/CS2/PlayerHub/CSViewPlayerModuleBase.hpp>

class NameController final :public CSViewPlayerModuleBase {
    std::array<char[128], 64>nameReplace{};
    std::map<uint64_t, int>nameReplaceInfo{};
    std::unique_ptr<MulNX::Hook>hkGetDecoratedPlayerName = nullptr;
    std::unique_ptr<MulNX::Hook>hkGetPlayerName = nullptr;
    bool bGetPlayerNameHooked = false;
    void HandleVHook(CS2::CCSPlayerController* pPlayerController);

    std::string newNameBuffer;
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& Msg)override;
    void HubPlayer(MulNX::UINode* node)override;
    void HubTeam(MulNX::UINode* node)override {};

    bool SetReplace(Steam64UID uid, const std::string& newName);
};