#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Intro/HookConsole/HookConsole.hpp>

class HookDemo final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook>hkPlaydemo = nullptr;
    std::unique_ptr<MulNX::Hook>hkDemoGotoTick = nullptr;
    void HookPlayDemo(CCmd* cmd);
    void HookDemoGotoTick(CCmd* cmd);
public:
    bool Init()override;
};