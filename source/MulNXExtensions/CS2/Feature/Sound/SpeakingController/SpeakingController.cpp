#include "SpeakingController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/HookConsole/HookConsole.hpp>

void SpeakingController::Menu() {
    MulNX::UI::Checkbox("语音名单模式（不属于白名单的禁言）", this->listMode);
    MulNX::UI::Checkbox("自动激活玩家语音", this->autoActiveTeamVoice);
    MulNX::UI::Checkbox("仅播放当前观战玩家的阵营语音(默认激活所有玩家语音)", this->onlyCurOBingSameTeam);
}
bool SpeakingController::Init() {
    this->SubscribeSync("Hook/Source2Client002::Inited", [this](MulNX::Message& msg) {
        this->tv_listen_voice_indices = this->CS2Con->GetCvar("tv_listen_voice_indices")->GetPtr<int>();
        this->tv_listen_voice_indices_h = this->CS2Con->GetCvar("tv_listen_voice_indices_h")->GetPtr<int>();
        });

    (*this)
        .SubscribeAsync<Steam64UID>("PlayerVoice/EnableOne")
        .SubscribeAsync<Steam64UID>("PlayerVoice/DisableOne")
        .SubscribeAsync<void>("PlayerVoice/ClearRules")
        .SubscribeAsync<void>("PlayerVoice/ListModeOn")
        .SubscribeAsync<void>("PlayerVoice/ListModeOff")
        ;

    this->UIRegisterCallback("UI.Sound", [this](auto&&...) {return this->Menu();});

    this->SendTask("Main", "CSControl", [this]() {
        try {
            this->Main();
        }
        catch (const MulNX::Exception& e) {
            this->LogError(e);
        }
        return true;
        });

    return true;
}

void SpeakingController::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "PlayerVoice/EnableOne"_hash: {
        auto&& [uid] = msg.Access<Steam64UID>();
        this->checkList[uid] = true;
        this->LogSucc(std::format("已添加白名单：{}", uid));
        break;
    }
    case "PlayerVoice/DisableOne"_hash: {
        auto&& [uid] = msg.Access<Steam64UID>();
        this->checkList[uid] = false;
        this->LogSucc(std::format("已添加黑名单：{}", uid));
        break;
    }
    case "PlayerVoice/ClearRules"_hash: {
        this->checkList.clear();
        this->LogSucc("已清空名单");
        break;
    }
    case "PlayerVoice/ListModeOn"_hash: {
        this->listMode = true;
        this->LogSucc("名单模式开启");
        break;
    }
    case "PlayerVoice/ListModeOff"_hash: {
        this->listMode = false;
        this->LogSucc("名单模式关闭");
        break;
    }
    }
}

bool SpeakingController::CheckState(int index, CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn) const {
    auto steam64UID = MulNX::MRead(controller->m_steamID());
    if (steam64UID == 0) return false;
    // 黑白名单优先
    auto it = this->checkList.find(steam64UID);
    if (it != this->checkList.end()) {
        return it->second;
    }
    // 名单模式下，非白名单一律禁言
    if (this->listMode.load(std::memory_order_acquire)) {
        return false;
    }
    // 未开启"仅观战同阵营" -> 默认全部允许
    if (!this->onlyCurOBingSameTeam) {
        return true;
    }
    // 仅播放当前观战者同阵营玩家的语音
    auto curOBing = this->CS2Entitys->TryGetObservingPawn();
    if (!curOBing) return false;
    auto OBingTeam = MulNX::MRead(curOBing->iTeamNum());
    auto team = MulNX::MRead(controller->iTeamNum());
    if (team != OBingTeam) return false;
    return true;
}

void SpeakingController::OnItPlayer(int index, CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn) {
    // index 为 1-based（1-64），对应引擎 slot 0-63
    //   slot 0-31  -> tv_listen_voice_indices   (bit 0-31)
    //   slot 32-63 -> tv_listen_voice_indices_h (bit 0-31)
    bool shouldListen = false;
    try {
        shouldListen = this->CheckState(index, controller, pawn);
    }
    catch (const MulNX::Exception& e) {
        this->LogWarning(e);
        shouldListen = false;
    }
    catch (...) {
        this->LogWarning("未知异常");
        shouldListen = false;
    }
    const int slot = index - 1;
    if (slot < 32) {
        // 低位 Cvar：位偏移 = slot。用 1u 避免 1 << 31 的符号位溢出 UB
        const int bit = static_cast<int>(1u << slot);
        if (shouldListen) this->bufferMask |= bit;
        else              this->bufferMask &= ~bit;
        std::atomic_ref<int> ato(*this->tv_listen_voice_indices);
        if (ato.load(std::memory_order_acquire) != this->bufferMask) {
            ato.store(this->bufferMask, std::memory_order_seq_cst);
        }
    }
    else {
        // 高位 Cvar：位偏移 = slot - 32
        const int bit = static_cast<int>(1u << (slot - 32));
        if (shouldListen) this->bufferMaskHigh |= bit;
        else              this->bufferMaskHigh &= ~bit;
        std::atomic_ref<int> ato(*this->tv_listen_voice_indices_h);
        if (ato.load(std::memory_order_acquire) != this->bufferMaskHigh) {
            ato.store(this->bufferMaskHigh, std::memory_order_seq_cst);
        }
    }
}
void SpeakingController::Main() {
    this->Update();
    if (!this->autoActiveTeamVoice) return;
    for (int i = 1; i <= 64; ++i) {
        auto oPlayer = this->CS2Entitys->TryGetPlayer(i);
        if (!oPlayer) continue;
        auto [controller, pawn] = oPlayer.value();
        this->OnItPlayer(i, controller, pawn);
    }
}