#include "DeathMsgController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Mirror/CS2Hash/CS2Hash.hpp>

void DeathMsgController::Menu() {
    ImGui::SeparatorText("击杀信息控制");
    MulNX::UI::Checkbox("覆盖击杀信息显示时间", this->bOverrideLifeTime);
    MulNX::UI::SliderFloat("目标击杀信息显示时长", this->fLifeTimeOverride, 0.01f, 1000.0f);
    MulNX::UI::Checkbox("覆盖第一人称击杀信息显示时间倍率", this->bOverrideLifeTimeMod);
    MulNX::UI::SliderFloat("目标第一人称击杀信息显示时间倍率", this->fLifeTimeModOverride, 0.01f, 50000.0f);
    ImGui::Separator();
}

bool DeathMsgController::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(CS2::Signatures::Hud::HudDeathNotice_HandlePlayerDeath).FindFuncStart();
        this->hkHudDeathNotice_HandlePlayerDeath = MulNX::Hook::Create(target.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            return this->OnPlayerDeath(ctx);
            }).value();
        this->RegisterAttachHook(this->hkHudDeathNotice_HandlePlayerDeath, "HudDeathNotice_HandlePlayerDeath");
        this->pRawHudDeathNotice_HandlePlayerDeath = reinterpret_cast<HudDeathNotice_HandlePlayerDeath_t>
            (this->hkHudDeathNotice_HandlePlayerDeath->pMaybeRawFunc);
        });

    this->SendTask("Update", "CSControl", [this]()->bool {
        this->Update();
        return true;
        });

    this->UIRegisterCallback("UI.2DVision", [this](auto&&...) {
        this->Menu();
        });

    return true;
}

void DeathMsgController::ProcessMsg(MulNX::Message& msg) {

}

MulNX::Hook::Then DeathMsgController::OnPlayerDeath(RegContext* ctx) {
    static CS2::CKV3MemberName attacker{ this->CS2Hashs->attacker, -1, nullptr };
    static CS2::CKV3MemberName userid{ this->CS2Hashs->userid, -1, nullptr };
    static CS2::CKV3MemberName assister{ this->CS2Hashs->assister, -1, nullptr };

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