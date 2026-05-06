#include "CrashDumper.hpp"
#include <MulNX/Systems/PathManager/PathManager.hpp>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

LONG WINAPI MulNX::CrashDumper::UnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo) {
    HANDLE hFile = CreateFileW(MulNX::CrashDumper::dumpPath, GENERIC_WRITE, 0,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei;
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = pExceptionInfo;
        mei.ClientPointers = FALSE;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
            hFile, MiniDumpNormal, &mei, NULL, NULL);
        CloseHandle(hFile);
    }
    if (MulNX::CrashDumper::previousFilter != nullptr)
        return MulNX::CrashDumper::previousFilter(pExceptionInfo);

    return EXCEPTION_EXECUTE_HANDLER;
}

bool MulNX::CrashDumper::Init() {
    auto path = this->ISys().PathManager()->PathGetForShared("Log");

    if (path.empty())
        return false;

    // 确保路径以分隔符结尾，拼接固定文件名
    std::wstring fullPath = path.wstring();
    if (fullPath.back() != L'\\' && fullPath.back() != L'/')
        fullPath += L'\\';
    fullPath += L"crash.dmp";

    if (fullPath.length() >= MAX_PATH)
        return false;
    wcscpy_s(MulNX::CrashDumper::dumpPath, MAX_PATH, fullPath.c_str());

    MulNX::CrashDumper::previousFilter = SetUnhandledExceptionFilter(MulNX::CrashDumper::UnhandledExceptionFilter);

    return true;
}