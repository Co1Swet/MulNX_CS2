#pragma once
#include <Intro/Intros.hpp>
#include <Intro/BackgroundEntityScan/EntityIterationMixin.hpp>

class ObserverController final : public CSModule<ObserverController>, public EntityIterationMixin<ObserverController> {
    void SpecSteam64UID(Steam64UID uid);

    bool Init() override;
    void ProcessMsg(MulNX::Message& Msg) override;
};