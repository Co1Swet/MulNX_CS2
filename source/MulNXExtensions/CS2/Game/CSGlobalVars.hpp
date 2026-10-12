#pragma once
#include <cstdint>
#include <MulNX/MulNX.hpp>

class CCSGlobalVars {
public:
    CCSGlobalVars() = delete;

    float realtime;              // 0x00
    int   framecount;            // 0x04
    float frametime;             // 0x08
    float absoluteframetime;     // 0x0C
    uint16_t interpolation_raw;  // 0x0E（从 0x0E 读 4 字节）
    uint16_t pad_0e;             // 0x10 对齐占位

    char  pad0[0x30 - 0x12];     // 0x12 ~ 0x2F

    float curtime;               // 0x30
    char  pad1[0x44 - 0x34];     
    int   tickCount;             // 0x44
};