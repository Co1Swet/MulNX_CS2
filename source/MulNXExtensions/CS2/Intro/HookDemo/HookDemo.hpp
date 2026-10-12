#pragma once
#include <Intro/HookConsole/HookConsole.hpp>

class HookDemo final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkPlaydemo{};
    std::unique_ptr<MulNX::Hook> hkDemoGotoTick{};
    std::unique_ptr<MulNX::Hook> hkDemeTimescale{};
    MulNX::Hook::Then OnCmdPlayDemo(MulNX::Hook* hk, RegContext* ctx);
    void HookDemoGotoTick(CCmd* cmd);
    void BeforePlay(std::string_view rawArg);
    bool Init()override;
};