#pragma once
#include <MulNX/Base/Hash/Hash.hpp>
#include <cstdint>
#include <string>
#include <source_location>


class MulNXInfo {
public:
    inline static constexpr const char Version[] = MulNXVersion;
    inline static constexpr const char TimeStamp[] = "Built: " __DATE__ " " __TIME__;
    inline static constexpr const char FullName[] = "Multiple Next eXtension";
    inline static constexpr const char DeveloperName[] = "Co1Swet";
#ifdef _DEBUG
    inline static constexpr const bool IsDebugVersion = true;
#else
    inline static constexpr const bool IsDebugVersion = false;
#endif
};

// 核心类前置声明

namespace MulNX {
    [[noreturn]] void ErrorTerminate(const std::string& Msg,
        const std::source_location& loc = std::source_location::current());
}

// 辅助模板：将函数签名 R(Args...) 转换为对应的函数指针类型 R(*)(Args...)
template<typename T>
struct MulNXFunc;

template<typename R, typename... Args>
struct MulNXFunc<R(Args...)> {
    using type = R(*)(Args...);
};

template<typename T>
T* Schema(auto* pThis, std::ptrdiff_t dif) {
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(pThis) + dif);
}