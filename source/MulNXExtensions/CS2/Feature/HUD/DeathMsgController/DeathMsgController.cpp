#include "DeathMsgController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Buildup/TimeController/TimeController.hpp>
#include <Buildup/CS2Hash/CS2Hash.hpp>

void DeathMsgController::Window() {
    auto w = MulNX::UI::RAIIWindow(I18n("dthmsg.window.name").c_str(), this->showWindow);
    if (!w || !w.ShouldDraw())return;
    MulNX::UI::Checkbox(I18n("dthmsg.enable").c_str(), this->enable);
}

bool DeathMsgController::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Hud::HandlePlayerDeath).FindFuncStart();
        this->hkHandlePlayerDeath = MulNX::Hook::Create(target.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            auto event = reinterpret_cast<CS2::CGameEvent*>(ctx->rdx);
            return this->HandleOnPlayerDeath(event);
            }).value();
        this->RegisterAttachHook(this->hkHandlePlayerDeath, "UI::OnPlayerDeath");
        });
    this->SendUIRoot(this->GetName(), [this](auto&&...) {return this->Window();});
    this->SendTask("Update", "CSControl", [this]()->bool {
        this->Update();
        return true;
        });

    this->UIRegisterCallback("UI.2DVision", [this](auto&&...) {
        MulNX::UI::Checkbox(I18n("dthmsg.window.control").c_str(), this->showWindow);
        });

    return true;
}

void DeathMsgController::ProcessMsg(MulNX::Message& msg) {

}

MulNX::Hook::Then DeathMsgController::HandleOnPlayerDeath(CS2::CGameEvent* event) {
    static CS2::CKV3MemberName attacker{ this->CS2Hashs->attacker, -1, nullptr };
    static CS2::CKV3MemberName userid{ this->CS2Hashs->userid, -1, nullptr };
    static CS2::CKV3MemberName assister{ this->CS2Hashs->assister, -1, nullptr };

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