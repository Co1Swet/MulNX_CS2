#pragma once
#include <Intro/HookConsole/HookConsole.hpp>
#include <Feature/DemoSystem/DemBase/DemModuleBase.hpp>

class HookDemo final :public DemModuleBase {
    std::unique_ptr<MulNX::Hook> hkPlaydemo{};
    std::unique_ptr<MulNX::Hook> hkDemoGotoTick{};
    void HookPlayDemo(CCmd* cmd);
    void HookDemoGotoTick(CCmd* cmd);
    void BeforePlay(std::string_view rawArg);
    bool Init()override;
};