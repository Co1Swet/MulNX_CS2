#pragma once
#include <MulNXExtensions/TimeLiner/TimeMixin.hpp>
#include <Feature/DemoSystem/DemModuleBase.hpp>

class DemoEventsRender final : public DemModuleBase, public TimeMixin<DemoEventsRender> {
    void TimeRend(TimeLiner* timeline, ImDrawList* dl)override;
    bool Init() override;
};