#pragma once

class CFakeDamageRecordPool {
    static constexpr size_t kMaxRecords = 64;
    CS2::CDamageRecord m_Records[kMaxRecords];
    int32_t        m_nCount = 0;

    CFakeDamageRecordPool() = default;
    CFakeDamageRecordPool(const CFakeDamageRecordPool&) = delete;
    CFakeDamageRecordPool& operator=(const CFakeDamageRecordPool&) = delete;
public:
    static CFakeDamageRecordPool& Get() {
        static CFakeDamageRecordPool s_Instance;
        return s_Instance;
    }

    void Clear() { m_nCount = 0; }
    int32_t Count() const { return m_nCount; }
    CS2::CDamageRecord* Data() { return m_Records; }

    void Add(const CS2::CHandleBase& hDamager,
        const CS2::CHandleBase& hRecipient,
        float flDamage,
        int32_t nNumHits,
        CS2::EKillTypes_t eKillType) {
        if (m_nCount >= kMaxRecords)
            return;

        auto* pRecord = &m_Records[m_nCount++];
        std::memset(pRecord, 0, sizeof(CS2::CDamageRecord));

        pRecord->m_hPlayerControllerDamager.value = hDamager.value;
        pRecord->m_hPlayerControllerRecipient.value = hRecipient.value;
        pRecord->m_flDamage = flDamage;
        pRecord->m_flActualHealthRemoved = flDamage;
        pRecord->m_iNumHits = nNumHits;
        pRecord->m_bIsOtherEnemy = true;
        pRecord->m_killType = eKillType;
    }
};