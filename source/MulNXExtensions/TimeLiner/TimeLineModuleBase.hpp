#pragma once
#include "TimeLiner.hpp"

class ITimeLineModule {
public:
    virtual void TimeLineCallback(TimeLiner* timeline, ImDrawList* dl) = 0;
};