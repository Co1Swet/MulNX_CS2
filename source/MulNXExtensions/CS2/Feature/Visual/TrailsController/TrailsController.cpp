#include "TrailsController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/CSController/CSController.hpp>
#include <Buildup/ParticleManager/ParticleManager.hpp>
#include <Buildup/PlayerHub/PlayerHub.hpp>

using DrawStuff_t = void(*)(CS2::C_BaseCSGrenadeProjectile*, char);

// ==================== UI 实现（即时生效） ====================

void TrailsController::HubPlayer(MulNX::UINode* node) {
    std::shared_lock lock(this->smutex);
    auto uid = this->Hub->currentSteamId.load(std::memory_order_acquire);

    // 从存储的 0-255 浮点转为 0-1 显示
    ParticleColor currentColor{ 1.0f, 1.0f, 1.0f }; // 默认白 (255/255)
    if (auto it = this->playerColors.find(uid); it != this->playerColors.end()) {
        currentColor.r = it->second.r / 255.0f;
        currentColor.g = it->second.g / 255.0f;
        currentColor.b = it->second.b / 255.0f;
    }

    if (ImGui::ColorEdit3("轨迹颜色", &currentColor.r)) {
        MulNX::Message msg("Trails/Player/Set"_hash);
        msg.p1.as<Steam64UID>() = uid;
        uint32_t colorU32 = ImGui::ColorConvertFloat4ToU32(
            ImVec4(currentColor.r, currentColor.g, currentColor.b, 1.0f));
        msg.p2.low<uint32_t>() = colorU32;
        this->PublishAsync(std::move(msg));
    }
}

void TrailsController::HubTeam(MulNX::UINode* node) {
    std::shared_lock lock(this->smutex);
    auto team = this->Hub->currentTeam.load(std::memory_order_acquire);
    if (team != CS2::ui8TeamNum::T && team != CS2::ui8TeamNum::CT) {
        ImGui::Text("请选择有效队伍 (T 或 CT)");
        return;
    }

    ParticleColor* pColor = (team == CS2::ui8TeamNum::T) ? &this->TColor : &this->CTColor;
    // 将 0-255 浮点转为 0-1 显示
    ParticleColor displayColor{
        pColor->r / 255.0f,
        pColor->g / 255.0f,
        pColor->b / 255.0f
    };

    if (ImGui::ColorEdit3("队伍默认轨迹颜色", &displayColor.r)) {
        MulNX::Message msg("Trails/Team/Set"_hash);
        uint32_t colorU32 = ImGui::ColorConvertFloat4ToU32(
            ImVec4(displayColor.r, displayColor.g, displayColor.b, 1.0f));
        msg.p1.high<CS2::ui8TeamNum>() = team;
        msg.p1.low<uint32_t>() = colorU32;
        this->PublishAsync(std::move(msg));
    }
}

void TrailsController::Menu(MulNX::UINode* node) {
    std::shared_lock lock(this->smutex);
    ParticleProp propCopy = this->prop;

    bool changed = false;
    changed |= ImGui::SliderFloat("生存期 (s)", &propCopy.lifetime, 0.5f, 10.0f);
    changed |= ImGui::SliderFloat("宽度", &propCopy.width, 0.1f, 5.0f);
    changed |= ImGui::SliderFloat("透明度", &propCopy.alpha, 0.0f, 1.0f);

    if (changed) {
        MulNX::Message msg("Trails/Prop"_hash);
        msg.p1.low<float>() = propCopy.lifetime;
        msg.p1.high<float>() = propCopy.width;
        msg.p2.low<float>() = propCopy.alpha;
        this->PublishAsync(std::move(msg));
    }
}

// ==================== 初始化 ====================

bool TrailsController::Init() {
    this->pParticleMgr = this->FindModule<ParticleManager>("ParticleManager");

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message&) {
        auto target = this->CS2->client.GetTextRegion()
            .FindRegion(MulNX::CS2::Signatures::Projectile::Func_BaseCSGrenadeProjectile_DrawStuff)
            .Data();
        this->hkFunc_BaseCSGrenadeProjectile_DrawStuff = this->CreateHook(
            "C_BaseCSGrenadeProjectile_DrawStuff", target,
            [this](MulNX::Hook* hk, RegContext* ctx) {
                auto pProjectile = (CS2::C_BaseCSGrenadeProjectile*)ctx->rcx;
                char flag = *(char*)&ctx->rdx;
                if (*pProjectile->m_nSnapshotTrajectoryEffectIndex() == -1)
                    return this->HandleOnCreate(pProjectile);
                hk->CallMaybeAs<DrawStuff_t>(pProjectile, flag);
                return this->HandleOnUpdate(pProjectile);
            }).value();
        this->hkFunc_BaseCSGrenadeProjectile_DrawStuff.Attach();
        });

    (*this)
        .SubscribeAsync("Trails/Player/Set")
        .SubscribeAsync("Trails/Player/Clear")
        .SubscribeAsync("Trails/Player/ClearAll")
        .SubscribeAsync("Trails/Team/Set")
        .SubscribeAsync("Trails/Team/Clear")
        .SubscribeAsync("Trails/Team/ClearAll")
        .SubscribeAsync("Trails/ClearAll")
        .SubscribeAsync("Trails/Prop");

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {
        this->Menu(node);
        return true;
        });

    this->SendTask("Update", "CSControl",[this]() {
        this->Update();
        return true;
        });

    return true;
}

