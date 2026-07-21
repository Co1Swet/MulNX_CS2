#pragma once
#include <Intro/CSModuleBase.hpp>

class NameController final :public CSModuleBase {
    std::array<char[128], 64>nameReplace{};
    std::map<uint64_t, int>nameReplaceInfo{};
    std::unique_ptr<MulNX::Hook>hkGetDecoratedPlayerName = nullptr;
    std::unique_ptr<MulNX::Hook>hkGetPlayerName = nullptr;
    bool bGetPlayerNameHooked = false;

    std::string newNameBuffer;
    bool Init()override;
    void ProcessMsg(MulNX::Message& Msg)override;
    void UIPlayer(MulNX::Message* msg);

    bool SetReplace(Steam64UID uid, const std::string& newName);
};