#pragma once
#include <cstdint>
#include <cstring>
#include <string_view>

namespace CS2 {
    // 32 字节字符串对象，兼容 MSVC std::string 布局
    class CUtlString {
    public:
        union {
            char* pHeap;          // +0x00
            char    sso[16];        // +0x00
        };
        uint64_t    size;           // +0x10
        uint64_t    capacity;       // +0x18

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

        // 引擎的 assign 函数（base + 0x45060）
        using Assign_t = __int64(__fastcall*)(void*, const char*, uint64_t);
        static inline Assign_t pAssign = nullptr;

        explicit CUtlStringRef(uint64_t* s) : slot(s) {}

        bool IsInitialized() const { return (*this->slot & 2) != 0; }

        CUtlString* Get() const {
            return reinterpret_cast<CUtlString*>(*this->slot & ~3ull);
        }
        std::string_view View() const {
            return this->Get()->View();
        }
        // 统一入口：任何长度都用引擎 assign
        void Assign(std::string_view newStr) {
            CUtlString* p = this->Get();
            pAssign(p, newStr.data(), newStr.size());
        }
    };
} // namespace CS2