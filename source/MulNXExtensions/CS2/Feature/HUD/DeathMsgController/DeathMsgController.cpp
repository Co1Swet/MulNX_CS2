#include "DeathMsgController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Mirror/CS2Hash/CS2Hash.hpp>
#include <unordered_map>

void DeathMsgController::Menu() {
    ImGui::SeparatorText("击杀信息控制");
    MulNX::UI::Checkbox("覆盖击杀信息显示时间", this->bOverrideLifeTime);
    MulNX::UI::SliderFloat("目标击杀信息显示时长", this->fLifeTimeOverride, 0.01f, 1000.0f);
    MulNX::UI::Checkbox("覆盖第一人称击杀信息显示时间倍率", this->bOverrideLifeTimeMod);
    MulNX::UI::SliderFloat("目标第一人称击杀信息显示时间倍率", this->fLifeTimeModOverride, 0.01f, 50000.0f);

    ImGui::SeparatorText("击杀信息过滤");
    MulNX::UI::Checkbox("仅显示白名单玩家的击杀信息", this->bFilterEnabled);
    ImGui::TextDisabled("满足以下任一条件即显示：");
    MulNX::UI::Checkbox("击杀者在白名单", this->bMatchAttacker);
    ImGui::SameLine();
    MulNX::UI::Checkbox("受害者在白名单", this->bMatchVictim);
    ImGui::SameLine();
    MulNX::UI::Checkbox("助攻者在白名单", this->bMatchAssister);

    if (ImGui::Button("添加当前观察的玩家")) {
        if (auto oUID = this->CS2Entitys->TryGetObservingSteam64UID(); oUID && *oUID != 0) {
            MulNX::Message msg("DeathMsg/Filter/Add"_hash);
            auto&& [uidRef] = msg.Access<Steam64UID>();
            uidRef = *oUID;
            this->PublishAsync(std::move(msg));
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("清空白名单")) {
        this->PublishAsync("DeathMsg/Filter/Clear"_hash);
    }
    ImGui::TextDisabled("也可在“玩家信息管理”中点击玩家进行添加（Bot 无 SteamID，无法加入）");

    std::vector<Steam64UID> uids;
    {
        std::shared_lock lock(this->smutex);
        uids.assign(this->whitelist.begin(), this->whitelist.end());
    }
    if (uids.empty()) {
        ImGui::TextDisabled("白名单为空");
    }

    std::unordered_map<Steam64UID, std::string> names;
    if (!uids.empty()) {
        for (int i = 1; i <= 64; ++i) {
            try {
                auto* pEntity = this->CS2Entitys->GetBaseEntity(i);
                if (!pEntity) continue;
                auto* pCtrl = pEntity->As<CS2::CCSPlayerController>();
                auto sid = MulNX::MRead(pCtrl->m_steamID());
                if (sid == 0) continue;
                names[sid] = MulNX::Memory::ReadString(pCtrl->m_iszPlayerName()).value_or("(名字读取失败)");
            }
            catch (const MulNX::Exception&) {}
        }
    }

    for (auto uid : uids) {
        std::string name = "(不在当前对局)";
        if (auto it = names.find(uid); it != names.end()) {
            name = it->second;
        }

        ImGui::PushID(static_cast<int>(uid ^ (uid >> 32)));
        if (ImGui::SmallButton("移除")) {
            MulNX::Message msg("DeathMsg/Filter/Remove"_hash);
            auto&& [uidRef] = msg.Access<Steam64UID>();
            uidRef = uid;
            this->PublishAsync(std::move(msg));
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(std::format("{} ({})", name, uid).c_str());
        ImGui::PopID();
    }
    ImGui::Separator();
}

void DeathMsgController::HubPlayer(MulNX::Message* umsg) {
    auto&& [uid] = umsg->Access<Steam64UID>();
    if (uid == 0) return;

    bool inList;
    {
        std::shared_lock lock(this->smutex);
        inList = this->whitelist.contains(uid);
    }
    if (ImGui::Checkbox("击杀信息白名单", &inList)) {
        MulNX::Message msg(inList ? "DeathMsg/Filter/Add"_hash : "DeathMsg/Filter/Remove"_hash);
        auto&& [uidRef] = msg.Access<Steam64UID>();
        uidRef = uid;
        this->PublishAsync(std::move(msg));
    }
}

bool DeathMsgController::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(CS2::Signatures::Hud::HudDeathNotice_HandlePlayerDeath).Data();
        this->hkHudDeathNotice_HandlePlayerDeath = MulNX::Hook::Create(target, [this](MulNX::Hook* hk, RegContext* ctx) {
            return this->OnPlayerDeath(ctx);
            }) | value_or_throw;
        this->RegisterAttachHook(this->hkHudDeathNotice_HandlePlayerDeath, "HudDeathNotice_HandlePlayerDeath");
        this->pRawHudDeathNotice_HandlePlayerDeath = reinterpret_cast<HudDeathNotice_HandlePlayerDeath_t>
            (this->hkHudDeathNotice_HandlePlayerDeath->pMaybeRawFunc);
        });

    this->SendTask("Update", "CSControl", [this]()->bool {
        this->Update();
        return true;
        });

    (*this)
        .SubscribeAsync<Steam64UID>("DeathMsg/Filter/Add")
        .SubscribeAsync<Steam64UID>("DeathMsg/Filter/Remove")
        .SubscribeAsync<void>("DeathMsg/Filter/Clear");

    this->UIRegisterCallback("UI.2DVision", [this](auto&&...) {
        this->Menu();
        });
    this->UIRegisterCallback("UI.Player.Info", [this](auto, auto msg) {
        this->HubPlayer(msg);
        });

    return true;
}

