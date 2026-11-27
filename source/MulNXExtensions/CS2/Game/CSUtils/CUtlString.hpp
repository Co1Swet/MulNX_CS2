#pragma once
#include "CSMemory.hpp"
#include <cstdint>
#include <string_view>

namespace CS2 {
    // CS2 的 32 字节字符串对象
    class CUtlString {
    public:
        union {
            char* pHeap;          // +0x00
            char sso[16];        // +0x00
        };
        uint64_t size = 0;           // +0x10
        uint64_t capacity = 15;       // +0x18

        // ── 基础访问 ──
        bool IsHeap() const { return this->capacity > 0xF; }

        char* Data() {
            return this->IsHeap() ? this->pHeap : this->sso;
        }
        const char* Data() const {
            return this->IsHeap() ? this->pHeap : this->sso;
        }

        uint64_t Size() const { return this->size; }
        bool     Empty() const { return this->size == 0; }

        std::string_view View() const {
            return std::string_view(this->Data(), (size_t)this->size);
        }

        using Assign_t = __int64(*)(CUtlString*, const char*, uint64_t);
        static inline Assign_t pAssign = nullptr;
        inline void Assign(const char* in, uint64_t size) {
            this->pAssign(this, in, size);
        }

        CUtlString() : pHeap(nullptr), size(0), capacity(15) {}
        ~CUtlString() {
            if (!this->IsHeap() || !this->pHeap) return;
            void* p = this->pHeap;
            if (this->capacity + 1 >= 0x1000)
                p = *reinterpret_cast<void**>(reinterpret_cast<char*>(pHeap) - 8);
            CS2::Memory::pFree(p);          // g_pMemAlloc->Free
            this->pHeap = nullptr;
            this->capacity = 15;
            this->size = 0;
        }

        CUtlString(const CUtlString&) = delete;
        CUtlString& operator=(const CUtlString&) = delete;

        // ── 带标记指针的解包 ──
        // 字段槽存的是 CUtlString* 带低 2 位标记
        static CUtlString* Unpack(uint64_t tagged) {
            return reinterpret_cast<CUtlString*>(tagged & ~3ull);
        }
        static bool IsInitialized(uint64_t tagged) {
            return (tagged & 2) != 0;
        }
        static uint64_t Pack(CUtlString* p, uint32_t tag) {
            return reinterpret_cast<uint64_t>(p) | tag;
        }
    };

    class CUtlStringRef {
    public:
        uint64_t* slot;

        explicit CUtlStringRef(uint64_t* s) : slot(s) {}

        bool IsInitialized() const { return (*this->slot & 2) != 0; }

        bool IsValid(uint64_t emptySentinelAddr) const {
            if ((*this->slot & 3) == 0) return false;
            CUtlString* p = this->Get();
            if (!p) return false;
            return true;
        }

        CUtlString* Get() const {
            return reinterpret_cast<CUtlString*>(*this->slot & ~3ull);
        }
        std::string_view View() const {
            return this->Get()->View();
        }
        // 统一入口：任何长度都用引擎 assign
        void Assign(const char* in,uint64_t size) {
            this->Get()->Assign(in, size);
        }
    };
} // namespace CS2