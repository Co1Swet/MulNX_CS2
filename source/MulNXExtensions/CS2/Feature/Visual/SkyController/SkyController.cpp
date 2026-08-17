#include "SkyController.hpp"
#include <Intro/HookConsole/HookConsole.hpp>
#include <Buildup/MaterialSystem/MaterialSystem.hpp>

void SkyController::Menu() {
    ImGui::SeparatorText("天空");
    MulNX::UI::Checkbox("启用天空修改", this->enable);

    // ========== 普通染色（RGBA） ==========
    uint32_t currentColorU32 = this->skyColor.load();
    ImVec4 colorVec4 = ImGui::ColorConvertU32ToFloat4(currentColorU32);
    if (ImGui::ColorEdit4("天空染色", (float*)&colorVec4)) {
        uint32_t newColorU32 = ImGui::ColorConvertFloat4ToU32(colorVec4);
        MulNX::Message msg("Sky/Color/Set"_hash);
        auto&& [colorRef] = msg.Access<uint32_t>();
        colorRef = newColorU32;
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    if (ImGui::Button("重置颜色")) {
        MulNX::Message msg("Sky/Color/Set"_hash);
        auto&& [colorRef] = msg.Access<uint32_t>();
        colorRef = IM_COL32(255, 0, 0, 255);
        this->PublishAsync(std::move(msg));
    }

    // ========== 亮度 ==========
    float brightness = this->brightness.load();
    if (ImGui::SliderFloat("亮度倍率", &brightness, 0.1f, 10.0f, "%.2f")) {
        MulNX::Message msg("Sky/Brightness/Set"_hash);
        auto&& [brightnessRef] = msg.Access<float>();
        brightnessRef = brightness;
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    if (ImGui::Button("重置亮度")) {
        MulNX::Message msg("Sky/Brightness/Set"_hash);
        auto&& [brightnessRef] = msg.Access<float>();
        brightnessRef = 2.0f;
        this->PublishAsync(std::move(msg));
    }
}

bool SkyController::Init() {
    this->pResourceSystem = this->FindModule<ResourceSystem>("ResourceSystem");
    this->pMaterialSystem = this->FindModule<MaterialSystem>("MaterialSystem");

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto posCallForce = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Sky::Pos_C_EnvSky_VF10_Call_ForceUpdateSkybox).Data();
        auto leaAddr = posCallForce + 2;
        int32_t disp = *reinterpret_cast<int32_t*>(leaAddr + 3);
        auto targetAddr = leaAddr + 7 + disp;

        this->hkForceUpdateSkybox = MulNX::Hook::Create(targetAddr, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pEnvSky = (CS2::C_EnvSky*)ctx->rcx;
            return this->HandleForceUpdateSkybox(pEnvSky);
            }).value();
        this->RegisterAttachHook(this->hkForceUpdateSkybox, "ForceUpdateSkybox");
        });

    (*this)
        .SubscribeAsync("Sky/Color/Set")
        .SubscribeAsync("Sky/Brightness/Set");

    this->CS2Con->RegisterCmd("MulNX/Sky/Update", [this](auto&&...) {
        this->HandleOnCmd();
        });

    this->UIRegisterCallback("UI.3DVision", [this](auto&&...) {return this->Menu();});

    this->SendTask("Update", "CSControl", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void SkyController::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Sky/Color/Set"_hash: {
        auto&& [color] = msg.Access<uint32_t>();
        this->skyColor.store(color);
        break;
    }

    case "Sky/Brightness/Set"_hash: {
        auto&& [brightness] = msg.Access<float>();
        this->brightness.store(brightness);
        break;
    }

    default:
        break;
    }
}

void SkyController::HandleOnCmd() {
    std::unique_lock lock(this->smutex);
    if (this->currentSkyName.size() != 0) {
        if (this->cachedMaterials.find(this->currentSkyName) == this->cachedMaterials.end()) {
            if (0 != this->pResourceSystem->PreCache(this->currentSkyName.c_str())) {
                this->cachedMaterials.insert({ this->currentSkyName, true });
                this->nextUpdateSkyboxNeedOverride.store(true);
            }
        }
    }

    for (int i = 0; i < 2048; i++) {
        auto pEntity = this->CS2->client.GetBaseEntity(i);
        if(!pEntity)continue;
        auto name = pEntity->GetName();

        if (_stricmp("env_sky", name.c_str())) continue;

        *(void**)((unsigned char*)pEntity + 0x118) = nullptr;

        auto pEntry = this->hkForceUpdateSkybox->GetHookTarget();
        using ForceUpdateSkybox_t = void* (*)(void*);
        reinterpret_cast<ForceUpdateSkybox_t>(pEntry)(pEntity);// 先到虚函数入口，然后会跳到我们的钩子，再继续原有流程
    }
}

MulNX::Hook::Then SkyController::HandleForceUpdateSkybox(CS2::C_EnvSky* pEnvSky) {
    if (!this->enable.load()) return MulNX::Hook::Then::Continue;

    this->LogWarning(pEnvSky->GetName());

    // 查找新材质
    CMaterial2** newMat = nullptr;
    this->pMaterialSystem->FindMaterial(&newMat, this->currentSkyName.c_str());

    if (0 != newMat) {
        auto now = std::bit_cast<CMaterial2**>(*pEnvSky->m_hSkyMaterial());
        if (now != newMat) {
            
        }
        // 手动增加引用计数
        auto counter = *(uint32_t*)((unsigned char*)newMat + 0x20);
        counter = counter + 1;
        *(uint32_t*)((unsigned char*)newMat + 0x20) = counter;
        // 替换实体中的材质指针
        *pEnvSky->m_hSkyMaterial() = std::bit_cast<uint64_t>(newMat);
    }

    if (this->nextUpdateSkyboxNeedOverride.load()) {
        
    }

    //*pEnvSky->m_vTintColor() = this->skyColor.load();
    //*pEnvSky->m_flBrightnessScale() = this->brightness.load();

    return MulNX::Hook::Then::Continue;
}