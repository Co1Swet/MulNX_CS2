#pragma once
#include <chrono>

namespace MulNX {
    inline int64_t ToUnixUs(std::chrono::system_clock::time_point tp) {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            tp.time_since_epoch()).count();
    }

    inline std::chrono::system_clock::time_point FromUnixUs(int64_t us) {
        return std::chrono::system_clock::time_point(
            std::chrono::microseconds(us)
        );
    }
}