#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <MulNXExtensions/CS2/DemoSystem/DemoStruct.hpp>

class RecordTaskConfiger final :public CSModuleBase {
    bool Window(MulNX::UINode* node);
    void ProcessMsg(MulNX::Message& msg)override;
public:
    bool Init()override;

    int preRecordTicks = 200;
    int postRecordTicks = 200;

    int preRecordTicksBekilled = 100;
    int postRecordTicksBekilled = 100;

    bool enableShotingTime = false;
    int preTicksShotingTime = 40;
    int postTicksShotingTime = 40;
    float ShotingTimeRate = 0.2;
};