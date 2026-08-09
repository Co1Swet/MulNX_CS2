#include "TrailsController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/CSController/CSController.hpp>
#include <Intro/HookConsole/HookConsole.hpp>
#include <Buildup/ParticleManager/ParticleManager.hpp>
#include <Buildup/PlayerHub/PlayerHub.hpp>
#include <MulNXUtils/ColorTran/ColorTran.hpp>

using DrawStuff_t = void(*)(CS2::C_BaseCSGrenadeProjectile*, char);

void TrailsController::HubPlayer(MulNX::Message* umsg) {
    std::shared_lock lock(this->smutex);
    auto&& [uid] = umsg->Access<Steam64UID>();

    // 从存储的 0-255 浮点转为 0-1 显示
    ParticleColor currentColor{ 1.0f, 1.0f, 1.0f }; // 默认白 (255/255)
    if (auto it = this->playerColors.find(uid); it != this->playerColors.end()) {
        currentColor.r = it->second.r / 255.0f;
        currentColor.g = it->second.g / 255.0f;
        currentColor.b = it->second.b / 255.0f;
    }

    if (ImGui::ColorEdit3("轨迹颜色", &currentColor.r)) {
        MulNX::ColorTran tran;
        tran.PraseF1(currentColor.r, currentColor.g, currentColor.b, 1.0f);
        uint32_t rgba = tran.ToU255RGBA();

        MulNX::Message msg("Trails/Player/Set"_hash);
        auto&& [uidRef, color] = msg.Access<Steam64UID, uint32_t>();
        uidRef = uid;
        color = rgba;
        this->PublishAsync(std::move(msg));
    }
}

void TrailsController::HubTeam(MulNX::Message* umsg) {
    std::shared_lock lock(this->smutex);
    auto&& [team] = umsg->Access<CS2::ui8TeamNum>();
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

    if (ImGui::ColorEdit3("队伍尾迹颜色", &displayColor.r)) {
        MulNX::Message msg;
        if (team == CS2::ui8TeamNum::T)
            msg.type = "Trails/T/Set"_hash;
        else if (team == CS2::ui8TeamNum::CT)
            msg.type = "Trails/CT/Set"_hash;
        else return;

        auto&& [rgba] = msg.Access<uint32_t>();
        MulNX::ColorTran tran;
        tran.PraseF1(displayColor.r, displayColor.g, displayColor.b, 1.0f);
        rgba = tran.ToU255RGBA();
        this->PublishAsync(std::move(msg));
    }

    if (ImGui::Button("恢复尾迹颜色")) {
        MulNX::Message msg;
        if (team == CS2::ui8TeamNum::T)
            msg.type = "Trails/T/Clear"_hash;
        else if (team == CS2::ui8TeamNum::CT)
            msg.type = "Trails/CT/Clear"_hash;
        else return;
        this->PublishAsync(std::move(msg));
    }
}

void TrailsController::Menu() {
    std::shared_lock lock(this->smutex);
    ParticleProp propCopy = this->prop;

    ImGui::Checkbox("投掷物提示小窗", this->sv_grenade_trajectory_prac_pipreview);
    bool changed = false;
    MulNX::UI::Checkbox("启用投掷物轨迹", this->trailsEnable);
    changed |= ImGui::SliderFloat("生存期 (s)", this->sv_grenade_trajectory_time_spectator, 0.0f, 10.0f);
    changed |= ImGui::SliderFloat("宽度", &propCopy.width, 0.1f, 5.0f);
    // changed |= ImGui::SliderFloat("透明度()", &propCopy.alpha, 0.0f, 1.0f);

    if (changed) {
        MulNX::Message msg("Trails/Prop"_hash);
        auto&& [lifetime, width, alpha] = msg.Access<float, float, float>();
        lifetime = *this->sv_grenade_trajectory_time_spectator;
        width = propCopy.width;
        alpha = propCopy.alpha;
        this->PublishAsync(std::move(msg));
    }
}

bool TrailsController::Init() {
    this->pParticleMgr = this->FindModule<ParticleManager>("ParticleManager");

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message&) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Projectile::Func_BaseCSGrenadeProjectile_DrawStuff).Data();
        this->hkFunc_BaseCSGrenadeProjectile_DrawStuff = MulNX::Hook::Create(target, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pProjectile = (CS2::C_BaseCSGrenadeProjectile*)ctx->rcx;
            char flag = *(char*)&ctx->rdx;
            if (*pProjectile->m_nSnapshotTrajectoryEffectIndex() == -1)
                return this->HandleOnCreate(pProjectile);
            hk->CallMaybeAs<DrawStuff_t>(pProjectile, flag);
            return this->HandleOnUpdate(pProjectile);
            }).value();
        this->RegisterAttachHook(this->hkFunc_BaseCSGrenadeProjectile_DrawStuff, "C_BaseCSGrenadeProjectile_DrawStuff");

        this->sv_grenade_trajectory_prac_pipreview = this->CS2Con->GetCVarByName("sv_grenade_trajectory_prac_pipreview")->GetPtr<bool>();
        this->sv_grenade_trajectory_time_spectator = this->CS2Con->GetCVarByName("sv_grenade_trajectory_time_spectator")->GetPtr<float>();
        });

    (*this)
        .SubscribeAsync<void>("Trails/Enable")
        .SubscribeAsync<void>("Trails/Disable")
        .SubscribeAsync<Steam64UID, uint32_t>("Trails/Player/Set")
        .SubscribeAsync<Steam64UID>("Trails/Player/Clear")
        .SubscribeAsync<void>("Trails/Player/ClearAll")
        .SubscribeAsync<uint32_t>("Trails/T/Set")
        .SubscribeAsync<uint32_t>("Trails/CT/Set")
        .SubscribeAsync<void>("Trails/T/Clear")
        .SubscribeAsync<void>("Trails/CT/Clear")
        .SubscribeAsync<void>("Trails/Team/ClearAll")
        .SubscribeAsync<void>("Trails/ClearAll")
        .SubscribeAsync<float, float, float>("Trails/Prop");

    this->SendTask("Update", "CSControl", [this]() {
        this->Update();
        return true;
        });

    this->UIRegisterCallback("UI.3DVision", [this](auto&&...) {return this->Menu();});
    this->UIRegisterCallback("UI.Player.Info", [this](auto, auto msg) {this->HubPlayer(msg);});
    this->UIRegisterCallback("UI.Team.Info", [this](auto, auto msg) {this->HubTeam(msg);});

    return true;
}

