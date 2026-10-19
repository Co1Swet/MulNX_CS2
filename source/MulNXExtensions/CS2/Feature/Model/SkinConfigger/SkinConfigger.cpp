#include "SkinConfigger.hpp"

bool SkinConfigger::Init() {

    (*this)
        .SubscribeAsync<void>("SkinChange/Disable")
        .SubscribeAsync<void>("SkinChange/BuyNoOwn")
        .SubscribeAsync<void>("SkinChange/BuyOwn")
        .SubscribeAsync<void>("SkinChange/OwnNoBuy")
        .SubscribeAsync<void>("SkinChange/OwnBuy")
        .SubscribeAsyncEx<Steam64UID, int>("SkinChange/PaintKit/Set")
        .SubscribeAsyncEx<Steam64UID, int>("SkinChange/Seed/Set")
        .SubscribeAsyncEx<Steam64UID, float>("SkinChange/Wear/Set")
        ;

    return true;
}

std::optional<SkinConfigger::PlayerWeaponCfg> SkinConfigger::GetPlayerConfig(Steam64UID steamID, const char* weaponClassName)const {
    auto it = this->cfgs.find(steamID);
    if (it == this->cfgs.end())return std::nullopt;
    auto itWeapon = it->second.find(weaponClassName);
    if (itWeapon == it->second.end())return std::nullopt;
    return itWeapon->second;
}

std::optional<SkinConfigger::PlayerWeaponCfg> SkinConfigger::GetWeaponConfig(
    std::optional<Steam64UID> oBuyerID,
    std::optional<Steam64UID> oOwner,
    const char* weaponClassName) const {

    const auto currentPolicy = this->policy.load(std::memory_order_acquire);
    if (currentPolicy == Policy::Disable) return std::nullopt;

    std::shared_lock lock(this->smutex);

    // 同一个人，回退无意义
    if (oBuyerID && oOwner && *oBuyerID == *oOwner)
        return this->GetPlayerConfig(*oBuyerID, weaponClassName);

    auto query = [&](const std::optional<Steam64UID>& id)
        -> std::optional<PlayerWeaponCfg> {
        if (!id) return std::nullopt;
        return this->GetPlayerConfig(*id, weaponClassName);
        };

    switch (currentPolicy) {
    case Policy::BuyNoOwn:
        return query(oBuyerID);
    case Policy::BuyOwn: {
        auto cfg = query(oBuyerID);
        return cfg ? cfg : query(oOwner);
    }
    case Policy::OwnNoBuy:
        return query(oOwner);
    case Policy::OwnBuy: {
        auto cfg = query(oOwner);
        return cfg ? cfg : query(oBuyerID);
    }
    default:
        return std::nullopt;
    }
}

void SkinConfigger::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "SkinChange/Disable"_hash: {
        this->policy = Policy::Disable;
        break;
    }
    case "SkinChange/BuyNoOwn"_hash: {
        this->policy = Policy::BuyNoOwn;
        break;
    }
    case "SkinChange/BuyOwn"_hash: {
        this->policy = Policy::BuyOwn;
        break;
    }
    case "SkinChange/OwnNoBuy"_hash: {
        this->policy = Policy::OwnNoBuy;
        break;
    }
    case "SkinChange/OwnBuy"_hash: {
        this->policy = Policy::OwnBuy;
        break;
    }
    case "SkinChange/PaintKit/Set"_hash: {
        auto pNetExt = msg.asp.get<MulNX::NetExt>();
        auto&& [steamID, paintKit] = msg.Access<Steam64UID, int>();
        std::unique_lock lock(this->smutex);
        this->cfgs[steamID][pNetExt->str1].paintKit = paintKit;
        break;
    }
    case "SkinChange/Seed/Set"_hash: {
        auto pNetExt = msg.asp.get<MulNX::NetExt>();
        auto&& [steamID, seed] = msg.Access<Steam64UID, int>();
        std::unique_lock lock(this->smutex);
        this->cfgs[steamID][pNetExt->str1].seed = seed;
        break;
    }
    case "SkinChange/Wear/Set"_hash: {
        auto pNetExt = msg.asp.get<MulNX::NetExt>();
        auto&& [steamID, wear] = msg.Access<Steam64UID, float>();
        std::unique_lock lock(this->smutex);
        this->cfgs[steamID][pNetExt->str1].wear = wear;
        break;
    }
    }
}