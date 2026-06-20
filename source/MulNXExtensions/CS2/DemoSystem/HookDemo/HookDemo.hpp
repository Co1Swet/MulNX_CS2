#pragma once
#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <MulNXExtensions/CS2/HookConsole/HookConsole.hpp>

class HookDemo final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook>hkPlaydemo = nullptr;
    std::unique_ptr<MulNX::Hook>hkDemoGotoTick = nullptr;
    void HookPlayDemo(CCmd* cmd);
    void HookDemoGotoTick(CCmd* cmd);
public:
    bool Init()override;
};