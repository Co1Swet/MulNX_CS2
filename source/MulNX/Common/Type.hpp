#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <type_traits>
#include <concepts>

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

// 辅助模板：将函数签名 R(Args...) 转换为对应的函数指针类型 R(*)(Args...)
template<typename T>
struct MulNXFunc;

template<typename R, typename... Args>
struct MulNXFunc<R(Args...)> {
    using type = R(*)(Args...);
};

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