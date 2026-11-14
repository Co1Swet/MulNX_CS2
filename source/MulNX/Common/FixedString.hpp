#pragma once
#include <algorithm>
#include <cstddef>
#include <string_view>

namespace MulNX {
    template <std::size_t N>
    struct FixedString {
        char data[N]{};

        constexpr FixedString() = default;

        consteval FixedString(const char(&str)[N]) {
            std::copy_n(str, N, data);
        }
        
        constexpr std::string_view view() const noexcept { return { data, N - 1 }; }
        constexpr const char* c_str() const noexcept { return data; }
        constexpr std::size_t size() const noexcept { return N - 1; }
        constexpr bool empty() const noexcept { return N == 1; }

        constexpr operator std::string_view() const noexcept { return view(); }
    };

    template <std::size_t N>
    FixedString(const char(&)[N]) -> FixedString<N>;

    template <std::size_t N, std::size_t M>
    constexpr auto operator+(const FixedString<N>& a, const FixedString<M>& b) {
        FixedString<N + M - 1> r{};
        std::copy_n(a.data, N - 1, r.data);
        std::copy_n(b.data, M, r.data + N - 1);
        return r;
    }

    template <std::size_t N>
    constexpr auto operator+(const FixedString<N>& a, char c) {
        FixedString<N + 1> r{};
        std::copy_n(a.data, N - 1, r.data);
        r.data[N - 1] = c;
        return r;
    }
}