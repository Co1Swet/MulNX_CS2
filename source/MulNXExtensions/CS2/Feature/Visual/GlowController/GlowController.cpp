#include "GlowController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/HookConsole/HookConsole.hpp>
#include <Buildup/PlayerHub/PlayerHub.hpp>
#include <MulNXUtils/ColorTran/ColorTran.hpp>

// ---------- UI 回调 ----------
void GlowController::HubPlayer(MulNX::Message* umsg) {
    std::shared_lock lock(this->smutex);
    auto&& [uid] = umsg->Access<Steam64UID>();

    // 默认白色 (RGBA: 0xFFFFFFFF)
    uint32_t currentRGBA = 0xFFFFFFFF;
    if (auto it = this->playerColors.find(uid); it != this->playerColors.end()) {
        currentRGBA = it->second;                     // 内部存储的 RGBA
    }
    else {
        ImGui::Text("当前玩家没有自定义发光颜色，使用默认颜色");
    }

    // 转为 ImGui 可用的 IM_COL32 格式 (AABBGGRR)
    uint32_t currentGameColor = MulNX::ColorTran::U255RGBA_ABGR_Swap(currentRGBA);
    ImVec4 colorVec4 = ImGui::ColorConvertU32ToFloat4(currentGameColor);

    if (ImGui::ColorEdit4("发光颜色修改", (float*)&colorVec4)) {
        // 从 ImGui 获取新的游戏格式颜色，转换回内部 RGBA
        uint32_t newGameColor = ImGui::ColorConvertFloat4ToU32(colorVec4);
        uint32_t newRGBA = MulNX::ColorTran::U255RGBA_ABGR_Swap(newGameColor);

        MulNX::Message msg("Glow/Player/Set"_hash);
        auto&& [uidRef, colorRef] = msg.Access<Steam64UID, uint32_t>();
        uidRef = uid;
        colorRef = newRGBA;                          // 发送 RGBA 值
        this->PublishAsync(std::move(msg));
    }

    ImGui::SameLine();
    if (ImGui::Button("重置发光颜色")) {
        MulNX::Message msg("Glow/Player/Clear"_hash);
        auto&& [uidRef] = msg.Access<Steam64UID>();
        uidRef = uid;
        this->PublishAsync(std::move(msg));
    }
}

void GlowController::HubTeam(MulNX::Message* umsg) {
    std::shared_lock lock(this->smutex);
    auto&& [team] = umsg->Access<CS2::ui8TeamNum>();

    // 默认白色 RGBA
    uint32_t currentRGBA = 0xFFFFFFFF;
    if (team == CS2::ui8TeamNum::T) {
        if (auto oT = this->TColor.load(); oT.has_value())
            currentRGBA = oT.value();
    }
    else if (team == CS2::ui8TeamNum::CT) {
        if (auto oCT = this->CTColor.load(); oCT.has_value())
            currentRGBA = oCT.value();
    }
    else {
        ImGui::Text("无效队伍");
        return;
    }

    if (currentRGBA == 0xFFFFFFFF && !this->TColor.load().has_value() && !this->CTColor.load().has_value())
        ImGui::Text("当前队伍没有自定义发光颜色，使用默认颜色");

    // 转为 ImGui 格式显示
    uint32_t currentGameColor = MulNX::ColorTran::U255RGBA_ABGR_Swap(currentRGBA);
    ImVec4 colorVec4 = ImGui::ColorConvertU32ToFloat4(currentGameColor);

    if (ImGui::ColorEdit4("发光颜色修改", (float*)&colorVec4)) {
        uint32_t newGameColor = ImGui::ColorConvertFloat4ToU32(colorVec4);
        uint32_t newRGBA = MulNX::ColorTran::U255RGBA_ABGR_Swap(newGameColor);

        MulNX::Message msg;
        if (team == CS2::ui8TeamNum::T)
            msg.type = "Glow/T/Set"_hash;
        else if (team == CS2::ui8TeamNum::CT)
            msg.type = "Glow/CT/Set"_hash;
        else return;

        auto&& [colorRef] = msg.Access<uint32_t>();
        colorRef = newRGBA;                          // 发送 RGBA
        this->PublishAsync(std::move(msg));
    }

    ImGui::SameLine();
    if (ImGui::Button("重置发光颜色")) {
        MulNX::Message msg;
        if (team == CS2::ui8TeamNum::T)
            msg.type = "Glow/T/Clear"_hash;
        else if (team == CS2::ui8TeamNum::CT)
            msg.type = "Glow/CT/Clear"_hash;
        else return;
        this->PublishAsync(std::move(msg));
    }
}

// ---------- 初始化 ----------
bool GlowController::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Utils::SetGlowColor);
        this->hkSetGlowColor = MulNX::Hook::Create(target.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            try {
                this->HandleSetGlowColor((CS2::CGlowProperty*)ctx->rcx, (uint32_t*)&ctx->rdx);
            }
            catch (MulNX::Exception& e) {
                this->LogError(e);
            }
            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkSetGlowColor, "SetGlowColor");
        });

    this->SendTask("Update", "CSControl", [this]() {
        this->Update();
        return true;
        });

    (*this)
        .SubscribeAsync<void>("Glow/Enable")
        .SubscribeAsync<void>("Glow/Disable")
        .SubscribeAsync<Steam64UID, uint32_t>("Glow/Player/Set")
        .SubscribeAsync<Steam64UID>("Glow/Player/Clear")
        .SubscribeAsync<void>("Glow/Player/ClearAll")
        .SubscribeAsync<uint32_t>("Glow/T/Set")
        .SubscribeAsync<uint32_t>("Glow/CT/Set")
        .SubscribeAsync<void>("Glow/T/Clear")
        .SubscribeAsync<void>("Glow/CT/Clear")
        .SubscribeAsync<void>("Glow/Team/ClearAll")
        .SubscribeAsync<void>("Glow/ClearAll");

    this->UIRegisterCallback("UI.Player.Info", [this](auto, auto msg) { this->HubPlayer(msg); });
    this->UIRegisterCallback("UI.Team.Info", [this](auto, auto msg) { this->HubTeam(msg); });

    this->UIRegisterCallback("UI.3DVision", [this](auto&&...) {
        MulNX::UI::Checkbox("隐藏发光", this->disableGlow);
        });

    return true;
}

