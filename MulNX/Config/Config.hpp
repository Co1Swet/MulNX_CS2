#pragma once

#include <cstdint>
#include <string>
#include <source_location>
#include <MulNX/Base/Hash/Hash.hpp>

using GameTime_t = float;
// 摄像机系统输入输出前置声明
class CameraSystemIO;

// 核心类前置声明

namespace MulNX {
    namespace Core {
        // 核心管理器
        class Core;
        // 模块管理器
        class ModuleManager;
        // 核心启动器基类
        class CoreStarterBase;
    }
    // 模块基类
    class ModuleBase;
    // 调试器接口
    class Debugger;
    // MulNX句柄系统
    class HandleSystem;
    // 跨进程信息接口
    class IPCer;
    // 输入系统
    class InputSystem;
    // 全局变量
    class GlobalVars;
    // MulNX UI系统
    class UISystem;
    // MulNX消息
    class Message;
    // MulNX消息类型
    using MsgType = size_t;
    // 消息管理器
    class MessageManager;
    // 消息管道
    class MessageChannel;
    // 路径管理器
    class PathManager;
    // UI节点
    class UINode;

    class I18nManager;
    class Logger;

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