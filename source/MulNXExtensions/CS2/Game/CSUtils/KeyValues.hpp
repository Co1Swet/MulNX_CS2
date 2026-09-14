#pragma once
#include <cstdint>

namespace CS2 {
    class KeyValues final {
    public:
        KeyValues() = delete;

        using SetString_t = void(*)(void*, const char*, const char*);
        inline static SetString_t pFuncSetString = nullptr;
        using FindKey_t = uint64_t(*)(void*, const char*, bool);
        inline static FindKey_t pFuncFindKey = nullptr;

        void SetString(const char* target, const char* value) {
            this->pFuncSetString(this, target, value);
        }
        uint64_t FindKey(const char* name, bool unk) {
            return this->pFuncFindKey(this, name, unk);
        }
    };
}