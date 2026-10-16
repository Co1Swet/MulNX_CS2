#pragma once
#include <MulNXExtensions/TimeLiner/ITimeRend.hpp>
#include <Feature/DemoSystem/DemModuleBase.hpp>

class DemoEventsRender final : public DemModuleBase, public ITimeRend {
    void TimeRend(TimeLiner* timeline, ImDrawList* dl) override;
    bool Init() override;
};