#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class DemoSystem final :public CSModuleBase {
    std::set<std::filesystem::path> DemoFiles{};
    int selectedDemoIndex = -1;  // 当前选中的列表项索引
    bool Window(MulNX::UINode* node);
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
};