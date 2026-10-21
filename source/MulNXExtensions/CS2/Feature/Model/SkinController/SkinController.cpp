#include "SkinController.hpp"
#include <MulNXThirdParty/hlae/binutils.h>

// 很多人喜欢做好一样东西后拆除脚手架，我也一样

void SkinController::Window() {
    auto w = MulNX::UI::RAIIWindow("模型修改");
    MulNX::UI::SliderInt("皮肤编号", this->index, 0, 2000);
}

bool SkinController::Init() {
    this->pSkinConfigger = this->FindModule<SkinConfigger>("SkinConfigger");

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {
        auto t = this->CS2->client.GetTextRegion().FindRegion(CS2::Signatures::Skin::CEconItemView::GetPaintKit).Data();
        void** vtable = (void**)Afx::BinUtils::FindClassVtable(this->CS2->client.hModule,
            ".?AVC_EconItemView@@", 0, 0);
        if (!vtable)MulNX::ErrorTerminate(".?AVC_EconItemView@@ 查找失败");
        if ((uint8_t*)(vtable[3]) != t)MulNX::ErrorTerminate("GetPaintKit 查找异常");

        this->hkGetPaintKit = MulNX::Hook::Create(t, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pRaw = (CEconItemView_GetPaintKit_t)hk->pMaybeRawFunc;
            auto pEconItemView = (CS2::C_EconItemView*)ctx->rcx;
            ctx->rax = pRaw(pEconItemView);
            try {
                auto o = this->OnGetPaintKit(pEconItemView);
                if (!o)return MulNX::Hook::Then::Return;
                ctx->rax = *o;
            }
            catch (const MulNX::Exception& e) {
                this->LogError(e);
            }
            return MulNX::Hook::Then::Return;
            }).value();
        this->RegisterAttachHook(this->hkGetPaintKit, "GetPaintKit");

        this->hkGetSeed = MulNX::Hook::Create((uint8_t*)(vtable[4]), [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pRaw = (CEconItemView_GetSeed_t)hk->pMaybeRawFunc;
            auto pEconItemView = (CS2::C_EconItemView*)ctx->rcx;
            ctx->rax = pRaw(pEconItemView);
            try {
                auto o = this->OnGetSeed(pEconItemView);
                if (!o)return MulNX::Hook::Then::Return;
                ctx->rax = *o;
            }
            catch (const MulNX::Exception& e) {
                this->LogError(e);
            }
            return MulNX::Hook::Then::Return;
            }).value();
        this->RegisterAttachHook(this->hkGetSeed, "GetSeed");

        this->hkGetWear = MulNX::Hook::Create((uint8_t*)(vtable[5]), [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pRaw = (CEconItemView_GetWear_t)hk->pMaybeRawFunc;
            auto pEconItemView = (CS2::C_EconItemView*)ctx->rcx;
            ctx->xmm0.m128_f32[0] = pRaw(pEconItemView, ctx->xmm1.m128_f32[0]);
            try {
                auto o = this->OnGetWear(pEconItemView);
                if (!o)return MulNX::Hook::Then::Return;
                ctx->xmm0.m128_f32[0] = *o;
            }
            catch (const MulNX::Exception& e) {
                this->LogError(e);
            }
            return MulNX::Hook::Then::Return;
            }).value();
        this->RegisterAttachHook(this->hkGetWear, "GetWear");
        });

    this->SendUIRoot("Models", [this](auto&&...) {
        this->Window();
        });

    return true;
}
std::optional<Steam64UID> SkinController::GetBuyerSteamID(CS2::C_EconItemView* pEconItemView) {
    try {
        auto id = MulNX::MRead(pEconItemView->m_iAccountID());
        uint64_t originalOwnerXuid = 0x0110000100000000ULL + id;
        return originalOwnerXuid;
    }
    catch (const MulNX::Exception& e) {
        this->LogError(e);
    }
    return std::nullopt;
}
CS2::C_CSWeaponBase* SkinController::GetWeapon(CS2::C_EconItemView* pEconItemView) {
    try {
        auto pAttributeList = pEconItemView->m_AttributeList();
        auto pManager = MulNX::MRead(pAttributeList->m_pManager());
        if (!pManager)return nullptr;
        auto hOuter = MulNX::MRead(pManager->m_hOuter());
        auto pWeapon = this->CS2Entitys->GetBaseEntityFromHandle(hOuter)->As<CS2::C_CSWeaponBase>();
        return pWeapon;
    }
    catch (const MulNX::Exception& e) {
        this->LogError(e);
    }
    return nullptr;
}
std::pair<std::optional<Steam64UID>, std::optional<Steam64UID>> SkinController::GetViewInfo(CS2::C_EconItemView* pEconItemView) {
    auto o = this->GetBuyerSteamID(pEconItemView);
    if (!o)return { std::nullopt,std::nullopt };
    auto buyerID = *o;
    auto pWeapon = this->GetWeapon(pEconItemView);
    if (!pWeapon)return { buyerID,std::nullopt };
    auto o2 = this->CS2Entitys->GetWeaponOwnerSteamID(pWeapon);
    if (!o2)return { buyerID,std::nullopt };
    auto ownerID = *o2;
    return { buyerID,ownerID };
}

std::optional<uint32_t> SkinController::OnGetPaintKit(CS2::C_EconItemView* pEconItemView) {
    return this->index.load();
    auto [oBuyer, oOwner] = this->GetViewInfo(pEconItemView);
    auto pWeapon = this->GetWeapon(pEconItemView);
    if (!pWeapon)return std::nullopt;
    auto oPlayerWeaponCfg = this->pSkinConfigger->GetWeaponConfig(oBuyer, oOwner, pWeapon->GetEntityClassName());
    
    if (!oPlayerWeaponCfg)return std::nullopt;
    return oPlayerWeaponCfg->paintKit;
}
std::optional<uint32_t> SkinController::OnGetSeed(CS2::C_EconItemView* pEconItemView) {
    auto [oBuyer, oOwner] = this->GetViewInfo(pEconItemView);
    auto pWeapon = this->GetWeapon(pEconItemView);
    if (!pWeapon)return std::nullopt;
    auto oPlayerWeaponCfg = this->pSkinConfigger->GetWeaponConfig(oBuyer, oOwner, pWeapon->GetEntityClassName());
    if (!oPlayerWeaponCfg)return std::nullopt;
    return oPlayerWeaponCfg->seed;
}
std::optional<float> SkinController::OnGetWear(CS2::C_EconItemView* pEconItemView) {
    auto [oBuyer, oOwner] = this->GetViewInfo(pEconItemView);
    auto pWeapon = this->GetWeapon(pEconItemView);
    if (!pWeapon)return std::nullopt;
    auto oPlayerWeaponCfg = this->pSkinConfigger->GetWeaponConfig(oBuyer, oOwner, pWeapon->GetEntityClassName());
    if (!oPlayerWeaponCfg)return std::nullopt;
    return oPlayerWeaponCfg->wear;
}