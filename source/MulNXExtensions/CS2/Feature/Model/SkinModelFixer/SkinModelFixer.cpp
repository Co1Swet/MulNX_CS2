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

        auto tNetUpdateMeshGroupMask = this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Skin::CSkeletonInstance::CModelState::NetUpdateMeshGroupMask).Data();
        this->hkNetUpdateMeshGroupMask = MulNX::Hook::Create(tNetUpdateMeshGroupMask, [this](MulNX::Hook* hk, RegContext* ctx) {
            try {
                return this->OnNetUpdateMeshGroupMask(hk, ctx);
            }
            catch (const MulNX::Exception& e) {
                this->LogError(e);
            }
            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkNetUpdateMeshGroupMask, "NetUpdateMeshGroupMask");

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
std::optional<bool> SkinModelFixer::UseLegacyModel(CS2::C_CSWeaponBase* pWeapon) {
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
    auto o = this->UseLegacyModel(pWeapon);
    if (!o.has_value())return MulNX::Hook::Then::Continue;
    if (*o) {
        ctx->rdx = 2;
    }
    else {
        ctx->rdx = 1;
    }
    return MulNX::Hook::Then::Continue;
}
MulNX::Hook::Then SkinModelFixer::OnNetUpdateMeshGroupMask(MulNX::Hook* hk, RegContext* ctx) {
    auto pModelState = (CS2::CModelState*)ctx->rcx;
    auto pWeaponSkeletonInstance = pModelState->GetOwner();
    auto pWeapon = MulNX::MRead(pWeaponSkeletonInstance->m_pOwner())->As<CS2::C_CSWeaponBase>();
    auto name = pWeapon->GetEntityClassName();
    if (!name)return MulNX::Hook::Then::Continue;
    if (!std::string_view(name).starts_with("weapon_"))return MulNX::Hook::Then::Continue;

    auto h = MulNX::MRead(pWeapon->m_hOwnerEntity());
    auto pEntity = this->CS2Entitys->GetBaseEntityFromHandle(h)->As<CS2::C_CSPlayerPawn>();
    if (pEntity) {
        this->CheckHudWeaponMesh(pEntity);
    }

    auto o = this->UseLegacyModel(pWeapon);
    if (!o.has_value())return MulNX::Hook::Then::Continue;
    uint64_t mask;
    if (*o) {
        mask = 2;
    }
    else {
        mask = 1;
    }
    using NetUpdateMeshGroupMask_t = void(*)(CS2::CModelState*, uint64_t*);
    reinterpret_cast<NetUpdateMeshGroupMask_t>(hk->pMaybeRawFunc)(pModelState, &mask);
    return MulNX::Hook::Then::Return;
}
MulNX::Hook::Then SkinModelFixer::OnRegenerateWeaponSkins(MulNX::Hook* hk, RegContext* ctx) {
    for (int i = 0;i < 64;++i) {
        auto pCtrl = this->CS2Entitys->GetBaseEntity(i)->As<CS2::CCSPlayerController>();
        if (!pCtrl || !pCtrl->IsPlayerController())continue;
        auto hPawn = MulNX::MRead(pCtrl->m_hPawn());
        auto pPawn = this->CS2Entitys->GetBaseEntityFromHandle(hPawn)->As<CS2::C_CSPlayerPawn>();
        if (!pPawn || !pPawn->IsPlayerPawn())continue;
        this->CheckHudWeaponMesh(pPawn);
    }
    return MulNX::Hook::Then::Continue;
}
void SkinModelFixer::CheckHudWeaponMesh(CS2::C_CSPlayerPawn* pPawn) {
    auto hHudModelArms = MulNX::MRead(pPawn->m_hHudModelArms());
    auto pArmsEntity = this->CS2Entitys->GetBaseEntityFromHandle(hHudModelArms);
    if (!pArmsEntity)return;

    auto pArmsSkel = static_cast<CS2::CSkeletonInstance*>(MulNX::MRead(pArmsEntity->pGameSceneNode()));
    if (!pArmsSkel)return;

    CS2::C_BaseEntity* pHudWeaponEntity = nullptr;
    CS2::CSkeletonInstance* pHudWeaponSkel = nullptr;

    for (auto pChild = MulNX::MRead(pArmsSkel->m_pChild());
        pChild != nullptr;
        pChild = MulNX::MRead(pChild->m_pNextSibling())) {

        auto pOwner = MulNX::MRead(pChild->m_pOwner());
        if (!pOwner)continue;
        auto name = pOwner->GetEntityClassName();
        if (name && std::string_view(name) == "cs2_hudmodel_weapon") {
            pHudWeaponEntity = pOwner;
            pHudWeaponSkel = static_cast<CS2::CSkeletonInstance*>(pChild);
            break;
        }
    }
    if (!pHudWeaponSkel || !pHudWeaponEntity)return;

    auto hWorldWeapon = MulNX::MRead(pHudWeaponEntity->m_hOwnerEntity());
    auto pWorldWeapon = this->CS2Entitys->GetBaseEntityFromHandle(hWorldWeapon);
    if (!pWorldWeapon)return;

    auto pWorldSkel = static_cast<CS2::CSkeletonInstance*>(MulNX::MRead(pWorldWeapon->pGameSceneNode()));
    if (!pWorldSkel)return;

    auto* pWorldModelState = pWorldSkel->m_modelState();
    auto ppWorldModel = pWorldModelState->m_hModel();

    if (MulNX::MRead(&ppWorldModel->m_pObject) == nullptr)return;

    auto* pHudModelState = pHudWeaponSkel->m_modelState();
    auto* pWorldMS = pWorldSkel->m_modelState();

    uint64_t worldMask = MulNX::MRead(pWorldMS->m_MeshGroupMask());
    if (MulNX::MRead(pHudModelState->m_MeshGroupMask()) == worldMask)return;
    this->pUpdateMeshGroupMask(pHudWeaponSkel, worldMask);
}