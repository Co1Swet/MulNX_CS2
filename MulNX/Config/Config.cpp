#include "Config.hpp"
#include <MulNX/Base/CharUtility/CharUtility.hpp>
#include <cstdlib>
#include <Windows.h>
#include <sstream>

void MulNX::ErrorTerminate(const std::string& Msg,
    const std::source_location& loc) {
    std::ostringstream oss{};

    oss << "致命错误："
        << "\n\n";

    oss << "错误描述："
        << Msg
        << "\n\n";

    oss << "发生于："
        << "\n文件: " << loc.file_name()
        << "\n函数: " << loc.function_name()
        << "\n行号: " << loc.line()
        << "\n列号: " << loc.column()
        << "\n\n";

    oss << "MulNX将在您点击确定后关闭当前进程";

    std::string Full = oss.str();
    std::wstring wFull = MulNX::CharUtility::U8ToW(Full);

    MessageBoxW(nullptr, wFull.c_str(), L"MulNX 错误中断！", MB_OK);
    std::terminate();
}