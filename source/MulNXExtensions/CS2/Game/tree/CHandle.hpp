#pragma once
#include <cstdint>

namespace CS2 {
    class CHandleBase {
    public:
        uint32_t value;
        CHandleBase(uint32_t handle) :value(handle) {}
        int GetIndexInEntityList() { return this->value & 0x7FFF; }
        bool Valid() { return this->value != 0xFFFFFFFF; }

        bool operator<(const CHandleBase& rhs) const noexcept {
            return this->value < rhs.value;
        }

        bool operator==(const CHandleBase& rhs) const noexcept {
            return this->value == rhs.value;
        }

        CHandleBase() :value(0xFFFFFFFF) {}
    };

    template<typename T>
    class CHandle :public CHandleBase {
    public:

    };

    template<typename T>
    class CStrongHandle {
    public:
        T* m_pObject;

        CStrongHandle() = delete;
        CStrongHandle(const CStrongHandle&) = delete;
        CStrongHandle(CStrongHandle&&) = delete;
        CStrongHandle& operator=(const CStrongHandle&) = delete;
        CStrongHandle& operator=(CStrongHandle&&) = delete;
        ~CStrongHandle() = delete;
    };
}