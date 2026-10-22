#pragma once
#include <Feature/Model/SkinController/SkinController.hpp>
#include <Mirror/CCStrike15ItemSystem/CCStrike15ItemSystem.hpp>

class SkinModelFixer final :public CSModuleBase {
    CCStrike15ItemSystem* pCCStrike15ItemSystem = nullptr;
    SkinController::CEconItemView_GetPaintKit_t pGetPaintKit = nullptr;

    using UpdateMeshGroupMask_t = void(*)(CS2::CSkeletonInstance* skeletonInstance, uint64_t meshGroupMask);
    UpdateMeshGroupMask_t pUpdateMeshGroupMask = nullptr;

    std::unique_ptr<MulNX::Hook> hkUpdateMeshGroupMask = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_NetUpdateMeshGroupMask_GettedValue = nullptr;

    std::optional<bool> UseLegacyModel(const char* name, CS2::C_CSWeaponBase* pWeapon);
    MulNX::Hook::Then OnUpdateMeshGroupMask(MulNX::Hook* hk, RegContext* ctx);
    MulNX::Hook::Then OnPos_NetUpdateMeshGroupMask_GettedValue(MulNX::Hook* hk, RegContext* ctx);

    bool Init()override;
};