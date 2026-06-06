#pragma once

#include <string>

namespace {
    // FNV-1a 64位常数
    constexpr size_t fnv_basis = 14695981039346656037ULL;
    constexpr size_t fnv_prime = 1099511628211ULL;
}

namespace MulNX {
    // 使用 FNV-1a 算法的编译时运行时 std::string 的哈希函数
    constexpr size_t HashString(const std::string& str) noexcept {
        size_t hash = fnv_basis;
        for (unsigned char c : str) {
            hash = (hash ^ static_cast<size_t>(c)) * fnv_prime;
        }
        return hash;
    }
}

// 编译时 std::string 的哈希函数
consteval size_t operator"" _hash(const char* str, size_t n) {
    std::string Str(str, n);
    return MulNX::HashString(Str);
}