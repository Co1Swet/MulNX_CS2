#include "SkinModelFixer.hpp"

bool SkinModelFixer::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {
        this->pGetPaintDef = (GetPaintDef_t)this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Skin::CEconItemView::GetPaintDef).Data();

        this->pUpdateMeshGroupMask = (UpdateMeshGroupMask_t)this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Skin::CSkeletonInstance::UpdateMeshGroupMask).Data();
        this->hkUpdateMeshGroupMask = MulNX::Hook::Create((uint8_t*)this->pUpdateMeshGroupMask, [this](MulNX::Hook* hk, RegContext* ctx) {
            try {
                return this->OnUpdateMeshGroupMask(hk, ctx);
            }
            catch (const MulNX::Exception& e) {
                this->LogError(e);
            }
            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkUpdateMeshGroupMask, "UpdateMeshGroupMask");

        auto tPos_NetUpdateMeshGroupMask_GettedValue = this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Skin::CSkeletonInstance
                ::Pos_NetUpdateMeshGroupMask_GettedValue).Data();
        this->hkPos_NetUpdateMeshGroupMask_GettedValue = MulNX::Hook::Create(tPos_NetUpdateMeshGroupMask_GettedValue, [this](MulNX::Hook* hk, RegContext* ctx) {
            try {
                return this->OnPos_NetUpdateMeshGroupMask_GettedValue(hk, ctx);
            }
            catch (const MulNX::Exception& e) {
                this->LogError(e);
            }
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_NetUpdateMeshGroupMask_GettedValue, "Pos_NetUpdateMeshGroupMask_GettedValue");
        });

    return true;
}
std::optional<bool> SkinModelFixer::UseLegacyModel(const char* name, CS2::C_CSWeaponBase* pWeapon) {
    auto sv = std::string_view(name);
    if (sv == "weapon_hkp2000") {
        return std::nullopt; // 疑似V社神秘屎山发力
    }
    auto pAttributeContainer = pWeapon->m_AttributeManager();
    auto pItem = pAttributeContainer->m_Item();
    auto def = this->pGetPaintDef(pItem);
    if (!def)return std::nullopt;
    if (def->m_bUseLegacyModel) {
        return true;
    }
    else {
        return false;
    }
}
MulNX::Hook::Then SkinModelFixer::OnUpdateMeshGroupMask(MulNX::Hook* hk, RegContext* ctx) {
    auto pS = (CS2::CSkeletonInstance*)ctx->rcx;
    auto pEntity = MulNX::MRead(pS->m_pOwner());
    auto name = pEntity->GetEntityClassName();
    if (std::string_view(name) != "cs2_hudmodel_weapon")return MulNX::Hook::Then::Continue;
    auto hWeapon = MulNX::MRead(pEntity->m_hOwnerEntity());
    auto pWeapon = this->CS2Entitys->GetBaseEntityFromHandle(hWeapon)->As<CS2::C_CSWeaponBase>();
    auto o = this->UseLegacyModel(name, pWeapon);
    if (!o.has_value())return MulNX::Hook::Then::Continue;
    if (*o) {
        ctx->rdx = 2;
    }
    else {
        ctx->rdx = 1;
    }
    return MulNX::Hook::Then::Continue;
}
MulNX::Hook::Then SkinModelFixer::OnPos_NetUpdateMeshGroupMask_GettedValue(MulNX::Hook* hk, RegContext* ctx) {
    auto pModelState = (CS2::CModelState*)ctx->rcx;
    auto pS = pModelState->GetOwner();
    auto pWeapon = MulNX::MRead(pS->m_pOwner())->As<CS2::C_CSWeaponBase>();
    auto name = pWeapon->GetEntityClassName();
    if (!name)return MulNX::Hook::Then::Continue;
    if (!std::string_view(name).starts_with("weapon_"))return MulNX::Hook::Then::Continue;
    auto o = this->UseLegacyModel(name, pWeapon);
    if (!o.has_value())return MulNX::Hook::Then::Continue;
    if (*o) {
        ctx->r8 = 2;
    }
    else {
        ctx->r8 = 1;
    }
    return MulNX::Hook::Then::Continue;
}