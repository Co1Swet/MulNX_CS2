#include "SkinModelFixer.hpp"

bool SkinModelFixer::Init() {
    this->pCCStrike15ItemSystem = this->FindModule<CCStrike15ItemSystem>("CCStrike15ItemSystem");

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {
        this->pGetPaintKit = this->FindModule<SkinController>("SkinController")->pGetPaintKit;

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
            .FindRegion(CS2::Signatures::Skin::CSkeletonInstance::Pos_NetUpdateMeshGroupMask_GettedValue).Data();
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

        auto tRegenerateWeaponSkins = this->CS2->client.GetTextRegion().FindRegion(CS2::Signatures::Skin::RegenerateWeaponSkins).Data();
        this->hkRegenerateWeaponSkins = MulNX::Hook::Create(tRegenerateWeaponSkins, [this](MulNX::Hook* hk, RegContext* ctx) {
            try {
                return this->OnRegenerateWeaponSkins(hk, ctx);
            }
            catch (const MulNX::Exception& e) {
                this->LogError(e);
            }
            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkRegenerateWeaponSkins, "RegenerateWeaponSkins");
        });

    return true;
}
std::optional<bool> SkinModelFixer::UseLegacyModel(const char* name, CS2::C_CSWeaponBase* pWeapon) {
    auto pAttributeContainer = pWeapon->m_AttributeManager();
    auto pItem = pAttributeContainer->m_Item();
    auto index = this->pGetPaintKit(pItem);
    auto def = this->pCCStrike15ItemSystem->FindPaintDefByIndex(index);
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
MulNX::Hook::Then SkinModelFixer::OnRegenerateWeaponSkins(MulNX::Hook* hk, RegContext* ctx) {
    auto pPawn = this->CS2Entitys->TryGetObservingPawn();
    if (!pPawn) return MulNX::Hook::Then::Continue;

    auto hHudModelArms = MulNX::MRead(pPawn->m_hHudModelArms());
    auto pArmsEntity = this->CS2Entitys->GetBaseEntityFromHandle(hHudModelArms);
    if (!pArmsEntity) return MulNX::Hook::Then::Continue;

    auto pArmsSkel = reinterpret_cast<CS2::CSkeletonInstance*>(
        MulNX::MRead(pArmsEntity->pGameSceneNode()));
    if (!pArmsSkel) return MulNX::Hook::Then::Continue;

    CS2::C_BaseEntity* pHudWeaponEntity = nullptr;
    CS2::CSkeletonInstance* pHudWeaponSkel = nullptr;

    auto pChild = MulNX::MRead(pArmsSkel->m_pChild());
    while (pChild) {
        auto pOwner = MulNX::MRead(pChild->m_pOwner());
        if (pOwner) {
            auto name = pOwner->GetEntityClassName();
            if (name && std::string_view(name) == "cs2_hudmodel_weapon") {
                pHudWeaponEntity = pOwner;
                pHudWeaponSkel = reinterpret_cast<CS2::CSkeletonInstance*>(pChild);
                break;
            }
        }
        pChild = MulNX::MRead(pChild->m_pNextSibling());
    }
    if (!pHudWeaponSkel || !pHudWeaponEntity)
        return MulNX::Hook::Then::Continue;

    auto hWorldWeapon = MulNX::MRead(pHudWeaponEntity->m_hOwnerEntity());
    auto pWorldWeapon = this->CS2Entitys->GetBaseEntityFromHandle(hWorldWeapon);
    if (!pWorldWeapon) return MulNX::Hook::Then::Continue;

    auto pWorldSkel = reinterpret_cast<CS2::CSkeletonInstance*>(
        MulNX::MRead(pWorldWeapon->pGameSceneNode()));
    if (!pWorldSkel) return MulNX::Hook::Then::Continue;

    auto* pWorldModelState = pWorldSkel->m_modelState();
    auto ppWorldModel = pWorldModelState->m_hModel();

    if (MulNX::MRead(&ppWorldModel->m_pObject) == nullptr) return MulNX::Hook::Then::Continue;

    auto* pHudModelState = pHudWeaponSkel->m_modelState();

    auto* pHudMS = pHudWeaponSkel->m_modelState();
    auto* pWorldMS = pWorldSkel->m_modelState();

    uint64_t worldMask = *pWorldMS->m_MeshGroupMask();
    if (*pHudMS->m_MeshGroupMask() != worldMask) {
        this->pUpdateMeshGroupMask(pHudWeaponSkel, worldMask);
    }

    using RegenerateWeaponSkins_t = uint64_t(*)(void);
    reinterpret_cast<RegenerateWeaponSkins_t>(hk->pMaybeRawFunc)();
    return MulNX::Hook::Then::Return;
}