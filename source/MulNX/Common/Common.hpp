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

    inline int64_t ToUnixUs(std::chrono::system_clock::time_point tp) {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            tp.time_since_epoch()).count();
    }

    inline std::chrono::system_clock::time_point FromUnixUs(int64_t us) {
        return std::chrono::system_clock::time_point(
            std::chrono::microseconds(us)
        );
    }
}