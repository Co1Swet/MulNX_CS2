#include "DeathMsgController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>
#include <MulNXExtensions/CS2/TimeController/TimeController.hpp>

using GetPlayerController_t = void* (__fastcall*)(void* event, uint32_t keyHash);
struct CKV3MemberName {
    uint32_t hash;
    int      index;
    const char* str;
};

bool DeathMsgController::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow(I18n("dthmsg.window.name").c_str(), this->ShowWindow);
    if (!w)return true;
    MulNX::UI::Checkbox(I18n("dthmsg.enable").c_str(), this->enable);
    return true;
}

bool DeathMsgController::Init() {
    auto pattern = MulNX::CS2::Signatures::CSHashString;
    uint8_t* callSite = this->CS2()->client.GetTextRegion()
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

    auto target = this->CS2()->client.GetTextRegion()
        .FindRegion(MulNX::CS2::Signatures::HandlePlayerDeath);
    this->hkHandlePlayerDeath = MulNX::Hook::Create(target.Data(), 0, false,
        [this](RegContext* ctx, MulNX::Hook* hk) {
            void* event = reinterpret_cast<void*>(ctx->rdx);
            return this->HandleOnPlayerDeath(event);
        }).value();
    this->hkHandlePlayerDeath->Attach();
    this->ISys().LogSucc(I18n("hook.attached", "UI::OnPlayerDeath"));

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});
    this->SendTask("CSControl", [this]()->bool {
        this->Update();
        return true;
        });

    return true;
}

void DeathMsgController::ProcessMsg(MulNX::Message& msg) {
    
}

MulNX::Hook::Then DeathMsgController::HandleOnPlayerDeath(void* event) {
    auto GetPlayerController = IVClass::Assume(event)->GetVFunc<CS2::CCSPlayerController * (const CKV3MemberName&)>(16);
 
    CKV3MemberName attacker{ this->attacker_hash, -1, nullptr };
    CKV3MemberName userid{ this->userid_hash, -1, nullptr };
    CKV3MemberName assister{ this->assister_hash, -1, nullptr };

    auto pKillerController = GetPlayerController(attacker);
    auto pBeKillerController = GetPlayerController(userid);
    auto pAssisterController = GetPlayerController(assister);

    try {
        auto killerSteamID = MulNX::MRead(pKillerController->m_steamID());
        auto beKillerSteamID = MulNX::MRead(pBeKillerController->m_steamID());
        uint64_t assisterSteamID = 0;
        if (pAssisterController) {
            assisterSteamID=MulNX::MRead(pAssisterController->m_steamID());
        }
        auto eventTick = this->CS2Time()->GetDemoTick();

        auto [msg, rp] = MulNX::Message::Create<KillEvent>("Game/KillEvent"_hash);
        rp->DemoTick = eventTick;
        rp->attackerSteamId = killerSteamID;
        rp->victimSteamId = beKillerSteamID;
        rp->assisterSteamId = assisterSteamID;
        this->ISys().PublishAsync(std::move(msg));

        if (!this->enable.load(std::memory_order_acquire))return MulNX::Hook::Then::Continue;

        auto currentObservingPawn = this->CS2()->client.TryGetObservingPawn();
        if (!currentObservingPawn)return MulNX::Hook::Then::Return;
        auto hObservingCtrl = MulNX::MRead(currentObservingPawn->m_hController());
        auto pObservingCtrl = this->CS2()->client.GetBaseEntityFromHandle(hObservingCtrl)->As<CS2::CCSPlayerController>();
        if (!pObservingCtrl)return MulNX::Hook::Then::Return;
        auto currentObSteamID = MulNX::MRead(pObservingCtrl->m_steamID());
        if (killerSteamID != currentObSteamID)return MulNX::Hook::Then::Return;
    }
    catch (...) {
        return MulNX::Hook::Then::Return;
    }
    return MulNX::Hook::Then::Continue;
}