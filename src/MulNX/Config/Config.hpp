#pragma once

#include <cstdint>
#include <string>
#include <source_location>
#include <MulNX/Base/Hash/Hash.hpp>

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

using GameTime_t = float;
// 摄像机系统输入输出前置声明
class CameraSystemIO;

// 核心类前置声明

namespace MulNX {
    namespace Core {
        class Core;
        class ModuleManager;
        class CoreStarterBase;
    }
    class ModuleBase;
    class Debugger;
    class HandleSystem;
    class IPCer;
    class InputSystem;
    class GlobalVars;
    class UISystem;
    class Message;
    using MsgType = size_t;
    class MessageManager;
    class MessageChannel;
    class PathManager;
    class UINode;
    class I18nManager;
    class Logger;
    class ShortcutManager;

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