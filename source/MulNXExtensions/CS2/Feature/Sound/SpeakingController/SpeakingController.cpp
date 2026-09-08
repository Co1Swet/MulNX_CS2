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
        return;
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
        this->LogError("名单模式关闭");
        break;
    }
    }
}

bool SpeakingController::CheckState(int index, CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn)const {
    auto steam64UID = MulNX::MRead(controller->m_steamID());
    if (steam64UID == 0)return false;
    auto it = this->checkList.find(steam64UID);
    if (it != this->checkList.end()) {
        return it->second;
    }
    if (this->listMode.load(std::memory_order_acquire)) {
        return false;
    }
    if (!this->onlyCurOBingSameTeam) {
        return true;
    }
    auto curOBing = this->CS2Entitys->TryGetObservingPawn();
    if (!curOBing)return false;
    auto OBingTeam = MulNX::MRead(curOBing->iTeamNum());
    auto team = MulNX::MRead(controller->iTeamNum());
    if (team != OBingTeam)return false;
    return true;
}
void SpeakingController::OnItPlayer(int index, CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn) {
    try {
        if (this->CheckState(index, controller, pawn)) {
            this->bufferMask |= (1 << index - 1);
        }
        else {
            this->bufferMask &= ~(1 << index - 1);
        }
    }
    catch (const MulNX::Exception& e) {
        this->LogWarning(e);
        this->bufferMask &= ~(1 << index - 1);
    }
    catch (...) {
        this->LogWarning("未知异常");
        this->bufferMask &= ~(1 << index - 1);
    }
    std::atomic_ref<int> atoIndices(*this->tv_listen_voice_indices);
    if (atoIndices.load(std::memory_order_acquire) != this->bufferMask) {
        atoIndices.store(this->bufferMask, std::memory_order_seq_cst);
    }
}
void SpeakingController::Main() {
    this->Update();
    if (!this->autoActiveTeamVoice)return;
    for (int i = 1;i < 32;++i) {
        auto oPlayer = this->CS2Entitys->TryGetPlayer(i);
        if (!oPlayer)continue;
        auto [controller, pawn] = oPlayer.value();
        this->OnItPlayer(i, controller, pawn);
    }
}