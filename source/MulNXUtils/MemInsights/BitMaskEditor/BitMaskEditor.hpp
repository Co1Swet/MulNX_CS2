#pragma once
#include <stdio.h>
#include <cstdint>
#include <atomic>

class BitMaskEditor {
public:
    std::atomic<uint64_t> mask;
    BitMaskEditor() : mask(0) {}

    // 渲染 64 个复选框，按 8 位一组分行
    void Render(const char* label = "Bit Mask");
};