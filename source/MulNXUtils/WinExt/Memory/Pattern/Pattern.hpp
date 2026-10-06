#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <optional>

namespace MulNX {
    namespace Memory {
        // 内存模式类，表示一个特定的字节模式，包含通配符为?，提供匹配功能
        class Pattern {
            static constexpr size_t Capacity = 256;
            std::array<std::optional<uint8_t>, Capacity> Bytes{};
            size_t Length = 0;

            static consteval int HexValue(char Character) {
                if (Character >= '0' && Character <= '9') return Character - '0';
                if (Character >= 'a' && Character <= 'f') return Character - 'a' + 10;
                if (Character >= 'A' && Character <= 'F') return Character - 'A' + 10;
                return -1;
            }

        public:
            template <size_t N>
            consteval Pattern(const char (&Raw)[N]) {
                size_t Position = 0;
                while (Position < N - 1) {
                    if (Raw[Position] == ' ') {
                        ++Position;
                        continue;
                    }
                    if (Position + 1 >= N - 1) {
                        throw "Invalid memory pattern format";
                    }
                    if (Raw[Position] == '?' && Raw[Position + 1] == '?') {
                        if (this->Length == Capacity) {
                            throw "Memory pattern exceeds maximum length";
                        }
                        this->Bytes[this->Length++] = std::nullopt;
                        Position += 2;
                        continue;
                    }
                    const int High = HexValue(Raw[Position]);
                    const int Low = HexValue(Raw[Position + 1]);
                    if (High < 0 || Low < 0) {
                        throw "Invalid memory pattern format";
                    }
                    if (this->Length == Capacity) {
                        throw "Memory pattern exceeds maximum length";
                    }
                    this->Bytes[this->Length++] = static_cast<uint8_t>((High << 4) | Low);
                    Position += 2;
                }
            }

            Pattern(std::string&& Raw);
            const uint8_t* First() const { return &Bytes[0].value(); }
            size_t size() const { return this->Length; }
            std::optional<uint8_t> operator[](size_t index) const { return Bytes[index]; }
        };
    }
}