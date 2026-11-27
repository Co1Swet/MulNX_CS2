#pragma once

namespace CS2 {
    class Memory {
    public:
        using Free_t = void(*)(void*);
        inline static Free_t pFree = nullptr;
    };
}