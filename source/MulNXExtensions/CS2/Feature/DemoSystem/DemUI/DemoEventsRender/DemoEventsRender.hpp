#pragma once
#include <MulNXExtensions/TimeLiner/TimeMixin.hpp>
#include <Feature/DemoSystem/DemModuleBase.hpp>

class DemoEventsRender final : public DemModule<DemoEventsRender>, public TimeMixin<DemoEventsRender> {
    void TimeRend(const TimeLiner* timeline, ImDrawList* dl)override;
    bool Init() override;
};