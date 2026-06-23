#include "GlowController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/PlayerHub/PlayerHub.hpp>
#include <MulNXExtensions/CS2/HookConsole/HookConsole.hpp>

void GlowController::HubPlayer(MulNX::UINode* node) {
    std::shared_lock lock(this->smutex);
    auto uid = this->Hub->currentSteamId.load(std::memory_order_acquire);

    // 1. 获取当前为该玩家设置的颜色（若存在），否则使用默认白色
    uint32_t currentColorU32 = IM_COL32(255, 255, 255, 255); // 默认白色
    auto it = this->playerColors.find(uid);
    if (it != this->playerColors.end()) {
        currentColorU32 = it->second;
    }
    else {
        ImGui::Text("当前玩家没有自定义发光颜色，使用默认颜色");
    }

    // 2. 将 uint32_t 颜色转换为 ImVec4，以便使用 ImGui 颜色编辑器
    ImVec4 colorVec4 = ImGui::ColorConvertU32ToFloat4(currentColorU32);

    if (ImGui::ColorEdit4("发光颜色修改", (float*)&colorVec4)) {
        // 颜色被修改后，转换回 uint32_t 并保存到 playerColors
        uint32_t newColorU32 = ImGui::ColorConvertFloat4ToU32(colorVec4);
        MulNX::Message msg("Glow/Player/Set"_hash);
        msg.p1.as<Steam64UID>() = uid;
        msg.p2.low<uint32_t>() = newColorU32;
        this->PublishAsync(std::move(msg));
    }

    ImGui::SameLine();
    if (ImGui::Button("重置发光颜色")) {
        MulNX::Message msg("Glow/Player/Clear"_hash);
        msg.p1.as<Steam64UID>() = uid;
        this->PublishAsync(std::move(msg));
    }
}
void GlowController::HubTeam(MulNX::UINode* node) {
    std::shared_lock lock(this->smutex);
    auto team = this->Hub->currentTeam.load(std::memory_order_acquire);

    uint32_t currentColorU32 = IM_COL32(255, 255, 255, 255); // 默认白色
    auto it = this->teamColors.find(team);
    if (it != this->teamColors.end()) {
        currentColorU32 = it->second;
    }
    else {
        ImGui::Text("当前队伍没有自定义发光颜色，使用默认颜色");
    }

    ImVec4 colorVec4 = ImGui::ColorConvertU32ToFloat4(currentColorU32);

    if (ImGui::ColorEdit4("发光颜色修改", (float*)&colorVec4)) {
        uint32_t newColorU32 = ImGui::ColorConvertFloat4ToU32(colorVec4);
        MulNX::Message msg("Glow/Team/Set"_hash);
        msg.p1.low<uint32_t>() = newColorU32;
        msg.p1.high<CS2::ui8TeamNum>() = team;
        this->PublishAsync(std::move(msg));
    }

    ImGui::SameLine();
    if (ImGui::Button("重置发光颜色")) {
        MulNX::Message msg("Glow/Team/Clear"_hash);
        msg.p1.high<CS2::ui8TeamNum>() = team;
        this->PublishAsync(std::move(msg));
    }
}

bool GlowController::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto region = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Utils::SetGlowColor);
        auto target = region.Data();
        this->hkSetGlowColor = MulNX::Hook::Create(target, [this](MulNX::Hook* hk,RegContext* ctx) {
                this->MySetGlowColor((CS2::CGlowProperty*)(ctx->rcx), (uint32_t*)&(ctx->rdx));
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkSetGlowColor->Attach();
        this->LogSucc(I18n("hook.attached", "SetGlowColor"));
        });

    this->SendTask("Update", "CSControl", [this]() {
        this->Update();
        return true;
        });

    (*this)
        .SubscribeAsync("Glow/Enable")
        .SubscribeAsync("Glow/Disable")
        .SubscribeAsync("Glow/Player/Set")
        .SubscribeAsync("Glow/Player/Clear")
        .SubscribeAsync("Glow/Player/ClearAll")
        .SubscribeAsync("Glow/Team/Set")
        .SubscribeAsync("Glow/Team/Clear")
        .SubscribeAsync("Glow/Team/ClearAll")
        .SubscribeAsync("Glow/ClearAll");

    this->CS2Con->RegisterCmd("mulnx_glow_enable", [this](CCommand* cmd) {this->PublishAsync("Glow/Enable"_hash);})
        .RegisterCmd("mulnx_glow_disable", [this](CCommand* cmd) {this->PublishAsync("Glow/Disable"_hash);})
        ;

    return true;
}

