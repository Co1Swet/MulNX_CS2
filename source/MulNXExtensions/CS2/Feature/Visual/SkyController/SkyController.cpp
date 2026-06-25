#include "SkyController.hpp"
#include <Buildup/MaterialSystem/MaterialSystem.hpp>

void SkyController::Menu(MulNX::UINode* node) {
    ImGui::SeparatorText("天空");
    MulNX::UI::Checkbox("启用天空修改", this->runFlag1);

    // ========== 普通染色（RGBA） ==========
    uint32_t currentColorU32 = this->skyColor.load();
    ImVec4 colorVec4 = ImGui::ColorConvertU32ToFloat4(currentColorU32);
    if (ImGui::ColorEdit4("天空染色", (float*)&colorVec4)) {
        uint32_t newColorU32 = ImGui::ColorConvertFloat4ToU32(colorVec4);
        MulNX::Message msg("Sky/Color/Set"_hash);
        msg.p1.low<uint32_t>() = newColorU32;
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    if (ImGui::Button("重置颜色")) {
        MulNX::Message msg("Sky/Color/Set"_hash);
        msg.p1.low<uint32_t>() = IM_COL32(255, 0, 0, 255);
        this->PublishAsync(std::move(msg));
    }

    // ========== 亮度 ==========
    float brightness = this->brightness.load();
    if (ImGui::SliderFloat("亮度倍率", &brightness, 0.1f, 10.0f, "%.2f")) {
        MulNX::Message msg("Sky/Brightness/Set"_hash);
        msg.p1.low<float>() = brightness;
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    if (ImGui::Button("重置亮度")) {
        MulNX::Message msg("Sky/Brightness/Set"_hash);
        msg.p1.low<float>() = 2.0f;
        this->PublishAsync(std::move(msg));
    }
}

bool SkyController::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto posCallForce = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Sky::Pos_C_EnvSky_VF10_Call_ForceUpdateSkybox).Data();
        auto leaAddr = posCallForce + 2;
        int32_t disp = *reinterpret_cast<int32_t*>(leaAddr + 3);
        auto targetAddr = leaAddr + 7 + disp;

        this->hkForceUpdateSkybox = this->CreateHook("ForceUpdateSkybox", targetAddr, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pEnvSky = (CS2::C_EnvSky*)ctx->rcx;
            return this->HandleForceUpdateSkybox(pEnvSky);
            }).value();
        this->hkForceUpdateSkybox.Attach();
        });

    (*this)
        .SubscribeAsync("Sky/Color/Set")
        .SubscribeAsync("Sky/Brightness/Set");

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {
        this->Menu(node);
        return true;
        });

    this->SendTask("Update", "CSControl", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void SkyController::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Sky/Color/Set"_hash:
        this->skyColor.store(msg.p1.low<uint32_t>());
        break;
    case "Sky/Brightness/Set"_hash:
        this->brightness.store(msg.p1.low<float>());
        break;
    default:
        break;
    }
}

MulNX::Hook::Then SkyController::HandleForceUpdateSkybox(CS2::C_EnvSky* pEnvSky) {
    if (!this->runFlag1.load()) return MulNX::Hook::Then::Continue;

    *pEnvSky->m_vTintColor() = this->skyColor.load();
    *pEnvSky->m_flBrightnessScale() = this->brightness.load();

    return MulNX::Hook::Then::Continue;
}