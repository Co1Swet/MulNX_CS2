#include "HookDemo.hpp"

bool HookDemo::Init() {
    this->SubscribeSync("Hook/RegisterConCommand", [this](MulNX::Message& msg) {
        auto&& [pCmd] = msg.Access<CCmd*>();
        std::string_view name = pCmd->m_pszName;
        if (name == "playdemo") this->HookPlayDemo(pCmd);
        if (name == "demo_gototick")  this->HookDemoGotoTick(pCmd);
        });

    return true;
}
void HookDemo::HookPlayDemo(CCmd* pCmd) {
    auto pf = pCmd->m_pCommandCallback;
    this->hkPlaydemo = MulNX::Hook::Create((uint8_t*)pCmd->m_pCommandCallback, [this](MulNX::Hook* hk, RegContext* ctx) {
        auto cmd = (CCommand*)ctx->rdx;
        hk->CallMaybeOrigin(0, ctx);
        std::string_view raw(cmd->pRawString);
        raw = raw.substr(raw.find(' ') + 1);
        this->LogInfo(std::format("播放Demo: {}", raw));
        return MulNX::Hook::Then::Return;
        }).value();
    this->hkPlaydemo->Attach();
    this->LogSucc(I18n("hook.attached", "PlayDemo"));
}
void HookDemo::HookDemoGotoTick(CCmd* pCmd) {
    auto pf = pCmd->m_pCommandCallback;
    this->hkDemoGotoTick = MulNX::Hook::Create((uint8_t*)pCmd->m_pCommandCallback, [this](MulNX::Hook* hk, RegContext* ctx) {
        auto cmd = (CCommand*)ctx->rdx;
        hk->CallMaybeOrigin(0, ctx);
        auto msg = MulNX::Message("Demo/GotoTick/Complete"_hash);
        auto&& [jumpTick] = msg.Access<int>();
        sscanf_s(cmd->pRawString, "demo_gototick %d", &jumpTick);
        this->LogInfo(std::format("跳转到tick: {}", jumpTick));
        this->PublishAsync(std::move(msg));
        return MulNX::Hook::Then::Return;
        }).value();
    this->hkDemoGotoTick->Attach();
    this->LogSucc(I18n("hook.attached", "DemoGotoTick"));
}