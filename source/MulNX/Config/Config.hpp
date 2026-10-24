#pragma once
#include <string>
#include <source_location>
#include <functional>

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

namespace MulNX {
    // 非交互入口可接管本进程的致命错误；未设置或处理后返回时，仍按原流程弹窗并终止。
    extern std::function<void(const std::string&)> errorTerminateHandler;
    [[noreturn]] void ErrorTerminate(const std::string& Msg,
        const std::source_location& loc = std::source_location::current());
}