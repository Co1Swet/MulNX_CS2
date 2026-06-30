#pragma once
#include <MulNX/Base/Hash/Hash.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <charconv>
#include <stdexcept>
#include <cstring>
#include <array>
#include <algorithm>   // std::max
#include <utility>     // std::index_sequence

// ========== 通用工具 ==========

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

// 字符串 → 任意类型
template<typename T>
T from_string(std::string_view token) {
    T value{};
    if constexpr (std::is_same_v<T, std::string>) {
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
        std::string s(token);
        if constexpr (std::is_same_v<T, float>)       value = std::stof(s);
        else if constexpr (std::is_same_v<T, double>) value = std::stod(s);
        else                                           value = std::stold(s);
    }
    else {
        static_assert(sizeof(T) == 0, "Unsupported argument type");
    }
    return value;
}

// ========== 编译期布局计算 ==========
namespace detail {

    // 有效对齐 / 有效大小：至少 4 字节，且自然对齐
    template<typename T>
    constexpr size_t effective_alignment = std::max(alignof(T), size_t(4));

    template<typename T>
    constexpr size_t effective_size = std::max(sizeof(T), size_t(4));

    // 向上对齐
    constexpr size_t align_to(size_t value, size_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    // 为参数包生成偏移量数组和附加参数区总大小
    template<typename... ArgTs>
    struct Layout {
        static constexpr size_t count = sizeof...(ArgTs);

        // 偏移量数组（从 raw 起始算，0‑7 为 hashid，偏移从 8 开始）
        static constexpr std::array<size_t, count> offsets = []() {
            std::array<size_t, count> arr{};
            size_t current = 8;          // hashid 占用 8 字节
            size_t idx = 0;
            (
                [&]() {
                    current = align_to(current, effective_alignment<ArgTs>);
                    arr[idx++] = current;
                    current += effective_size<ArgTs>;
                }(),
                    ...
                    );
            return arr;
            }();

        // 每个参数的实际 sizeof (用于 sizes 记录)
        static constexpr std::array<size_t, count> real_sizes = { sizeof(ArgTs)... };

        // 附加参数区总大小（从偏移 8 到最后一个参数末尾）
        static constexpr size_t total_args_size = []() {
            if constexpr (count == 0)
                return 0;
            else {
                // 最后一个参数偏移 + 最后一个参数有效大小 - 8
                constexpr size_t last_idx = count - 1;
                return offsets[last_idx] + effective_size<std::tuple_element_t<last_idx, std::tuple<ArgTs...>>> -8;
            }
            }();

        // 整个 raw 缓冲区大小
        static constexpr size_t raw_total_size = 8 + total_args_size;
    };

} // namespace detail

// ========== 解析结果 ==========
struct ParsedArgs {
    std::vector<size_t> sizes;   // [0]: hashid 大小(8), [1..N]: 各参数实际 sizeof
    std::vector<uint8_t> raw;    // 对齐后的连续二进制
};

// 安全读取 raw 中指定偏移的值
template<typename T>
T read_from_raw(const std::vector<uint8_t>& raw, size_t offset) {
    T value;
    std::memcpy(&value, raw.data() + offset, sizeof(T));
    return value;
}

// ========== 创建命令处理器 ==========
template<typename... ArgTs>
std::function<ParsedArgs(std::string)> createHandler(uint64_t hashid) {
    using Layout = detail::Layout<ArgTs...>;

    // 编译期检查：附加参数总大小不得超过 16 字节
    static_assert(Layout::total_args_size <= 16,
        "Total size of aligned arguments (each at least 4 bytes) exceeds 16 bytes. "
        "Reduce the number of parameters or use smaller types.");

    // 捕获编译期数据（按值捕获数组是合法的）
    return [hashid](const std::string& line) -> ParsedArgs {
        constexpr auto offsets = Layout::offsets;
        constexpr auto sizes = Layout::real_sizes;

        auto tokens = split(line);
        if (tokens.empty())
            throw std::runtime_error("Empty command");
        if (MulNX::HashString(std::string(tokens[0])) != hashid)
            throw std::runtime_error("Command hash mismatch");
        if (tokens.size() - 1 < sizeof...(ArgTs))
            throw std::runtime_error("Not enough arguments");

        ParsedArgs result;
        // 预分配并置零（保证填充区为 0）
        result.raw.resize(Layout::raw_total_size, 0);
        // 写入 hashid
        std::memcpy(result.raw.data(), &hashid, sizeof(hashid));
        result.sizes.push_back(sizeof(hashid));

        // 写入各参数
        size_t token_idx = 1;
        (
            [&]() {
                auto value = from_string<ArgTs>(tokens[token_idx]);
                size_t off = offsets[token_idx - 1];   // 参数索引从 0 开始
                std::memcpy(result.raw.data() + off, &value, sizeof(ArgTs));
                result.sizes.push_back(sizeof(ArgTs));
                ++token_idx;
            }(),
                ...
                );

        return result;
        };
}