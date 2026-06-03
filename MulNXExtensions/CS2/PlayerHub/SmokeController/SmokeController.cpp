#include "SmokeController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>
#include <MulNXExtensions/CS2/PlayerHub/PlayerHub.hpp>

void SmokeController::Menu(MulNX::UINode* node) {
    auto view = this->Hub->showView.load(std::memory_order_acquire);
    if (view == PlayerHub::View::Player) {
        this->MenuPlayer(node);
    }
    else {
        this->MenuTeam(node);
    }
}

void SmokeController::MenuPlayer(MulNX::UINode* node) {
    auto uid = this->Hub->currentSteamId.load(std::memory_order_acquire);

    uint32_t currentColorU32 = IM_COL32(255, 255, 255, 255);
    auto it = this->playerColors.find(uid);
    if (it != this->playerColors.end()) {
        currentColorU32 = it->second;
    }
    else {
        ImGui::Text("当前玩家没有自定义烟雾颜色，使用默认颜色");
    }

    ImVec4 colorVec4 = ImGui::ColorConvertU32ToFloat4(currentColorU32);
    if (ImGui::ColorEdit4("烟雾颜色修改", (float*)&colorVec4,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
        uint32_t newColorU32 = ImGui::ColorConvertFloat4ToU32(colorVec4);
        MulNX::Message msg("Smoke/Player/Set"_hash);
        msg.p1.as<Steam64UID>() = uid;
        msg.p2.low<uint32_t>() = newColorU32;
        this->ISys().PublishAsync(std::move(msg));
    }

    ImGui::SameLine();
    if (ImGui::Button("重置")) {
        MulNX::Message msg("Smoke/Player/Clear"_hash);
        msg.p1.as<Steam64UID>() = uid;
        this->ISys().PublishAsync(std::move(msg));
    }
}

void SmokeController::MenuTeam(MulNX::UINode* node) {
    auto team = this->Hub->currentTeam.load(std::memory_order_acquire);

    uint32_t currentColorU32 = IM_COL32(255, 255, 255, 255);
    auto it = this->teamColors.find(team);
    if (it != this->teamColors.end()) {
        currentColorU32 = it->second;
    }
    else {
        ImGui::Text("当前队伍没有自定义烟雾颜色，使用默认颜色");
    }

    ImVec4 colorVec4 = ImGui::ColorConvertU32ToFloat4(currentColorU32);
    if (ImGui::ColorEdit4("烟雾颜色修改", (float*)&colorVec4,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
        uint32_t newColorU32 = ImGui::ColorConvertFloat4ToU32(colorVec4);
        MulNX::Message msg("Smoke/Team/Set"_hash);
        msg.p1.low<uint32_t>() = newColorU32;
        msg.p1.high<CS2::ui8TeamNum>() = team;
        this->ISys().PublishAsync(std::move(msg));
    }

    ImGui::SameLine();
    if (ImGui::Button("重置")) {
        MulNX::Message msg("Smoke/Team/Clear"_hash);
        msg.p1.high<CS2::ui8TeamNum>() = team;
        this->ISys().PublishAsync(std::move(msg));
    }
}

bool SmokeController::Init() {
    this->ISys().SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {

        // 1. 定位并挂钩 SetSmokeProps
        auto target = this->CS2->client.GetTextRegion()
            .FindRegion(MulNX::CS2::Signatures::SetSmokeProps).Data();

        this->hkSetSmokeProps = MulNX::Hook::Create(target, [this](MulNX::Hook* hook, RegContext* ctx) {
            // 再应用自定义颜色
            this->MySetSmokeProps((CS2::C_SmokeGrenadeProjectile*)(ctx->rcx));
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkSetSmokeProps->Attach();
        this->ISys().LogSucc(I18n("hook.attached", "SetSmokeProps"));

        // 2. 注册 UI
        this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {
            this->Menu(node);
            return true;
            });

        // 3. 消息处理线程
        this->ISys().SendTask("Update", "CSControl", [this]() {
            this->Update();
            return true;
            });
        });

    // 4. 订阅消息
    this->ISys()
        .SubscribeAsync("Smoke/Player/Set")
        .SubscribeAsync("Smoke/Player/Clear")
        .SubscribeAsync("Smoke/Player/ClearAll")
        .SubscribeAsync("Smoke/Team/Set")
        .SubscribeAsync("Smoke/Team/Clear")
        .SubscribeAsync("Smoke/Team/ClearAll")
        .SubscribeAsync("Smoke/ClearAll");

    return true;
}

void SmokeController::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "Smoke/Player/Set"_hash: {
        auto uid = Msg.p1.as<Steam64UID>();
        auto color = Msg.p2.low<uint32_t>();
        std::unique_lock lock(this->Hub->smutex);
        this->playerColors[uid] = color;
        break;
    }
    case "Smoke/Player/Clear"_hash: {
        auto uid = Msg.p1.as<Steam64UID>();
        std::unique_lock lock(this->Hub->smutex);
        this->playerColors.erase(uid);
        break;
    }
    case "Smoke/Player/ClearAll"_hash: {
        std::unique_lock lock(this->Hub->smutex);
        this->playerColors.clear();
        break;
    }
    case "Smoke/Team/Set"_hash: {
        auto team = Msg.p1.high<CS2::ui8TeamNum>();
        auto color = Msg.p1.low<uint32_t>();
        std::unique_lock lock(this->Hub->smutex);
        this->teamColors[team] = color;
        break;
    }
    case "Smoke/Team/Clear"_hash: {
        auto team = Msg.p1.high<CS2::ui8TeamNum>();
        std::unique_lock lock(this->Hub->smutex);
        this->teamColors.erase(team);
        break;
    }
    case "Smoke/Team/ClearAll"_hash: {
        std::unique_lock lock(this->Hub->smutex);
        this->teamColors.clear();
        break;
    }
    case "Smoke/ClearAll"_hash: {
        std::unique_lock lock(this->Hub->smutex);
        this->playerColors.clear();
        this->teamColors.clear();
        break;
    }
    default:
        break;
    }
}

void SmokeController::MySetSmokeProps(CS2::C_SmokeGrenadeProjectile* pSmoke) {
    std::shared_lock lock(this->Hub->smutex);
    try {
        
        // 获取投掷者实体
        auto hThrower = MulNX::MRead(pSmoke->m_hThrower());
        auto* pThrower = this->CS2->client.GetBaseEntityFromHandle(hThrower)->As<CS2::C_CSPlayerPawn>();

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
    catch (const std::exception& e) {
        return;
    }
}