#include "SkinController.hpp"

void SkinController::Window() {
    auto w = MulNX::UI::RAIIWindow("模型修改");
    MulNX::UI::SliderInt("皮肤编号", this->index, 0, 2000);
}

bool SkinController::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {
        auto t = this->CS2->client.GetTextRegion().FindRegion(CS2::Signatures::Skin::CEconItemView::GetPaint).Data();
        this->hkGetPaint = MulNX::Hook::Create(t, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pRaw = (CEconItemView_GetPaint_t)hk->pMaybeRawFunc;
            auto pEconItemView = (CS2::C_EconItemView*)ctx->rcx;
            ctx->rax = pRaw(pEconItemView);
            try {
                if (auto oIndex = this->GetPaintIndex(pEconItemView)) {
                    ctx->rax = *oIndex;
                }
            }
            catch (const MulNX::Exception& e) {
                this->LogError(e);
            }
            return MulNX::Hook::Then::Return;
            }).value();
        this->RegisterAttachHook(this->hkGetPaint, "GetPaint");
        });

    this->SendUIRoot("Models", [this](auto&&...) {
        this->Window();
        });

    return true;
}

std::optional<uint64_t> SkinController::GetPaintIndex(CS2::C_EconItemView* pEconItemView) {
    auto id = MulNX::MRead(pEconItemView->m_iAccountID());
    auto defId = MulNX::MRead(pEconItemView->m_iItemDefinitionIndex());

    auto pAttributeList = pEconItemView->m_AttributeList();
    auto pManager = MulNX::MRead(pAttributeList->m_pManager());
    if (!pManager)return std::nullopt;
    auto hOuter = MulNX::MRead(pManager->m_hOuter());
    auto pWeapon = this->CS2Entitys->GetBaseEntityFromHandle(hOuter)->As<CS2::C_CSWeaponBase>();
    if (!pWeapon)return std::nullopt;

    uint32_t low = MulNX::MRead(pWeapon->m_OriginalOwnerXuidLow());
    uint32_t high = MulNX::MRead(pWeapon->m_OriginalOwnerXuidHigh());
    auto id2 = ((uint64_t)high << 32) | low;

    auto name = pWeapon->GetName();

    auto hOwner = MulNX::MRead(pWeapon->m_hOwnerEntity());
    auto pOwner = this->CS2Entitys->GetBaseEntityFromHandle(hOwner)->As<CS2::C_CSPlayerPawn>();
    if (!pOwner)return std::nullopt;
    auto hCtrl = MulNX::MRead(pOwner->m_hController());
    auto pCtrl = this->CS2Entitys->GetBaseEntityFromHandle(hCtrl)->As<CS2::CCSPlayerController>();
    if (!pCtrl)return std::nullopt;
    auto steamID = MulNX::MRead(pCtrl->m_steamID());

    uint64_t originalOwnerXuid = 0x0110000100000000ULL + MulNX::MRead(pEconItemView->m_iAccountID());

    return this->index.load(std::memory_order_acquire);
}