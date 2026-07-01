#pragma once
#include <cstddef>
#include <type_traits>
#include <chrono>
#include <concepts>

template <typename F>
class scope_exit {
    F f;
public:
    explicit scope_exit(F&& func) : f(std::forward<F>(func)) {}
    ~scope_exit() { f(); }
    scope_exit(const scope_exit&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;
};

namespace MulNX {
    template<typename T>
    concept Pod = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

    template<typename T, size_t size>
    concept PodSize = Pod<T> && sizeof(T) == size;

    template<typename T, size_t min, size_t max>
    concept PodSizeIn = Pod<T> && sizeof(T) > min && sizeof(T) <= max;

    template <Pod... Ts>
    consteval auto ComputeOffsets() noexcept {
        constexpr size_t alignments[] = { alignof(Ts)... };
        constexpr size_t sizes[] = { sizeof(Ts)... };
        std::array<size_t, sizeof...(Ts)> offsets{};
        size_t current = 0;
        for (size_t i = 0; i < sizeof...(Ts); ++i) {
            // 将当前偏移向上对齐到该类型的对齐值
            current = (current + alignments[i] - 1) & ~(alignments[i] - 1);
            offsets[i] = current;
            current += sizes[i];
        }
        return offsets;
    }

    inline int64_t ToUnixUs(std::chrono::system_clock::time_point tp) {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            tp.time_since_epoch()).count();
    }

    inline std::chrono::system_clock::time_point FromUnixUs(int64_t us) {
        return std::chrono::system_clock::time_point(
            std::chrono::microseconds(us)
        );
    }

    // 类型名萃取（可定制）
    template <typename T>
    struct TypeName {
        static std::string get() {
            return std::string(typeid(T).name());
        }
    };

    template <typename T>
    std::string SingleTypeString() {
        return TypeName<T>::get();
    }

    // 递归展开参数包，生成 "<T1> <T2> ..." 格式
    template <typename... Args>
    std::string GetTypeString() {
        if constexpr (sizeof...(Args) == 0) {
            return "";
        }
        else {
            std::string result;
            ((result += "<" + SingleTypeString<Args>() + "> "), ...);
            if (!result.empty()) result.pop_back(); // 删除末尾空格
            return result;
        }
    }
}

// 便捷宏 —— 定义别名的同时注册类型名
#define MULNX_USING(alias, underlying) \
    using alias = underlying;          \
    template<> struct ::MulNX::TypeName<alias> { \
        static std::string get() { return #alias; } \
    }

// 枚举注册宏 —— 必须传入完整的类型名（含命名空间）
#define MULNX_ENUMCLASS(EnumType) \
    template<> struct ::MulNX::TypeName<EnumType> { \
        static std::string get() { return #EnumType; } \
    }