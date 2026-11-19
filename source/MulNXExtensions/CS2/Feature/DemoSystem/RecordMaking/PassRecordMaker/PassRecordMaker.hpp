#pragma once
#include <Feature/DemoSystem/DemoSystem.hpp>

class PassRecordMaker final :public DemModule<PassRecordMaker> {
    class AutoRecordRule* pAutoRecordRule = nullptr;

    
    MulNX::CoTask AsyncWaitInfo(bool& succ, const std::string& key, const std::string& demoName);
    MulNX::CoTask EnsureMake(std::string key, std::string demoName,
        std::optional<Steam64UID> localSteamUID = std::nullopt);
    MulNX::CoTask OnUID(Steam64UID uid);

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};