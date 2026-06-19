#include "SkinController.hpp"
#include <MulNX/Base/UI/UI.hpp>


void SkinController::Window(MulNX::UINode* node) {
    ImGui::SeparatorText(I18n("skin.changer").c_str());
    MulNX::UI::SliderInt(I18n("skin.target.index").c_str(), this->targetIndex, 0, 10000);
    MulNX::UI::Checkbox(I18n("skin.target.legacy").c_str(), this->legacyModel);
    if (ImGui::Button(I18n("skin.apply").c_str())) {
        this->PublishAsync("Skin/Apply"_hash);
    }
}

bool SkinController::Init() {
    (*this)
        .SubscribeSync("Hook/OnSetupView", [this](MulNX::Message& msg) {this->Update();})
        .SubscribeAsync("Skin/Apply")
        ;

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::RegenerateWeaponSkins);
        this->regenerateWeaponSkins = (RegenerateWeaponSkins)target.Data();
        // static auto hkSkin = MulNX::Hook::Create(target.Data(), 0, false, [this](RegContext* ctx, MulNX::Hook* Hook) {
        //     return MulNX::Hook::Then::Continue;
        //     }).value();
        // hkSkin->Attach();

        // 计算总偏移：m_AttributeManager + m_Item + m_AttributeList + m_Attributes
        uint16_t totalOffset = cs2_dumper::schemas::client_dll::C_EconEntity::m_AttributeManager
            + cs2_dumper::schemas::client_dll::C_AttributeContainer::m_Item
            + cs2_dumper::schemas::client_dll::C_EconItemView::m_AttributeList
            + cs2_dumper::schemas::client_dll::CAttributeList::m_Attributes;

        // 在 + 0x52 处写入 totalOffset（2 字节）
        uint8_t* patchAddr = (uint8_t*)this->regenerateWeaponSkins + 0x52;

        DWORD oldProtect;
        if (VirtualProtect(patchAddr, sizeof(uint16_t), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            *(uint16_t*)patchAddr = totalOffset;
            VirtualProtect(patchAddr, sizeof(uint16_t), oldProtect, &oldProtect);
        }
        else {
            MulNX::ErrorTerminate(I18n("skin.controller.hook.fail"));
        }
        this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});
        });

    return true;
}

void SkinController::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Skin/Apply"_hash: {
        this->Apply();
        break;
    }
    }
}

void SkinController::Apply() {
    auto iTargetIndex = this->targetIndex.load(std::memory_order_acquire);
    auto fTargetIndex = static_cast<float>(iTargetIndex);
    auto isLegacy = this->legacyModel.load(std::memory_order_acquire);

    // 1. 获取武器及关键字段
    auto localPlayer = this->CS2->client.TryGetObservingPawn();
    auto weaponServe = MulNX::MRead(localPlayer->pWeaponServices());
    auto hWeapon = MulNX::MRead(weaponServe->hActiveWeapon());
    auto pWeapon = this->CS2->client.GetBaseEntityFromHandle(hWeapon)->As<CS2::C_CSWeaponBase>();
    auto pManager = pWeapon->m_AttributeManager();
    auto item = pManager->m_Item();
    auto list = item->m_AttributeList();
    CS2::C_UtlVectorEmbeddedNetworkVar<CS2::CEconItemAttribute>* attributes = list->m_Attributes();

    auto pStatu = reinterpret_cast<CS2::CSkeletonInstance*>(MulNX::MRead(pWeapon->pGameSceneNode()))->m_modelState();
    auto pMesh = pStatu->m_MeshGroupMask();

    // 2. 保存原始状态
    auto origItemIDHigh = MulNX::MRead(item->m_iItemIDHigh());
    auto origFallbackPaintKit = MulNX::MRead(pWeapon->m_nFallbackPaintKit());
    const uint32_t origAttrSize = attributes->m_nSize;
    CS2::CEconItemAttribute* origAttrData = attributes->m_pData;
    auto origMesh = MulNX::MRead(pMesh);

    // 3. 定义一个自动恢复的清理类（保证即使异常也能恢复）
    struct RestoreGuard {
        decltype(item) item;
        uint32_t savedItemIDHigh;
        decltype(pWeapon) pWeapon;
        int32_t savedPaintKit;
        CS2::C_UtlVectorEmbeddedNetworkVar<CS2::CEconItemAttribute>* attributes;
        uint32_t savedAttrSize;
        CS2::CEconItemAttribute* savedAttrData;
        CS2::CEconItemAttribute* newAttr;   // 我们分配的临时内存
        uint64_t* pMesh;                    // Mesh 指针
        uint64_t savedMesh;                 // 原始 Mesh 值

        ~RestoreGuard() {
            // 恢复属性列表
            attributes->m_pData = savedAttrData;
            attributes->m_nSize = savedAttrSize;
            free(newAttr);

            // 恢复武器基本状态
            MulNX::MWrite(item->m_iItemIDHigh(), savedItemIDHigh);
            MulNX::MWrite(pWeapon->m_nFallbackPaintKit(), savedPaintKit);

            // 恢复 Mesh（如果存在）
            if (pMesh) MulNX::MWrite(pMesh, savedMesh);
        }
    };

    // 4. 分配新属性（用于临时应用皮肤）
    CS2::CEconItemAttribute* paintAttr = (CS2::CEconItemAttribute*)malloc(CS2::CEconItemAttribute::ofsize);
    MulNX::MWrite(paintAttr->m_iAttributeDefinitionIndex(), (uint16_t)6);
    MulNX::MWrite(paintAttr->m_flValue(), fTargetIndex);
    MulNX::MWrite(paintAttr->m_flInitialValue(), fTargetIndex);

    // char buffer[200];
    // memcpy(buffer, item->m_szCustomName(), 199);
    // memcpy(buffer, item->m_szCustomNameOverride(), 199);
    // memcpy(item->m_szCustomNameOverride(), "test test name", 15);
    // memcpy(buffer, item->m_szCustomNameOverride(), 199);
    //memcpy(item->m_szCustomNameOverride(), "test", 5);

    RestoreGuard guard{
        item, origItemIDHigh,
        pWeapon, origFallbackPaintKit,
        attributes, origAttrSize, origAttrData,
        paintAttr
    };

    // 5. 修改为目标皮肤状态
    MulNX::MWrite(item->m_iItemIDHigh(), 0xFFFFFFFF);
    MulNX::MWrite(pWeapon->m_nFallbackPaintKit(), iTargetIndex);
    attributes->m_nSize = 1;
    attributes->m_pData = paintAttr;

    // 6. 设置模型掩码（老模型=2 或 新=1）
    if (pMesh) {
        MulNX::MWrite(pMesh, isLegacy ? (uint64_t)2 : (uint64_t)1);
    }

    // 6. 应用皮肤
    this->regenerateWeaponSkins(nullptr);

    // 7. 函数结束时 guard 析构，自动恢复所有原始值并释放 paintAttr
}