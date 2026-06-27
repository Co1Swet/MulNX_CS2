#include "SpeakingController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/HookConsole/HookConsole.hpp>

void SpeakingController::Menu(MulNX::UINode* node) {
    MulNX::UI::Checkbox("自动激活玩家语音", this->runFlag1);
    MulNX::UI::Checkbox("仅播放当前观战玩家的阵营语音(默认激活所有玩家语音)", this->onlyCurOBingSameTeam);
}
bool SpeakingController::Init() {
    this->SubscribeSync("Hook/Source2Client002::Inited", [this](MulNX::Message& msg) {
        this->tv_listen_voice_indices = this->CS2Con->GetCvar("tv_listen_voice_indices")->GetPtr<int>();
        return;
        });
    this->participateIt = true;
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Menu(node);});
    return true;
}
void SpeakingController::OnItBegin() {
    if (!this->runFlag1)return;
    this->bufferMask = 0;
    auto curOBing = this->CS2->client.TryGetObservingPawn();
    if (!curOBing)return;
    try {
        this->targetTeam = MulNX::MRead(curOBing->iTeamNum());
    }
    catch (...) {
        return;
    }
}
void SpeakingController::OnItPlayer(int index, CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn) {
    if (!this->runFlag1)return;
    if (!this->onlyCurOBingSameTeam) {
        this->bufferMask |= (1 << index - 1);
        return;
    }
    try {
        auto team = MulNX::MRead(controller->iTeamNum());
        if (team != this->targetTeam)return;
        this->bufferMask |= (1 << index - 1);
    }
    catch (...) {
        return;
    }
}
void SpeakingController::OnItEnd() {
    if (!this->runFlag1)return;
    *this->tv_listen_voice_indices = this->bufferMask;
}