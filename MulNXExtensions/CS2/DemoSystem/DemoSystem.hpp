#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class DemoSystem final :public CSModuleBase {
    std::set<std::filesystem::path> demoFiles{};
    int selectedDemoIndex = -1;  // 当前选中的列表项索引
    std::filesystem::path pathDemos;
    bool Window(MulNX::UINode* node);
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
};