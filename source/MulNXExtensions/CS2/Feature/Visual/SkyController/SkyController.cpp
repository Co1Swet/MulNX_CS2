#include "SkyController.hpp"
#include <Intro/HookConsole/HookConsole.hpp>
#include <Buildup/MaterialSystem/MaterialSystem.hpp>

void SkyController::Menu() {
    ImGui::SeparatorText("天空");
    // RGB(A)
    auto oColor = this->skyColor.load();
    uint32_t currentColorU32 = oColor ? oColor.value() : 0;
    if (!oColor)ImGui::Text("当前未设置天空调色");
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
        this->PublishAsync("Sky/Color/Reset"_hash);
    }

    // 亮度
    auto oBright = this->brightScale.load();
    float brightness = oBright ? oBright.value() : 1.0f;
    if (!oBright)ImGui::Text("当前未设置亮度倍率");
    if (ImGui::SliderFloat("亮度倍率", &brightness, 0.01f, 100.0f)) {
        MulNX::Message msg("Sky/Brightness/Set"_hash);
        auto&& [brightnessRef] = msg.Access<float>();
        brightnessRef = brightness;
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    if (ImGui::Button("重置亮度")) {
        this->PublishAsync("Sky/BrightScale/Reset"_hash);
    }

    if (ImGui::BeginListBox("双击以更换天空")) {
        std::shared_lock lock(this->smutex);
        for (size_t i = 0; i < this->skyNames.size(); ++i) {
            const std::string& name = this->skyNames[i];
            ImGui::PushID(static_cast<int>(i));
            ImGui::Selectable(name.c_str());
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Sky/materials/Set"_hash);
                rp->str1 = name;
                this->PublishAsync(std::move(msg));
            }
            ImGui::PopID();
        }
        ImGui::EndListBox();
    }
    auto pName = this->wantedSkyName.load();
    ImGui::Text(std::format("目标天空：{}", pName ? pName->c_str() : "无").c_str());
    if (ImGui::Button("刷新天空")) {
        this->AsyncCommand("MulNX/Sky/UpdateOnMainThread");
    }
    ImGui::SameLine();
    if (ImGui::Button("恢复天空")) {
        this->PublishAsync("Sky/materials/Reset"_hash);
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

    auto dir = this->PathGet("Game");
    try {
        YAML::Node config = YAML::LoadFile((dir / "Game.yaml").string());
        if (config["skyNames"]) {
            for (const auto& node : config["skyNames"]) {
                std::string name = node.as<std::string>();
                auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Sky/materials/ReportNew"_hash);
                rp->str1 = name;
                this->PublishAsync(std::move(msg));
            }
        }
    }
    catch (const std::exception& e) {
        this->LogError(std::format("解析 Game.yaml 失败: {}", e.what()));
    }

    (*this)
        .SubscribeAsync("Sky/Color/Set")
        .SubscribeAsync("Sky/Color/Reset")
        .SubscribeAsync("Sky/BrightScale/Set")
        .SubscribeAsync("Sky/BrightScale/Reset")
        .SubscribeAsync("Sky/materials/Set")
        .SubscribeAsync("Sky/materials/Reset")
        .SubscribeAsync("Sky/materials/ReportNew")
        ;

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
    case "Sky/Color/Reset"_hash: {
        this->skyColor.store(std::nullopt);
        break;
    }
    case "Sky/BrightScale/Set"_hash: {
        auto&& [brightScale] = msg.Access<float>();
        this->brightScale.store(brightScale);
        break;
    }
    case "Sky/BrightScale/Reset"_hash: {
        this->brightScale.store(std::nullopt);
        break;
    }
    case "Sky/materials/Set"_hash: {
        this->OnMsgSetMaterial(msg);
        break;
    }
    case "Sky/materials/Reset"_hash: {
        this->wantedSkyName.store(nullptr);
        this->AsyncCommand("MulNX/Sky/UpdateOnMainThread");
        break;
    }
    case "Sky/materials/ReportNew"_hash: {
        this->OnMsgReportNew(msg);
        break;
    }
    default:
        break;
    }
}

