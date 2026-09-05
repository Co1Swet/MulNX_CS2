#pragma once
#include <MulNX/Common/Message.hpp>
#include <MulNX/Common/Hash.hpp>
#include <functional>
#include <string_view>
#include <stdexcept>
#include <utility>

namespace MulNX {
    // 字符串分割
    inline std::vector<std::string_view> split(std::string_view str, char delim = ' ') {
        std::vector<std::string_view> tokens;
        size_t start = 0, end;
        while (start <= str.size()) {
            end = str.find(delim, start);
            if (end == std::string_view::npos) end = str.size();
            if (end > start)
                tokens.push_back(str.substr(start, end - start));
            start = end + 1;
        }
        return tokens;
    }

    // 字符串转类型（支持基础类型、字符串、枚举类）
    template<typename T>
    T from_string(std::string_view token) {
        T value{};
        if constexpr (std::is_enum_v<T>) {
            // 枚举：先解析为其底层类型，再转换
            using Underlying = std::underlying_type_t<T>;
            Underlying temp{};
            int base = 10;
            const char* start = token.data();
            if (token.size() >= 2 && token[0] == '0' &&
                (token[1] == 'x' || token[1] == 'X')) {
                base = 16;
                start += 2;
            }
            auto [ptr, ec] = std::from_chars(start, token.data() + token.size(), temp, base);
            if (ec != std::errc{} || ptr != token.data() + token.size())
                throw std::invalid_argument("Invalid enum value: " + std::string(token));
            value = static_cast<T>(temp);
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            value = T(token);
        }
        else if constexpr (std::is_integral_v<T>) {
            int base = 10;
            const char* start = token.data();
            if (token.size() >= 2 && token[0] == '0' &&
                (token[1] == 'x' || token[1] == 'X')) {
                base = 16;
                start += 2;
            }
            auto [ptr, ec] = std::from_chars(start, token.data() + token.size(), value, base);
            if (ec != std::errc{} || ptr != token.data() + token.size())
                throw std::invalid_argument("Invalid number: " + std::string(token));
        }
        else if constexpr (std::is_floating_point_v<T>) {
            char* end = nullptr;
            if constexpr (std::is_same_v<T, float>)
                value = std::strtof(token.data(), &end);
            else if constexpr (std::is_same_v<T, double>)
                value = std::strtod(token.data(), &end);
            else
                value = std::strtold(token.data(), &end);
            if (end != token.data() + token.size())
                throw std::invalid_argument("Invalid float: " + std::string(token));
        }
        else {
            static_assert(sizeof(T) == 0, "Unsupported type for command argument");
        }
        return value;
    }

    /**
     * 创建一个填充器（Filler）
     * 返回的 callable 接受 Message& 和一个仅包含参数的 string_view，
     * 它通过 Message::Access<ArgTs...>() 获取引用，将参数字符串转换为对应类型并赋值。
     * 如果参数数量不足或转换失败，将抛出异常。
     */
    template <Pod... ArgTs>
    std::function<void(Message&, std::string_view)> CreateFiller() {
        return [](Message& msg, std::string_view payload) {
            auto tokens = split(payload);
            if (tokens.size() < sizeof...(ArgTs)) {
                throw std::runtime_error("Insufficient arguments for command filler");
            }

            // 通过 Access 获取 extra 中对应类型的引用元组
            auto args = msg.Access<ArgTs...>();

            // 遍历并赋值
            size_t idx = 0;
            auto assign_one = [&]<size_t I>(auto& /* dummy */) {
                using T = std::tuple_element_t<I, std::tuple<ArgTs...>>;
                std::get<I>(args) = from_string<T>(tokens[idx]);
                ++idx;
            };

            [&] <size_t... Is>(std::index_sequence<Is...>) {
                (assign_one.template operator() < Is > (args), ...);
            }(std::make_index_sequence<sizeof...(ArgTs)>{});
            };
    }
} // namespace MulNX