// ---------- 消息处理 ----------
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
        auto&& [uid, color] = Msg.Access<Steam64UID, uint32_t>();   // color 已是 RGBA
        std::unique_lock lock(this->smutex);
        this->playerColors[uid] = color;
        break;
    }
    case "Glow/Player/Clear"_hash: {
        auto&& [uid] = Msg.Access<Steam64UID>();
        std::unique_lock lock(this->smutex);
        this->playerColors.erase(uid);
        break;
    }
    case "Glow/Player/ClearAll"_hash: {
        std::unique_lock lock(this->smutex);
        this->playerColors.clear();
        break;
    }
    case "Glow/T/Set"_hash: {
        auto&& [color] = Msg.Access<uint32_t>();                    // RGBA
        this->TColor.store(color);
        break;
    }
    case "Glow/CT/Set"_hash: {
        auto&& [color] = Msg.Access<uint32_t>();
        this->CTColor.store(color);
        break;
    }
    case "Glow/T/Clear"_hash: {
        this->TColor.store(std::nullopt);
        break;
    }
    case "Glow/CT/Clear"_hash: {
        this->CTColor.store(std::nullopt);
        break;
    }
    case "Glow/Team/ClearAll"_hash: {
        this->TColor.store(std::nullopt);
        this->CTColor.store(std::nullopt);
        break;
    }
    case "Glow/ClearAll"_hash: {
        this->TColor.store(std::nullopt);
        this->CTColor.store(std::nullopt);
        std::unique_lock lock(this->smutex);
        this->playerColors.clear();
        break;
    }
    default:
        break;
    }
}

// ---------- 钩子处理：注入颜色 ----------
void GlowController::HandleSetGlowColor(CS2::CGlowProperty* pGlowProperty, uint32_t* color) {
    if (this->disableGlow.load()) {
        *color = 0;
        return;
    }

    auto PraseTeamFromColor = [&]() -> CS2::ui8TeamNum {
        // 游戏颜色为 ABGR 格式，低 24 位为 BGR
        constexpr uint32_t TDefaultRGB = 0x56AFE0; // B=0x56, G=0xAF, R=0xE0
        constexpr uint32_t CTDefaultRGB = 0xDD9B72; // B=0xDD, G=0x9B, R=0x72

        uint32_t rgbPart = *color & 0x00FFFFFF;

        if (rgbPart == TDefaultRGB)
            return CS2::ui8TeamNum::T;
        if (rgbPart == CTDefaultRGB)
            return CS2::ui8TeamNum::CT;
        return CS2::ui8TeamNum::unk;
        };

    CS2::ui8TeamNum team = PraseTeamFromColor();

    // 定位玩家 Pawn
    auto pBaseModelEntity = pGlowProperty->GetOwner();
    CS2::C_CSPlayerPawn* pPlayerPawn = nullptr;
    if (pBaseModelEntity->IsPlayerPawn()) {
        pPlayerPawn = pBaseModelEntity->As<CS2::C_CSPlayerPawn>();
    }
    else {
        auto hOwnerEntity = MulNX::MRead(pBaseModelEntity->m_hOwnerEntity());
        pPlayerPawn = this->CS2->client.GetBaseEntityFromHandle(hOwnerEntity)->As<CS2::C_CSPlayerPawn>();
    }

    // 检查队伍颜色
    if (pPlayerPawn) {
        auto temp = MulNX::MRead(pPlayerPawn->iTeamNum());
        if (temp == CS2::ui8TeamNum::T)team = CS2::ui8TeamNum::T;
        if (temp == CS2::ui8TeamNum::CT)team = CS2::ui8TeamNum::CT;
    }

    if (team == CS2::ui8TeamNum::T) {
        if (auto oT = this->TColor.load()) {
            *color = MulNX::ColorTran::U255RGBA_ABGR_Swap(*oT);
        }
    }
    else if (team == CS2::ui8TeamNum::CT) {
        if (auto oCT = this->CTColor.load()) {
            *color = MulNX::ColorTran::U255RGBA_ABGR_Swap(*oCT);
        }
    }

    if (!pPlayerPawn) return;
    auto hController = MulNX::MRead(pPlayerPawn->m_hController());
    auto pController = this->CS2->client.GetBaseEntityFromHandle(hController)->As<CS2::CBasePlayerController>();
    if (!pController) return;

    Steam64UID uid = MulNX::MRead(pController->m_steamID());

    // 检查玩家个人颜色
    std::shared_lock lock(this->smutex);
    auto itPlayer = this->playerColors.find(uid);
    if (itPlayer != this->playerColors.end()) {
        *color = MulNX::ColorTran::U255RGBA_ABGR_Swap(itPlayer->second);   // 内部 RGBA → 游戏格式
    }
}