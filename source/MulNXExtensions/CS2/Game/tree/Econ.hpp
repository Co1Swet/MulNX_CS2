#pragma once

namespace CS2 {
    class CCSWeaponBaseVData {
    public:
        int32_t* m_nKillAward() { return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::CCSWeaponBaseVData::m_nKillAward); }
    };

    class CEconItemDefinition {
    public:
        virtual ~CEconItemDefinition() = delete;
        virtual void unk001() = 0;
        virtual const char* GetUIToken() = 0;
    };

    class C_EconItemView {
    public:
        uint16_t* m_iItemDefinitionIndex() { return Schema<uint16_t>(this, cs2_dumper::schemas::client_dll::C_EconItemView::m_iItemDefinitionIndex); }
        uint32_t* m_iItemIDHigh() { return Schema<uint32_t>(this, cs2_dumper::schemas::client_dll::C_EconItemView::m_iItemIDHigh); }
        uint32_t* m_iAccountID() { return Schema<uint32_t>(this, cs2_dumper::schemas::client_dll::C_EconItemView::m_iAccountID); }
        bool* m_bInitialized() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::C_EconItemView::m_bInitialized); }
        using name161 = char[161];
        name161* m_szCustomName() { return Schema<name161>(this, cs2_dumper::schemas::client_dll::C_EconItemView::m_szCustomName); }
        name161* m_szCustomNameOverride() { return Schema<name161>(this, cs2_dumper::schemas::client_dll::C_EconItemView::m_szCustomNameOverride); }
        CAttributeList* m_AttributeList() { return Schema<CAttributeList>(this, cs2_dumper::schemas::client_dll::C_EconItemView::m_AttributeList); }

        using GetFromItemID_t = CS2::C_EconItemView* (*) (uint64_t);
        inline static GetFromItemID_t pGetFromItemID = nullptr;
        using GetVData_t = CCSWeaponBaseVData * (*)(C_EconItemView*);
        inline static GetVData_t pGetVData = nullptr;

        virtual ~C_EconItemView() = delete;
        virtual void unk001() = 0;
        virtual void unk002() = 0;
        virtual void unk003() = 0;
        virtual void unk004() = 0;
        virtual void unk005() = 0;
        virtual void unk006() = 0;
        virtual void unk007() = 0;
        virtual void unk008() = 0;
        virtual void unk009() = 0;
        virtual void unk010() = 0;
        virtual void unk011() = 0;
        virtual void unk012() = 0;
        virtual void unk013() = 0;
        virtual void unk014() = 0;
        virtual void unk015() = 0;
        virtual CEconItemDefinition* GetEconItemDefinition() = 0;
    };

    class C_AttributeContainer :public CAttributeManager {
    public:
        C_EconItemView* m_Item() { return Schema<C_EconItemView>(this, cs2_dumper::schemas::client_dll::C_AttributeContainer::m_Item); }
    };
}