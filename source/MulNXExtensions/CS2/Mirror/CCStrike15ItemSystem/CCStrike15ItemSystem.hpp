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

class CCStrike15ItemSystem final :public CSModuleBase {
    using GetInner_t = uintptr_t(*)(void);
    GetInner_t pGetInner = nullptr;
    using FindPaintDefByIndex_t = CS2::CStrike15PaintDef* (*)(uintptr_t pInner, int index);
    FindPaintDefByIndex_t pFindPaintDefByIndex = nullptr;

    bool Init()override;
public:
    CS2::CStrike15PaintDef* FindPaintDefByIndex(int index);
};