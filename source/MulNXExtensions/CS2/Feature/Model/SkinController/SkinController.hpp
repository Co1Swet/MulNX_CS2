#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Feature/Model/SkinConfigger/SkinConfigger.hpp>

class SkinController final :public CSModuleBase {
    SkinConfigger* pSkinConfigger = nullptr;
public:
    using CEconItemView_GetPaintKit_t = uint32_t(*)(CS2::C_EconItemView* pEconItemView);
    CEconItemView_GetPaintKit_t pGetPaintKit = nullptr;
    using CEconItemView_GetSeed_t = uint32_t(*)(CS2::C_EconItemView* pEconItemView);
    using CEconItemView_GetWear_t = float(*)(CS2::C_EconItemView* pEconItemView, float defaultWear);
private:
    std::unique_ptr<MulNX::Hook> hkGetPaintKit = nullptr;
    std::unique_ptr<MulNX::Hook> hkGetSeed = nullptr;
    std::unique_ptr<MulNX::Hook> hkGetWear = nullptr;

    std::optional<Steam64UID> GetBuyerSteamID(CS2::C_EconItemView* pEconItemView);
    CS2::C_CSWeaponBase* GetWeapon(CS2::C_EconItemView* pEconItemView);
    std::pair<std::optional<Steam64UID>, std::optional<Steam64UID>> GetViewInfo(CS2::C_EconItemView* pEconItemView);

    std::optional<uint32_t> OnGetPaintKit(CS2::C_EconItemView* pEconItemView);
    std::optional<uint32_t> OnGetSeed(CS2::C_EconItemView* pEconItemView);
    std::optional<float> OnGetWear(CS2::C_EconItemView* pEconItemView);
    
    void Window();
    bool Init()override;
};