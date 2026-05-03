#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class ObserverController final : public CSModuleBase {
private:
    bool CampathPlaying = false;
    uint8_t currentMode = 0;         // 当前检测到的模式
    uint8_t startedAsSpecMode = 0;   // 0:未记录, 2:原是mode2, 4:原是mode4
private:
    bool HandlePlayStarted();
    bool HandlePlayEnded();
    void OnSpecModeChanged(uint8_t newMode);
    bool SpecHandle(CS2::CHandleBase handle);
    CS2::CCSPlayerController* FindControllerBySteam64UID(Steam64UID uid);
    void SetSpecMode(uint8_t mode);
public:
    bool Init() override;
    void ProcessMsg(MulNX::Message& Msg) override;
    void Main();
    void UpdateObserverState();   // 只负责轮询并发布 spec_mode_changed_to 事件
};