#pragma once
#include <concepts>
#include <expected>
#include <stdexcept>
#include <type_traits>
#include <utility>

// E 必须是 std::runtime_error 派生类（去掉 cvref 再判断）
template <class E>
concept runtime_error_derived =
std::derived_from<std::remove_cvref_t<E>, std::runtime_error>;

struct value_or_throw_t {
    // 右值：move 出 value / error
    template <class T, class E>
        requires runtime_error_derived<E>
    constexpr T operator()(std::expected<T, E>&& e) const {
        if (!e.has_value()) throw std::move(e).error();
        if constexpr (!std::is_void_v<T>) return std::move(*e);
    }
    // 非 const 左值：拷贝 value / error
    template <class T, class E>
        requires runtime_error_derived<E>
    constexpr T operator()(std::expected<T, E>& e) const {
        if (!e.has_value()) throw e.error();
        if constexpr (!std::is_void_v<T>) return *e;
    }
    // const 左值：拷贝 value / error
    template <class T, class E>
        requires runtime_error_derived<E>
    constexpr T operator()(const std::expected<T, E>& e) const {
        if (!e.has_value()) throw e.error();
        if constexpr (!std::is_void_v<T>) return *e;
    }
};

inline constexpr value_or_throw_t value_or_throw;

// 管道 operator| ，同样带约束
template <class T, class E>
    requires runtime_error_derived<E>
constexpr auto operator|(std::expected<T, E>&& e, value_or_throw_t) -> T {
    return value_or_throw(std::move(e));
}

template <class T, class E>
    requires runtime_error_derived<E>
constexpr auto operator|(std::expected<T, E>& e, value_or_throw_t) -> T {
    return value_or_throw(e);
}

template <class T, class E>
    requires runtime_error_derived<E>
constexpr auto operator|(const std::expected<T, E>& e, value_or_throw_t) -> T {
    return value_or_throw(e);
}
// 给非法 E 更友好的编译错误
template <class T, class E>
    requires (!runtime_error_derived<E>)
T operator|(std::expected<T, E>&&, value_or_throw_t) {
    static_assert(runtime_error_derived<E>,
        "pipe::value_or_throw: E must derive from std::runtime_error");
}
template <class T, class E>
    requires (!runtime_error_derived<E>)
T operator|(std::expected<T, E>&, value_or_throw_t) {
    static_assert(runtime_error_derived<E>,
        "pipe::value_or_throw: E must derive from std::runtime_error");
}
template <class T, class E>
    requires (!runtime_error_derived<E>)
T operator|(const std::expected<T, E>&, value_or_throw_t) {
    static_assert(runtime_error_derived<E>,
        "pipe::value_or_throw: E must derive from std::runtime_error");
}