void GlowController::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "Glow/Enable"_hash: {
        this->disableGlow.store(false);
        break;
    }
    case "Glow/Disable"_hash: {
        this->disableGlow.store(true);
        break;
    }
    case "Glow/Player/Set"_hash: {
        auto uid = Msg.p1.as<Steam64UID>();
        auto color = Msg.p2.low<uint32_t>();
        std::unique_lock lock(this->smutex);
        this->playerColors[uid] = color;
        break;
    }
    case "Glow/Player/Clear"_hash: {
        auto uid = Msg.p1.as<Steam64UID>();
        std::unique_lock lock(this->smutex);
        this->playerColors.erase(uid);
        break;
    }
    case "Glow/Player/ClearAll"_hash: {
        std::unique_lock lock(this->smutex);
        this->playerColors.clear();
        break;
    }
    case "Glow/Team/Set"_hash: {
        auto team = Msg.p1.high<CS2::ui8TeamNum>();
        auto color = Msg.p1.low<uint32_t>();
        std::unique_lock lock(this->smutex);
        this->teamColors[team] = color;
        break;
    }
    case "Glow/Team/Clear"_hash: {
        auto team = Msg.p1.high<CS2::ui8TeamNum>();
        std::unique_lock lock(this->smutex);
        this->teamColors.erase(team);
        break;
    }
    case "Glow/Team/ClearAll"_hash: {
        std::unique_lock lock(this->smutex);
        this->teamColors.clear();
        break;
    }
    case "Glow/ClearAll"_hash: {
        std::unique_lock lock(this->smutex);
        this->playerColors.clear();
        this->teamColors.clear();
        break;
    }
    default:
        break;
    }
}

void GlowController::MySetGlowColor(CS2::CGlowProperty* pGlowProperty, uint32_t* color) {
    std::shared_lock lock(this->smutex);

    if (this->disableGlow.load()) {
        *color = 0;
        return;
    }

    auto pBaseModelEntity = pGlowProperty->GetOwner();
    CS2::C_CSPlayerPawn* pPlayerPawn = nullptr;
    if (pBaseModelEntity->IsPlayerPawn()) {
        pPlayerPawn = pBaseModelEntity->As<CS2::C_CSPlayerPawn>();
    }
    else {
        auto hOwnerEntity = *pBaseModelEntity->m_hOwnerEntity();
        pPlayerPawn = this->CS2->client.GetBaseEntityFromHandle(hOwnerEntity)->As<CS2::C_BaseEntity>()->As<CS2::C_CSPlayerPawn>();
    }
    if (!pPlayerPawn)return;

    auto hController = *pPlayerPawn->As<CS2::C_BasePlayerPawn>()->m_hController();
    auto pController = this->CS2->client.GetBaseEntityFromHandle(hController)->As<CS2::CBasePlayerController>();

    if (!pController)return;
    Steam64UID uid = *pController->m_steamID();

    auto itPlayer = this->playerColors.find(uid);
    if (itPlayer != this->playerColors.end()) {
        *color = itPlayer->second;
        return;
    }
    auto team = *pBaseModelEntity->iTeamNum();
    auto itTeam = this->teamColors.find(team);
    if (itTeam != this->teamColors.end()) {
        *color = itTeam->second;
        return;
    }

    return;
}