// ==================== Hook 处理（颜色已是 0-255 浮点） ====================

std::optional<TrailsController::ParticleColor> TrailsController::FindColor(CS2::C_CSPlayerPawn* pPawn, CS2::CCSPlayerController* pController) {
    std::shared_lock lock(this->smutex);
    if (pController) {
        auto steamID = MulNX::MRead(pController->m_steamID());
        if (auto it = this->playerColors.find(steamID); it != this->playerColors.end()) {
            return it->second;   // 直接使用 0-255 范围的颜色
        }
    }
    auto team = MulNX::MRead(pPawn->iTeamNum());
    return (team == CS2::ui8TeamNum::T) ? this->TColor : this->CTColor;
}

MulNX::Hook::Then TrailsController::HandleOnCreate(CS2::C_BaseCSGrenadeProjectile* pProjectile) {
    try {
        auto pPawn = this->CS2->client.GetBaseEntityFromHandle(MulNX::MRead(pProjectile->m_hThrower()))->As<CS2::C_CSPlayerPawn>();
        auto pController = this->CS2->client.GetBaseEntityFromHandle(MulNX::MRead(pPawn->m_hController()))->As<CS2::CCSPlayerController>();
        auto bColor = this->FindColor(pPawn, pController);

        if (!bColor)
            return MulNX::Hook::Then::Continue;

        int newIdx = -1;
        this->pParticleMgr->CreateParticle(&newIdx, "particles/entity/spectator_utility_trail.vpcf", 8, 0, 0, 0, 0);
        if (newIdx == -1)
            return MulNX::Hook::Then::Continue;

        *pProjectile->m_nSnapshotTrajectoryEffectIndex() = newIdx;
        *pProjectile->m_flTrajectoryTrailEffectCreationTime() =
            MulNX::MRead(this->CS2->CSGlobalVars->fCurrentTime());
        this->pParticleMgr->UpdateParticle(newIdx, 0x10, &bColor.value(), 0);
        return MulNX::Hook::Then::Continue;
    }
    catch (...) {
        return MulNX::Hook::Then::Continue;
    }
}

MulNX::Hook::Then TrailsController::HandleOnUpdate(CS2::C_BaseCSGrenadeProjectile* pProjectile) {
    std::shared_lock lock(this->smutex);
    this->pParticleMgr->UpdateParticle(
        *pProjectile->m_nSnapshotTrajectoryEffectIndex(), 0x3, &this->prop, 0);
    return MulNX::Hook::Then::Return;
}

// ==================== 消息处理（颜色转为 0-255 浮点存储） ====================

void TrailsController::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Trails/Player/Set"_hash: {
        auto uid = msg.p1.as<Steam64UID>();
        uint32_t colorU32 = msg.p2.low<uint32_t>();
        ImVec4 v4 = ImGui::ColorConvertU32ToFloat4(colorU32); // [0,1]
        std::unique_lock lock(this->smutex);
        this->playerColors[uid] = { v4.x * 255.0f, v4.y * 255.0f, v4.z * 255.0f };
        break;
    }
    case "Trails/Player/Clear"_hash: {
        auto uid = msg.p1.as<Steam64UID>();
        std::unique_lock lock(this->smutex);
        this->playerColors.erase(uid);
        break;
    }
    case "Trails/Player/ClearAll"_hash: {
        std::unique_lock lock(this->smutex);
        this->playerColors.clear();
        break;
    }
    case "Trails/Team/Set"_hash: {
        auto team = msg.p1.high<CS2::ui8TeamNum>();
        uint32_t colorU32 = msg.p1.low<uint32_t>();
        ImVec4 v4 = ImGui::ColorConvertU32ToFloat4(colorU32);
        std::unique_lock lock(this->smutex);
        if (team == CS2::ui8TeamNum::T)
            this->TColor = { v4.x * 255.0f, v4.y * 255.0f, v4.z * 255.0f };
        else if (team == CS2::ui8TeamNum::CT)
            this->CTColor = { v4.x * 255.0f, v4.y * 255.0f, v4.z * 255.0f };
        break;
    }
    case "Trails/Team/Clear"_hash: {
        auto team = msg.p1.high<CS2::ui8TeamNum>();
        std::unique_lock lock(this->smutex);
        // 重置为 0-255 默认颜色
        if (team == CS2::ui8TeamNum::T)
            this->TColor = { 255.0f, 51.0f, 51.0f };   // 红色
        else if (team == CS2::ui8TeamNum::CT)
            this->CTColor = { 51.0f, 102.0f, 255.0f };  // 蓝色
        break;
    }
    case "Trails/Team/ClearAll"_hash: {
        std::unique_lock lock(this->smutex);
        this->TColor = { 255.0f, 51.0f, 51.0f };
        this->CTColor = { 51.0f, 102.0f, 255.0f };
        break;
    }
    case "Trails/ClearAll"_hash: {
        std::unique_lock lock(this->smutex);
        this->playerColors.clear();
        this->TColor = { 255.0f, 51.0f, 51.0f };
        this->CTColor = { 51.0f, 102.0f, 255.0f };
        break;
    }
    case "Trails/Prop"_hash: {
        ParticleProp newProp;
        newProp.lifetime = msg.p1.low<float>();
        newProp.width = msg.p1.high<float>();
        newProp.alpha = msg.p2.low<float>();
        std::unique_lock lock(this->smutex);
        this->prop = newProp;
        break;
    }
    default:
        break;
    }
}