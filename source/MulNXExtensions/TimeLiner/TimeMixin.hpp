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
    virtual float GetMinTime() = 0;
    virtual float GetMaxTime() = 0;

    virtual float GetTime() = 0;
    // Separate opt-in path; existing consumers retain their original clock API.
    virtual MulNX::CameraPathTime GetCameraPathTime() {
        const float time = GetTime();
        MulNX::CameraPathTime sample;
        sample.time = time;
        return sample;
    }
    virtual bool SetTime(float time) = 0;

    virtual void OnSetOn(float time) {};
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