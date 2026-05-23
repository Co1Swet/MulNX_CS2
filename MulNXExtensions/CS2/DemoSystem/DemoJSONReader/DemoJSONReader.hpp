#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <MulNXExtensions/CS2/DemoSystem/DemoStruct.hpp>

class DemoJSONReader final : public CSModuleBase {
    std::filesystem::path dirDemos;
    Demo::Info demoInfo;
    bool Window(MulNX::UINode* node);
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};