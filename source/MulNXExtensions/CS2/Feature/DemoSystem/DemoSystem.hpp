#pragma once
#include <Intro/CSModuleBase.hpp>

class DemoSystem final :public CSModuleBase {
    std::set<std::filesystem::path> demoFiles{};
    int selectedDemoIndex = -1;  // 当前选中的列表项索引
    std::filesystem::path dirData{};
    bool Window(MulNX::UICoordinator* uico);
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
};