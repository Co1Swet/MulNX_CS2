#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Feature/DemoSystem/DemoStruct.hpp>

class RecordTaskConfiger final :public CSModuleBase {
    void Window();
    void ProcessMsg(MulNX::Message& msg)override;
    bool Init()override;
public:
    int preRecordTicks = 200;
    int postRecordTicks = 200;

    int preRecordTicksBekilled = 100;
    int postRecordTicksBekilled = 100;

    bool enableShotingTime = false;
    int preTicksShotingTime = 40;
    int postTicksShotingTime = 40;
    float ShotingTimeRate = 0.2;

    // 邻近拼合阈值（tick）
    float mergeThresholdTicks = 480.0f;
};