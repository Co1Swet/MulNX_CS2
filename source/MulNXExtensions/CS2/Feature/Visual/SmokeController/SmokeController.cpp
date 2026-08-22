#include "SmokeController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Buildup/PlayerHub/PlayerHub.hpp>

void SmokeController::HubPlayer(MulNX::Message* umsg) {
    std::shared_lock lock(this->smutex);
    auto&& [uid] = umsg->Access<Steam64UID>();

    uint32_t currentColorU32 = IM_COL32(255, 255, 255, 255);
    auto it = this->playerColors.find(uid);
    if (it != this->playerColors.end()) {
        currentColorU32 = it->second;
    }
    else {
        ImGui::Text("当前玩家没有自定义烟雾颜色，使用默认颜色");
    }

    ImVec4 colorVec4 = ImGui::ColorConvertU32ToFloat4(currentColorU32);
    if (ImGui::ColorEdit4("烟雾颜色修改", (float*)&colorVec4)) {
        uint32_t newColorU32 = ImGui::ColorConvertFloat4ToU32(colorVec4);
        MulNX::Message msg("Smoke/Player/Set"_hash);
        auto&& [uidRef, newColorRef] = msg.Access<Steam64UID, uint32_t>();
        uidRef = uid;
        newColorRef = newColorU32;
        this->PublishAsync(std::move(msg));
    }

    ImGui::SameLine();
    if (ImGui::Button("重置烟雾颜色")) {
        MulNX::Message msg("Smoke/Player/Clear"_hash);
        auto&& [uidRef] = msg.Access<Steam64UID>();
        uidRef = uid;
        this->PublishAsync(std::move(msg));
    }
}

void SmokeController::HubTeam(MulNX::Message* umsg) {
    std::shared_lock lock(this->smutex);
    auto&& [team] = umsg->Access<CS2::ui8TeamNum>();

    uint32_t currentColorU32 = IM_COL32(255, 255, 255, 255);
    auto it = this->teamColors.find(team);
    if (it != this->teamColors.end()) {
        currentColorU32 = it->second;
    }
    else {
        ImGui::Text("当前队伍没有自定义烟雾颜色，使用默认颜色");
    }

    ImVec4 colorVec4 = ImGui::ColorConvertU32ToFloat4(currentColorU32);
    if (ImGui::ColorEdit4("烟雾颜色修改", (float*)&colorVec4)) {
        uint32_t newColorU32 = ImGui::ColorConvertFloat4ToU32(colorVec4);
        MulNX::Message msg("Smoke/Team/Set"_hash);
        auto&& [colorRef, teamRef] = msg.Access<uint32_t, CS2::ui8TeamNum>();
        colorRef = newColorU32;
        teamRef = team;
        this->PublishAsync(std::move(msg));
    }

    ImGui::SameLine();
    if (ImGui::Button("重置烟雾颜色")) {
        MulNX::Message msg("Smoke/Team/Clear"_hash);
        auto&& [teamRef] = msg.Access<CS2::ui8TeamNum>();
        teamRef = team;
        this->PublishAsync(std::move(msg));
    }
}

bool SmokeController::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Projectile::SetSmokeProps).Data();
        this->hkSetSmokeProps = MulNX::Hook::Create(target, [this](MulNX::Hook* hook, RegContext* ctx) {
            this->MySetSmokeProps((CS2::C_SmokeGrenadeProjectile*)(ctx->rcx));
            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkSetSmokeProps, "SetSmokeProps");

        this->SendTask("Update", "CSControl", [this]() {
            this->Update();
            return true;
            });
        });

    (*this)
        .SubscribeAsync("Smoke/Player/Set")
        .SubscribeAsync("Smoke/Player/Clear")
        .SubscribeAsync("Smoke/Player/ClearAll")
        .SubscribeAsync("Smoke/Team/Set")
        .SubscribeAsync("Smoke/Team/Clear")
        .SubscribeAsync("Smoke/Team/ClearAll")
        .SubscribeAsync("Smoke/ClearAll");

    this->UIRegisterCallback("UI.Player.Info", [this](auto, auto msg) {this->HubPlayer(msg);});
    this->UIRegisterCallback("UI.Team.Info", [this](auto, auto msg) {this->HubTeam(msg);});

    return true;
}

void SmokeController::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "Smoke/Player/Set"_hash: {
        auto&& [uid, color] = Msg.Access<Steam64UID, uint32_t>();
        std::unique_lock lock(this->smutex);
        this->playerColors[uid] = color;
        break;
    }
    case "Smoke/Player/Clear"_hash: {
        auto&& [uid] = Msg.Access<Steam64UID>();
        std::unique_lock lock(this->smutex);
        this->playerColors.erase(uid);
        break;
    }
    case "Smoke/Player/ClearAll"_hash: {
        std::unique_lock lock(this->smutex);
        this->playerColors.clear();
        break;
    }
    case "Smoke/Team/Set"_hash: {
        auto&& [color, team] = Msg.Access<uint32_t, CS2::ui8TeamNum>();
        std::unique_lock lock(this->smutex);
        this->teamColors[team] = color;
        break;
    }
    case "Smoke/Team/Clear"_hash: {
        auto&& [team] = Msg.Access<CS2::ui8TeamNum>();
        std::unique_lock lock(this->smutex);
        this->teamColors.erase(team);
        break;
    }
    case "Smoke/Team/ClearAll"_hash: {
        std::unique_lock lock(this->smutex);
        this->teamColors.clear();
        break;
    }
    case "Smoke/ClearAll"_hash: {
        std::unique_lock lock(this->smutex);
        this->playerColors.clear();
        this->teamColors.clear();
        break;
    }
    default:
        break;
    }
}

void SmokeController::MySetSmokeProps(CS2::C_SmokeGrenadeProjectile* pSmoke) {
    std::shared_lock lock(this->smutex);
    try {
        
        // 获取投掷者实体
        auto hThrower = MulNX::MRead(pSmoke->m_hThrower());
        auto* pThrower = this->CS2->client.GetBaseEntityFromHandle(hThrower)->As<CS2::C_CSPlayerPawn>();

        if (!pThrower) {
            return;
        }

        // 获取控制器
        auto hController = MulNX::MRead(pThrower->m_hController());
        auto pController = this->CS2->client.GetBaseEntityFromHandle(hController)->As<CS2::CBasePlayerController>();

        if (!pController) {
            return;
        }

        Steam64UID uid = MulNX::MRead(pController->m_steamID());

        // 获取颜色向量指针
        auto* pColorVec = pSmoke->vSmokeColor();

        // 1. 优先玩家自定义颜色
        auto itPlayer = this->playerColors.find(uid);
        if (itPlayer != this->playerColors.end()) {
            uint32_t c = itPlayer->second;
            // ImGui 格式为 0xAABBGGRR，引擎期望 R、G、B 分量范围 0-255 的 float
            pColorVec->x = (float)(c & 0xFF);                // R
            pColorVec->y = (float)((c >> 8) & 0xFF);         // G
            pColorVec->z = (float)((c >> 16) & 0xFF);        // B
            return;
        }

        // 2. 队伍自定义颜色
        auto team = *pThrower->iTeamNum();
        auto itTeam = this->teamColors.find(team);
        if (itTeam != this->teamColors.end()) {
            uint32_t c = itTeam->second;
            pColorVec->x = (float)(c & 0xFF);                // R
            pColorVec->y = (float)((c >> 8) & 0xFF);         // G
            pColorVec->z = (float)((c >> 16) & 0xFF);        // B
            return;
        }
    }
    catch (const MulNX::Exception& e) {
        this->LogError(e);
    }
}