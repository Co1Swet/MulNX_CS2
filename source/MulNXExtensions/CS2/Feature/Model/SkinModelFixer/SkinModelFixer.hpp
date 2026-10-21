#pragma once
#include <Intro/CSModuleBase.hpp>

namespace CS2 {
#pragma pack(push, 1)
    struct CStrike15PaintDef {
        uint32_t    m_nType;                    // +0x00  类型/标志，0 = 无 legacy 材质
        uint8_t     m_pad004[4];                // +0x04
        const char* m_szName;                   // +0x08  名字
        uint8_t     m_pad010[0x34];             // +0x10 -> +0x44
        uint32_t    m_nUnknown044;              // +0x44  未命名
        uint8_t     m_pad048[0x20];             // +0x48 -> +0x68
        uint32_t    m_nUnknown068;              // +0x68  未命名
        uint8_t     m_pad06C[0x41];             // +0x6C -> +0xAD
        bool        m_bUseNormal;               // +0xAD  use_normal
        bool        m_bUseLegacyModel;          // +0xAE  use_legacy_model
        uint8_t     m_pad0AF[0x19];             // +0xAF -> +0xC8
        const char* m_szLegacyMaterialPath;     // +0xC8  显式 legacy 材质路径，可空
    };
#pragma pack(pop)

    static_assert(offsetof(CStrike15PaintDef, m_nType) == 0x00);
    static_assert(offsetof(CStrike15PaintDef, m_szName) == 0x08);
    static_assert(offsetof(CStrike15PaintDef, m_nUnknown044) == 0x44);
    static_assert(offsetof(CStrike15PaintDef, m_nUnknown068) == 0x68);
    static_assert(offsetof(CStrike15PaintDef, m_bUseNormal) == 0xAD);
    static_assert(offsetof(CStrike15PaintDef, m_bUseLegacyModel) == 0xAE);
    static_assert(offsetof(CStrike15PaintDef, m_szLegacyMaterialPath) == 0xC8);
}

class SkinModelFixer final :public CSModuleBase {
    using GetPaintDef_t = CS2::CStrike15PaintDef* (*)(CS2::C_EconItemView* pItemView);
    GetPaintDef_t pGetPaintDef = nullptr;

    using UpdateMeshGroupMask_t = void(*)(CS2::CSkeletonInstance* skeletonInstance, uint64_t meshGroupMask);
    UpdateMeshGroupMask_t pUpdateMeshGroupMask = nullptr;

    std::unique_ptr<MulNX::Hook> hkUpdateMeshGroupMask = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_NetUpdateMeshGroupMask_GettedValue = nullptr;

    MulNX::Hook::Then OnUpdateMeshGroupMask(MulNX::Hook* hk, RegContext* ctx);
    MulNX::Hook::Then OnPos_NetUpdateMeshGroupMask_GettedValue(MulNX::Hook* hk, RegContext* ctx);

    bool Init()override;
};