std::optional<TrailsController::ParticleColor> TrailsController::FindColor(CS2::C_CSPlayerPawn* pPawn, CS2::CCSPlayerController* pController) {
    if (pController) {
        auto steamID = MulNX::MRead(pController->m_steamID());
        auto it = this->playerColors.find(steamID);
        if (it != this->playerColors.end()) {
            return it->second;   // 直接使用 0-255 范围的颜色
        }
    }
    auto team = MulNX::MRead(pPawn->iTeamNum());
    return (team == CS2::ui8TeamNum::T) ? this->TColor : this->CTColor;
}

MulNX::Hook::Then TrailsController::HandleOnCreate(CS2::C_BaseCSGrenadeProjectile* pProjectile) {
    std::shared_lock lock(this->smutex);
    if (this->trailsEnable) {
        this->prop.lifetime = *this->sv_grenade_trajectory_time_spectator;
    }
    else {
        this->prop.lifetime = 0;
    }
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
        *pProjectile->m_flTrajectoryTrailEffectCreationTime() = MulNX::MRead(this->CS2->CSGlobalVars->fCurrentTime());
        this->pParticleMgr->UpdateParticle(newIdx, 0x3, &this->prop, 0);
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

void TrailsController::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Trails/Enable"_hash: {
        this->trailsEnable = true;
        break;
    }
    case "Trails/Disable"_hash: {
        this->trailsEnable = false;
        break;
    }
    case "Trails/Player/Set"_hash: {
        auto&& [uid, rgba] = msg.Access<Steam64UID, uint32_t>();
        std::unique_lock lock(this->smutex);
        MulNX::ColorTran tran;
        tran.PraseU255RGBA(rgba);
        ParticleColor col;
        tran.ToF255RGB(col.r, col.g, col.b);
        this->playerColors[uid] = col;
        break;
    }
    case "Trails/Player/Clear"_hash: {
        auto&& [uid] = msg.Access<Steam64UID>();
        std::unique_lock lock(this->smutex);
        this->playerColors.erase(uid);
        break;
    }
    case "Trails/Player/ClearAll"_hash: {
        std::unique_lock lock(this->smutex);
        this->playerColors.clear();
        break;
    }
    case "Trails/T/Set"_hash: {
        auto&& [rgba] = msg.Access<uint32_t>();
        std::unique_lock lock(this->smutex);
        MulNX::ColorTran tran;
        tran.PraseU255RGBA(rgba);
        TrailsController::ParticleColor tColor;
        tran.ToF255RGB(tColor.r, tColor.g, tColor.b);
        this->TColor = tColor;
        break;
    }
    case "Trails/CT/Set"_hash: {
        auto&& [rgba] = msg.Access<uint32_t>();
        std::unique_lock lock(this->smutex);
        MulNX::ColorTran tran;
        tran.PraseU255RGBA(rgba);
        ParticleColor ctColor;
        tran.ToF255RGB(ctColor.r, ctColor.g, ctColor.b);
        this->CTColor = ctColor;
        break;
    }
    case "Trails/T/Clear"_hash: {
        std::unique_lock lock(this->smutex);
        this->TColor = this->TRaw;
        break;
    }
    case "Trails/CT/Clear"_hash: {
        std::unique_lock lock(this->smutex);
        this->CTColor = this->CTRaw;
        break;
    }
    case "Trails/Team/ClearAll"_hash: {
        std::unique_lock lock(this->smutex);
        this->TColor = this->TRaw;
        this->CTColor = this->CTRaw;
        break;
    }
    case "Trails/ClearAll"_hash: {
        std::unique_lock lock(this->smutex);
        this->playerColors.clear();
        this->TColor = this->TRaw;
        this->CTColor = this->CTRaw;
        break;
    }
    case "Trails/Prop"_hash: {
        auto&& [lifetime, width, alpha] = msg.Access<float, float, float>();
        ParticleProp newProp{ lifetime, width, alpha };
        std::unique_lock lock(this->smutex);
        this->prop = newProp;
        break;
    }
    default:
        break;
    }
}