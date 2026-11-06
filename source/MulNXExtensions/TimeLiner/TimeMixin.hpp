#pragma once
#include "TimeLiner.hpp"

class TimeLiner;
class ImDrawList;

class ITimeRend {
public:
    virtual void TimeRend(const TimeLiner* timeline, ImDrawList* dl) {};
};

template<typename T>
class TimeMixin : public ITimeRend {
    T* This() { return static_cast<T*>(this); }
protected:
    TimeLiner* pTimeline = nullptr;
    TimeMixin() {
        This()->preInits.push_back([this]() {
            this->pTimeline = This()->FindModule<TimeLiner>("TimeLiner");
            this->pTimeline->timeLineModules.push_back(static_cast<ITimeRend*>(this));
            return true;
            });
    }
};

class ITimeAdapter {
public:
    virtual void TimeRend(TimeLiner* timeline, ImDrawList* dl) {};
    virtual double GetMinTime() = 0;
    virtual double GetMaxTime() = 0;

    virtual double GetTime() = 0;
    virtual bool SetTime(double time) = 0;

    virtual void OnSetOn(double time) {};
    virtual void OnSetOff() {};
};

template<typename T>
class TimeAdaptMixin :public ITimeAdapter {
    T* This() { return static_cast<T*>(this); }
protected:
    TimeLiner* pTimeline = nullptr;
    TimeAdaptMixin() {
        This()->preInits.push_back([this]() {
            this->pTimeline = This()->FindModule<TimeLiner>("TimeLiner");
            return true;
            });
    }
};