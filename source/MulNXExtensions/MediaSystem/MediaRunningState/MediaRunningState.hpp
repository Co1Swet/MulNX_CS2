#pragma once
#include <MulNX/MulNX.hpp>

class MediaRunningState final :public MulNX::Module<MediaRunningState> {
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
public:
    std::atomic<bool> recording = false;
    std::atomic<bool> advancedMode = false;
};