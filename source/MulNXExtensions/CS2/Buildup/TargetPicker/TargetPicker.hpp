#pragma once
#include <Intro/CSModuleBase.hpp>

class TargetPicker final :public CSModuleBase {
    Steam64UID lastDetected = 0;
    std::atomic<Steam64UID> target = 0;
    std::atomic<std::chrono::steady_clock::time_point>lastUpdateTime;
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    void Main();
public:
    bool UpdateTarget();
    Steam64UID GetTarget()const;
    void SetTarget(Steam64UID uid);
    std::chrono::steady_clock::time_point GetLastUpdateTime();
};