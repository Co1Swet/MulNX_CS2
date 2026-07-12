#pragma once

class ITimeAdapter {
public:
    virtual float GetMinTime() = 0;
    virtual float GetMaxTime() = 0;

    virtual float GetTime() = 0;
    virtual bool SetTime(float time) = 0;

    virtual void OnSetOn(float time) {};
    virtual void OnSetOff() {};
};