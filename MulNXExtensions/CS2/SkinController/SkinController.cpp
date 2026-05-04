#include "SkinController.hpp"
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

bool SkinController::Init() {
    if (!MulNXInfo::IsDebugVersion) {
        this->ISys().LogWarning("此模块处于禁用状态！因为其仅在调试版本开放！");
        return true;
    }
    
    this->SendTask("CSControl", [this]()->bool {
        this->Main();
        return true;
        });

    auto target = this->CS2()->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::RegenerateWeaponSkins);
    this->re = (rebuild)target.Data();
    // static auto hkSkin = MulNX::Hook::Create(target.Data(), 0, false, [this](RegContext* ctx, MulNX::Hook* Hook) {
    //     return MulNX::Hook::Then::Continue;
    //     }).value();
    // hkSkin->Attach();
    return true;
}

void SkinController::Main() {
    if (!this->pInputSystem->CheckComboClick('J', 2)) return;

    // 计算总偏移：m_AttributeManager + m_Item + m_AttributeList + m_Attributes
    uint16_t totalOffset = cs2_dumper::schemas::client_dll::C_EconEntity::m_AttributeManager
        + cs2_dumper::schemas::client_dll::C_AttributeContainer::m_Item
        + cs2_dumper::schemas::client_dll::C_EconItemView::m_AttributeList
        + cs2_dumper::schemas::client_dll::CAttributeList::m_Attributes;

    // 获取 RegenerateWeaponSkins 函数入口地址
    uintptr_t fnAddr = reinterpret_cast<uintptr_t>(this->re);

    // 在 fnAddr + 0x52 处写入 totalOffset（2 字节）
    uint8_t* patchAddr = (uint8_t*)this->re + 0x52;
    DWORD oldProtect;

    // 1. 修改页面保护为可读写
    if (VirtualProtect(patchAddr, sizeof(uint16_t), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        // 2. 写入新偏移
        *(uint16_t*)patchAddr = totalOffset;

        // 3. 恢复原有保护
        VirtualProtect(patchAddr, sizeof(uint16_t), oldProtect, &oldProtect);
    }

    auto localPlayer = this->CS2()->client.GetLocalPlayerPawn();
    auto weaponServe = MulNX::MRead(localPlayer->pWeaponServices());
    auto hWeapon = MulNX::MRead(weaponServe->hActiveWeapon());
    auto pWeapon = this->CS2()->client.GetBaseEntityFromHandle(hWeapon)->As<CS2::C_CSWeaponBase>();
    auto pManager = pWeapon->m_AttributeManager();
    auto item = pManager->m_Item();
    MulNX::MWrite(item->m_iItemIDHigh(), 0xFFFFFFFF);
    MulNX::MWrite(pWeapon->m_nFallbackPaintKit(), 445);

    auto list = item->m_AttributeList();
    CS2::C_UtlVectorEmbeddedNetworkVar<CS2::CEconItemAttribute>* attributes = list->m_Attributes();
    attributes->m_nSize = 0;
    attributes->m_pData = nullptr;

    CS2::CEconItemAttribute* paintAttr = (CS2::CEconItemAttribute*)malloc(CS2::CEconItemAttribute::ofsize);
    *paintAttr->m_iAttributeDefinitionIndex() = 6;          // paint attribute ID
    *paintAttr->m_flValue() = 445.0f;        // paint kit ID as float
    *paintAttr->m_flInitialValue() = 445.0f;

    attributes->m_nSize = 1;
    attributes->m_pData = paintAttr;

    this->re(nullptr);

    return;

}