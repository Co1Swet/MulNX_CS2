#pragma once
#include <Intro/CSModuleBase.hpp>

class SkinConfigger final :public CSModuleBase {
    struct PlayerWeaponCfg {
        std::optional<int> paintKit{};
        std::optional<int> seed{};
        std::optional<float> wear{};
    };

    enum class Policy :int {
        Disable,
        BuyNoOwn, // 不回退
        BuyOwn, // 回退
        OwnNoBuy, // 不回退
        OwnBuy // 回退
    };

    std::atomic<Policy> policy = Policy::BuyOwn;

    std::unordered_map<Steam64UID, std::unordered_map<std::string, PlayerWeaponCfg>>cfgs{};

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    std::optional<PlayerWeaponCfg> GetPlayerConfig(Steam64UID steamID, const char* weaponClassName)const;
public:
    std::optional<PlayerWeaponCfg> GetWeaponConfig(std::optional<Steam64UID> oBuyerID, std::optional<Steam64UID> oOwner,
        const char* weaponClassName)const;

    void Menu()const;
};