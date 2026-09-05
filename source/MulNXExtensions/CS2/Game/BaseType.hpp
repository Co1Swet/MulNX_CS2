#pragma once
#include <MulNX/MulNX.hpp>
MULNX_USING(GameTime_t, float);
MULNX_USING(Steam64UID, uint64_t);

template<typename T>
T* Schema(auto* pThis, std::ptrdiff_t dif) {
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(pThis) + dif);
}