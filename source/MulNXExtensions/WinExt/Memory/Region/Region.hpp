#pragma once

#include "../Pattern/Pattern.hpp"
#include <Windows.h>

namespace MulNX {
    namespace Memory {
        // 内存区域类，表示一个连续的内存块，提供迭代器支持
        // 左闭右开区间 [Base, End)，Size = End - Base
        class Region {
        private:
            uintptr_t Base = 0;
            size_t Size = 0;
            const size_t RawSize;
            DWORD Protection = 0;// 当前保护属性
        
            std::optional<uint8_t*> FindHead(const uint8_t* Begin, const uint8_t Byte)const;
            bool MatchPattern(const uint8_t* Address, const Pattern& Pattern)const;
        public:
            Region() = delete;
            Region(uintptr_t Base, size_t Size);
            static Region InValid() { return Region(0, 0); }
            // 迭代器支持
            const uint8_t* Begin()const { return reinterpret_cast<const uint8_t*>(Base); }
            const uint8_t* End()const { return reinterpret_cast<const uint8_t*>(Base + Size); }
            size_t GetSize() const { return this->Size; }
            uint8_t* Data() { return reinterpret_cast<uint8_t*>(Base); }
            uint8_t* Rdata() { return reinterpret_cast<uint8_t*>(Base + Size); }
            bool IsValid() const { return Base != 0 && Size != 0; }
            DWORD protection() const { return Protection; }
            bool TryResize(size_t NewSize);

            Region FindRegion(const Pattern& pattern)const;

            Region FindFuncStart();
        };
    }
}