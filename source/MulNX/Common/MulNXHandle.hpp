#pragma once
#include <compare>
#include <cstdint>
#include <atomic>

// MulNX资源句柄
class MulNXHandle {
private:
    constexpr inline static uint64_t Invalid = 0xFFFFFFFFFFFFFFFF;
    inline static std::atomic<uint64_t> CurrentHandleValue = 16;
    uint64_t Value;
public:
    // 默认构造函数创建无效句柄
    MulNXHandle() {
        this->Value = MulNXHandle::Invalid;
    }
    static MulNXHandle CreateHandle() {
        MulNXHandle handle{};
        handle.Value = MulNXHandle::CurrentHandleValue.fetch_add(1);
        return handle;
    }
    bool IsValid()const {
        return this->Value != MulNXHandle::Invalid;
    }
    uint64_t GetValue()const {
        return this->Value;
    }
    bool operator == (const MulNXHandle& Other)const = default;
    auto operator<=>(const MulNXHandle&)const = default;
};
namespace std {
    template<>
    struct hash<MulNXHandle> {
        size_t operator()(const MulNXHandle& Handle)const noexcept {
            return std::hash<uint64_t>{}(Handle.GetValue());
        }
    };
}