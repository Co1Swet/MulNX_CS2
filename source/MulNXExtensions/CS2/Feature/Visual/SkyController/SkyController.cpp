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

    this->CS2Con->RegisterCmd("MulNX/Sky/UpdateOnMainThread", [this](auto&&...) {
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

    auto pWantedSkyName = this->wantedSkyName.load();
    if (!pWantedSkyName)return;

    auto skyName = *pWantedSkyName;
    if (skyName.size() == 0) return;

    auto EnsureSky = [&]()->bool {
        if (this->cachedMaterials.find(skyName) != this->cachedMaterials.end())return true;
        if (this->pResourceSystem->PreCache(skyName.c_str()) == 0)return false;
        this->cachedMaterials.insert({ skyName, true });
        return true;
        };

    if (!EnsureSky())return;

    for (int i = 0; i < 2048; i++) {
        auto pEntity = this->CS2->client.GetBaseEntity(i);
        if (!pEntity)continue;
        
        auto name = pEntity->GetName();
        if (_stricmp("env_sky", name.c_str())) continue;

        *(void**)((char*)pEntity + 0x118) = nullptr;

        auto pEntry = this->hkForceUpdateSkybox->GetHookTarget();
        using ForceUpdateSkybox_t = void* (*)(void*);
        reinterpret_cast<ForceUpdateSkybox_t>(pEntry)(pEntity);// 先到虚函数入口，然后会跳到我们的钩子，再继续原有流程
    }
}

MulNX::Hook::Then SkyController::HandleForceUpdateSkybox(CS2::C_EnvSky* pEnvSky) {
    if (!this->enable.load()) return MulNX::Hook::Then::Continue;

    //*pEnvSky->m_vTintColor() = this->skyColor.load();
    //*pEnvSky->m_flBrightnessScale() = this->brightness.load();
    auto pB = pEnvSky->m_flBrightnessScale();
    *pB *= 0.1;

    // 读取当前材质
    auto nowMat = std::bit_cast<CMaterial2**>(*pEnvSky->m_hSkyMaterial());
    auto nowName = (*nowMat)->GetName();

    // 查找新材质
    CMaterial2** newMat = nullptr;
    auto p = this->wantedSkyName.load();
    if (!p)return MulNX::Hook::Then::Continue;
    auto newName = *p;
    this->pMaterialSystem->FindMaterial(&newMat, newName.c_str());

    if (!newMat)return MulNX::Hook::Then::Continue;
    if (nowMat == newMat) return MulNX::Hook::Then::Continue;

    // 手动增加引用计数
    auto pCount = (uint32_t*)((char*)newMat + 0x20);
    (*pCount)++;
    // 替换实体中的材质指针
    *pEnvSky->m_hSkyMaterial() = std::bit_cast<uint64_t>(newMat);

    return MulNX::Hook::Then::Continue;
}