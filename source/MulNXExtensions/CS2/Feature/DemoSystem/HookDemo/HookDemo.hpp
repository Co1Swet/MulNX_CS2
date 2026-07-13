#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Intro/HookConsole/HookConsole.hpp>

class HookDemo final :public CSModuleBase {
    WrapHook hkPlaydemo{};
    WrapHook hkDemoGotoTick{};
    void HookPlayDemo(CCmd* cmd);
    void HookDemoGotoTick(CCmd* cmd);
    void BeforePlay(std::string_view rawArg);
    bool Init()override;
};