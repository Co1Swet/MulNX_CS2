#include "Config.hpp"
#include <MulNX/Base/CharUtility/CharUtility.hpp>
#include <cstdlib>
#include <Windows.h>
#include <sstream>

void MulNX::ErrorTerminate(const std::string& msg, const std::source_location& loc) {
    std::ostringstream oss{};

    auto str = std::format(
        "致命错误：\n"
        "\n"
        "\n"
        "错误描述：{}\n"
        "\n"
        "\n"
        "    发生于： \n"
        "    文件: {} \n"
        "    函数: {} \n "
        "    行号: {} \n "
        "    列号: {} \n "
        "\n"
        "\n"
        "版本号：{} \n"
        "请将此提示截图以及根目录/Log下的 Log_CS2Injector.txt 和 Log_CS2OBTool.txt交给开发者\n"
        "这对问题修复可能至关重要\n"
        "MulNX将在您点击确定后关闭当前进程",
        msg, loc.file_name(), loc.function_name(), loc.line(), loc.column(), MulNXVersion
    );

    std::wstring wFull = MulNX::CharUtility::U8ToW(str);
    MessageBoxW(nullptr, wFull.c_str(), L"MulNX 错误中断！", MB_OK);
    std::terminate();
}