#pragma once

class TimeLiner;
class ImDrawList;
class ITimeRend {
public:
    virtual void TimeRend(TimeLiner* timeline, ImDrawList* dl) {};
};