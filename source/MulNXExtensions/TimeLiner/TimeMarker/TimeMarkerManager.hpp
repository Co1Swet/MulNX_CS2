#pragma once
#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/TimeLiner/TimeMixin.hpp>

class TimeMarkerManager final : public MulNX::Module<TimeMarkerManager>, public TimeMixin<TimeMarkerManager> {
    std::vector<float> markers{};

    void TimeRend(const TimeLiner* timeline, ImDrawList* dl) override;
    bool Init() override;
};