void SkyController::OnMsgSetMaterial(MulNX::Message& msg) {
    auto p = msg.asp.get<MulNX::NetExt>();
    if (!p) {
        this->LogError("尝试设置天空，而未携带NetExt");
        return;
    }
    if (p->str1.empty()) {
        this->LogError("尝试设置天空，而未携带天空名于str1");
        return;
    }
    this->LogSucc(std::format("目标天空设置为：{}", p->str1));
    this->wantedSkyName.store(std::make_shared<const std::string>(p->str1));
    this->AsyncCommand("MulNX/Sky/UpdateOnMainThread");
}

void SkyController::OnMsgReportNew(MulNX::Message& msg) {
    auto name = msg.asp.get<MulNX::NetExt>()->str1;
    if (name.ends_with(".vmat_c")) {
        // .vmat_c -> .vmat
        name.erase(name.size() - 2);
    }
    std::unique_lock lock(this->smutex);
    auto it = std::ranges::find(this->skyNames, name);
    if (it != this->skyNames.end())return;
    this->LogSucc(std::format("已记录新天空：{}", name));
    this->skyNames.push_back(std::move(name));
}

void SkyController::HandleOnCmd() {
    auto pWantedSkyName = this->wantedSkyName.load();
    if (!pWantedSkyName)return;

    auto skyName = *pWantedSkyName;
    if (skyName.size() == 0) return;

    auto EnsureSky = [&]()->bool {
        CMaterial2** ensureSky = nullptr;
        this->pMaterialSystem->FindMaterial(&ensureSky, skyName.c_str());
        if (!ensureSky)return false;
        auto name = (*ensureSky)->GetName();
        if (!name)return false;
        if (std::string(name) == "materials/error.vmat")return false;
        return true;
        };

    if (!EnsureSky()) {
        if (this->pResourceSystem->PreCache(skyName.c_str())) {
            this->LogSucc(std::format("天空缓存成功：{}", skyName));
        }
        else {
            this->wantedSkyName.store(nullptr);
            this->LogError(std::format("天空未缓存且加载失败：{}", skyName));
            return;
        }
    }

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
    if (auto oColor = this->skyColor.load()) {
        *pEnvSky->m_vTintColor() = oColor.value();
    }
    if (auto oBright = this->brightScale.load()) {
        *pEnvSky->m_flBrightnessScale() *= oBright.value();
    }

    // 读取当前材质
    auto nowMat = std::bit_cast<CMaterial2**>(*pEnvSky->m_hSkyMaterial());
    auto nowName = (*nowMat)->GetName();

    this->LogInfo(std::format("当前天空：{}", nowName));

    [&]()->void {
        std::shared_lock lock(this->smutex);
        auto it = std::ranges::find(this->skyNames, nowName);
        if (it != this->skyNames.end())return;
        lock.unlock();
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Sky/materials/ReportNew"_hash);
        rp->str1 = nowName;
        this->PublishAsync(std::move(msg));
        }();

    // 查找新材质
    auto p = this->wantedSkyName.load();
    if (!p)return MulNX::Hook::Then::Continue;

    CMaterial2** newMat = nullptr;
    auto newName = *p;
    this->pMaterialSystem->FindMaterial(&newMat, newName.c_str());

    // 对比
    if (!newMat)return MulNX::Hook::Then::Continue;
    if (*nowMat == *newMat)
        return MulNX::Hook::Then::Continue;

    // 手动增加引用计数
    auto pCount = (uint32_t*)((char*)newMat + 0x20);
    (*pCount)++;
    // 替换实体中的材质指针
    *pEnvSky->m_hSkyMaterial() = std::bit_cast<uint64_t>(newMat);

    this->LogInfo(std::format("天空替换至：{}", newName));

    return MulNX::Hook::Then::Continue;
}