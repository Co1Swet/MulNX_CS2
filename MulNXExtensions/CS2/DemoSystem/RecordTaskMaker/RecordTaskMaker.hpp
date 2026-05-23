#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <MulNXExtensions/CS2/DemoSystem/DemoStruct.hpp>

class RecordTaskMaker final :public CSModuleBase {
    std::map<std::string, Demo::Info>demos;
public:
    bool Init()override;
};