#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>

class RecordTaskConfiger final :public DemModuleBase {
    void Window();
    void ProcessMsg(MulNX::Message& msg)override;
    bool Init()override;

    // 邻近拼合阈值（秒）
    std::atomic<float> mergeThresholdSeconds = 12.0f;
public:
    int preRecordTicks = 200;
    int postRecordTicks = 200;

    int preRecordTicksBekilled = 100;
    int postRecordTicksBekilled = 100;

    bool enableShotingTime = false;
    int preTicksShotingTime = 40;
    int postTicksShotingTime = 40;
    float ShotingTimeRate = 0.2;

    

    int postTickRoundEnd = 64;

    int GetMergeThresholdTicks()const;
};