#pragma once
#include <cmath>

namespace MulNX {
    // Normalize time only. Catmull-Rom and Quaternion Squad remain unchanged.
    inline bool CameraSegmentTime(double time, float begin, float end, bool precise, float& fraction) noexcept {
        if (!std::isfinite(time) || !std::isfinite(begin) || !std::isfinite(end) ||
            end <= begin || time < double(begin) || time > double(end)) return false;
        if (precise) {
            const double duration = double(end) - double(begin);
            fraction = static_cast<float>((time - double(begin)) / duration);
        } else {
            // Preserve the original float operations in the default mode.
            const float duration = end - begin;
            if (!std::isfinite(duration)) return false;
            fraction = (static_cast<float>(time) - begin) / duration;
        }
        return std::isfinite(fraction) && fraction >= 0 && fraction <= 1;
    }
}
