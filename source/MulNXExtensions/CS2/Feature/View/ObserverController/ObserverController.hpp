#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Intro/BackgroundEntityScan/EntityIterationMixin.hpp>

class ObserverController final : public CSModuleBase, public EntityIterationMixin<ObserverController> {
    void SpecSteam64UID(Steam64UID uid);

    bool Init() override;
    void ProcessMsg(MulNX::Message& Msg) override;
};