void DeathMsgController::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "DeathMsg/Filter/Add"_hash: {
        auto&& [uid] = msg.Access<Steam64UID>();
        if (uid == 0) break;
        std::unique_lock lock(this->smutex);
        this->whitelist.insert(uid);
        break;
    }
    case "DeathMsg/Filter/Remove"_hash: {
        auto&& [uid] = msg.Access<Steam64UID>();
        std::unique_lock lock(this->smutex);
        this->whitelist.erase(uid);
        break;
    }
    case "DeathMsg/Filter/Clear"_hash: {
        std::unique_lock lock(this->smutex);
        this->whitelist.clear();
        break;
    }
    default:break;
    }
}

bool DeathMsgController::ShouldShow(CS2::CGameEvent* pEvent) {
    static CS2::CKV3MemberName attacker{ this->CS2Hashs->attacker, -1, nullptr };
    static CS2::CKV3MemberName userid{ this->CS2Hashs->userid, -1, nullptr };
    static CS2::CKV3MemberName assister{ this->CS2Hashs->assister, -1, nullptr };

    try {
        auto inWhitelist = [&](const CS2::CKV3MemberName& key) -> bool {
            auto pCtrl = pEvent->GetPlayerController(key);
            if (!pCtrl) return false;
            auto uid = MulNX::MRead(pCtrl->m_steamID());
            if (uid == 0) return false;  // Bot
            std::shared_lock lock(this->smutex);
            return this->whitelist.contains(uid);
        };

        if (this->bMatchAttacker.load(std::memory_order_acquire) && inWhitelist(attacker)) return true;
        if (this->bMatchVictim.load(std::memory_order_acquire) && inWhitelist(userid)) return true;
        if (this->bMatchAssister.load(std::memory_order_acquire) && inWhitelist(assister)) return true;
        return false;
    }
    catch (const MulNX::Exception& e) {
        this->LogError(e);
        return true;
    }
}

MulNX::Hook::Then DeathMsgController::OnPlayerDeath(RegContext* ctx) {
    if (this->bFilterEnabled.load(std::memory_order_acquire)) {
        auto pEvent = reinterpret_cast<CS2::CGameEvent*>(ctx->rdx);
        if (pEvent && !this->ShouldShow(pEvent)) {
            ctx->rax = 0;
            return MulNX::Hook::Then::Return;
        }
    }

    auto pHudDeathNotice = ctx->rcx;

    float* pLifetime = (float*)(pHudDeathNotice + 0x78);
    float* pLifetimeMod = (float*)(pHudDeathNotice + 0x7C);

    auto originLifetime = *pLifetime;
    auto originLifetimeMod = *pLifetimeMod;
    if (this->fLifeTimeOverride.load(std::memory_order_acquire) < 0.0f)
        this->fLifeTimeOverride.store(*pLifetime, std::memory_order_release);
    if (this->bOverrideLifeTime.load(std::memory_order_acquire)) {
        *pLifetime = this->fLifeTimeOverride.load(std::memory_order_acquire);
    }
    if (this->fLifeTimeModOverride.load(std::memory_order_acquire) < 0.0f)
        this->fLifeTimeModOverride.store(*pLifetimeMod, std::memory_order_release);
    if (this->bOverrideLifeTimeMod.load(std::memory_order_acquire)) {
        *pLifetimeMod = this->fLifeTimeModOverride.load(std::memory_order_acquire);
    }
    ctx->rax = (uint64_t)this->pRawHudDeathNotice_HandlePlayerDeath(ctx->rcx, ctx->rdx);
    *pLifetime = originLifetime;
    *pLifetimeMod = originLifetimeMod;
    return MulNX::Hook::Then::Return;
}
