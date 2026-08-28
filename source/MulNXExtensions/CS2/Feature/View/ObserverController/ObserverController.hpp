#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Intro/BackgroundEntityScan/EntityIterationMixin.hpp>

class ObserverController final : public CSModuleBase, public EntityIterationMixin<ObserverController> {
    void OnSpecModeChanged(uint8_t newMode);
    bool SpecHandle(CS2::CHandleBase handle);
    void SetSpecMode(uint8_t mode);
    void SpecSteam64UID(Steam64UID uid);

    bool Init() override;
    void ProcessMsg(MulNX::Message& Msg) override;
    void Main();
    void UpdateObserverState();   // 只负责轮询并发布 spec_mode_changed_to 事件
    bool SpecPlayer(int IndexInMap);
};