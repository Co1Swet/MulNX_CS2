#pragma once

#include <cstdint>
#include <MulNX/MulNX.hpp>

class C_GlobalVars {
public:
    // float RealTime{};
    // int FrameCount{};
    // int MaxClients{};
    // float IntervalPerTick{};
    // float CurrentTime{};
    // int TickCount{};
    // float IntervalPerTick2{};
    // char* CurrentMapName = nullptr;

    float* fRealTime() { return Schema<float>(this, 0x00); }
    float* fCurrentTime() { return Schema<float>(this, 0x30); }
    int* iTickCount() { return Schema<int>(this, 0x44); }
};