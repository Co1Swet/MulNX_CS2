#pragma once
#include <Feature/Model/SkinController/SkinController.hpp>

class SkinModelFixer final :public CSModuleBase {
    SkinController::GetPaintDef_t pGetPaintDef = nullptr;

    using UpdateMeshGroupMask_t = void(*)(CS2::CSkeletonInstance* skeletonInstance, uint64_t meshGroupMask);
    UpdateMeshGroupMask_t pUpdateMeshGroupMask = nullptr;

    std::unique_ptr<MulNX::Hook> hkUpdateMeshGroupMask = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_NetUpdateMeshGroupMask_GettedValue = nullptr;

    MulNX::Hook::Then OnUpdateMeshGroupMask(MulNX::Hook* hk, RegContext* ctx);
    MulNX::Hook::Then OnPos_NetUpdateMeshGroupMask_GettedValue(MulNX::Hook* hk, RegContext* ctx);

    bool Init()override;
};