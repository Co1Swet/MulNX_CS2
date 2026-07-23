#pragma once
#include "TimeLiner.hpp"

template<typename T>
class TimeMixin {
    T* This() { return static_cast<T*>(this); }
public:
    TimeLiner* pTimeline = nullptr;
    TimeMixin() {
        This()->preInits.push_back([this]() {
            this->pTimeline = static_cast<TimeLiner*>(This()->FindModule("TimeLiner"));
            return true;
            });
    }
};