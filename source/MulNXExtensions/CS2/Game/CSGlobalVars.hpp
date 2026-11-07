#pragma once

#pragma pack(push, 1)
struct CCSGlobalVars {
    float realtime;              // 0x00
    int   framecount;            // 0x04
    float frametime;             // 0x08
    float absoluteframetime;     // 0x0C

    char  pad_0x10[0x0C];        // 0x10 (跳过前三个)
    float interval_per_tick;     // 0x1C (1/64)
    char  pad_0x20[0x10];        // 0x20 ~ 0x2F

    float curtime;               // 0x30
    char  pad_0x34[4];           // 0x34 (跳过1个)
    float interpolation_amount;  // 0x38 (在 0~1 之间循环)
    // 这个和上面那个在很多情况一样，具体没细看
    float unkFloat;              // 0x3C
    char  pad_0x40[4];           // 0x40

    int   tickCount;             // 0x44
};
#pragma pack(pop)