#pragma once
#include <Feature/Model/SkinController/SkinController.hpp>
#include <Mirror/CCStrike15ItemSystem/CCStrike15ItemSystem.hpp>

class SkinModelFixer final :public CSModuleBase {
    CCStrike15ItemSystem* pCCStrike15ItemSystem = nullptr;
    SkinController::CEconItemView_GetPaintKit_t pGetPaintKit = nullptr;

    using UpdateMeshGroupMask_t = void(*)(CS2::CSkeletonInstance* skeletonInstance, uint64_t meshGroupMask);
    UpdateMeshGroupMask_t pUpdateMeshGroupMask = nullptr;

    std::unique_ptr<MulNX::Hook> hkUpdateMeshGroupMask = nullptr;
    std::unique_ptr<MulNX::Hook> hkNetUpdateMeshGroupMask = nullptr;
    std::unique_ptr<MulNX::Hook> hkRegenerateWeaponSkins = nullptr;

    std::optional<bool> UseLegacyModel(CS2::C_CSWeaponBase* pWeapon);
    MulNX::Hook::Then OnUpdateMeshGroupMask(MulNX::Hook* hk, RegContext* ctx);
    MulNX::Hook::Then OnNetUpdateMeshGroupMask(MulNX::Hook* hk, RegContext* ctx);
    MulNX::Hook::Then OnRegenerateWeaponSkins(MulNX::Hook* hk, RegContext* ctx);

    void CheckHudWeaponMesh(CS2::C_CSPlayerPawn* pPawn);

    bool Init()override;
};