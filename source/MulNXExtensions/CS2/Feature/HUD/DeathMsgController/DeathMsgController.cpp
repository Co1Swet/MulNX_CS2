#include "DeathMsgController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Buildup/TimeController/TimeController.hpp>

bool DeathMsgController::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow(I18n("dthmsg.window.name").c_str(), this->showWindow);
    if (!w)return true;
    MulNX::UI::Checkbox(I18n("dthmsg.enable").c_str(), this->enable);
    return true;
}

bool DeathMsgController::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto pattern = MulNX::CS2::Signatures::Utils::CSHashString;
        uint8_t* callSite = this->CS2->client.GetTextRegion()
            .FindRegion(pattern).Data();

        // call 指令位于 callSite + 12 (0x0C) 处
        uint8_t* callAddr = callSite + 12;
        // E8 后面 4 字节是相对偏移
        int32_t relOffset = *reinterpret_cast<int32_t*>(callAddr + 1);
        // 目标地址 = call 指令下一条指令地址 + relOffset
        this->CSHashString = reinterpret_cast<HashFunc_t>(callAddr + 5 + relOffset);

        this->CSHashString(&this->attacker_hash, "attacker");
        this->CSHashString(&this->userid_hash, "userid");
        this->CSHashString(&this->assister_hash, "assister");

        auto target = this->CS2->client.GetTextRegion()
            .FindRegion(MulNX::CS2::Signatures::Hud::HandlePlayerDeath);
        this->hkHandlePlayerDeath = MulNX::Hook::Create(target.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            auto event = std::bit_cast<CS2::CGameEvent*>(ctx->rdx);
            return this->HandleOnPlayerDeath(event);
            }).value();
        this->hkHandlePlayerDeath->Attach();
        this->LogSucc(I18n("hook.attached", "UI::OnPlayerDeath"));

        this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});
        this->SendTask("Update", "CSControl", [this]()->bool {
            this->Update();
            return true;
            });
        });

    return true;
}

void DeathMsgController::ProcessMsg(MulNX::Message& msg) {

}

MulNX::Hook::Then DeathMsgController::HandleOnPlayerDeath(CS2::CGameEvent* event) {
    CS2::CKV3MemberName attacker{ this->attacker_hash, -1, nullptr };
    CS2::CKV3MemberName userid{ this->userid_hash, -1, nullptr };
    CS2::CKV3MemberName assister{ this->assister_hash, -1, nullptr };

    auto pKillerController = event->GetPlayerController(attacker);
    auto pBeKillerController = event->GetPlayerController(userid);
    auto pAssisterController = event->GetPlayerController(assister);

    try {
        auto killerSteamID = MulNX::MRead(pKillerController->m_steamID());
        auto beKillerSteamID = MulNX::MRead(pBeKillerController->m_steamID());
        uint64_t assisterSteamID = 0;
        if (pAssisterController) {
            assisterSteamID = MulNX::MRead(pAssisterController->m_steamID());
        }

        if (!this->enable.load(std::memory_order_acquire))return MulNX::Hook::Then::Continue;

        auto currentObservingPawn = this->CS2->client.TryGetObservingPawn();
        if (!currentObservingPawn)return MulNX::Hook::Then::Return;
        auto hObservingCtrl = MulNX::MRead(currentObservingPawn->m_hController());
        auto pObservingCtrl = this->CS2->client.GetBaseEntityFromHandle(hObservingCtrl)->As<CS2::CCSPlayerController>();
        if (!pObservingCtrl)return MulNX::Hook::Then::Return;
        auto currentObSteamID = MulNX::MRead(pObservingCtrl->m_steamID());
        if (killerSteamID != currentObSteamID)return MulNX::Hook::Then::Return;
    }
    catch (...) {
        return MulNX::Hook::Then::Return;
    }
    return MulNX::Hook::Then::Continue;
}