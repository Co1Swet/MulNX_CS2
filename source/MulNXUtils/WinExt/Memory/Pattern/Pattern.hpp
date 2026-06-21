#pragma once

#include <string>
#include <vector>
#include <optional>

namespace MulNX {
    namespace Memory {
        // 内存模式类，表示一个特定的字节模式，包含通配符为?，提供匹配功能
        class Pattern {
            std::vector<std::optional<uint8_t>> Bytes;// 解析后的字节数组，其中std::nullopt表示通配符
        public:
            Pattern(std::string&& Raw);
            const uint8_t* First() const { return &Bytes[0].value(); }
            size_t size() const { return Bytes.size(); }
            std::optional<uint8_t> operator[](size_t index) const { return Bytes[index]; }
        };
    }
}