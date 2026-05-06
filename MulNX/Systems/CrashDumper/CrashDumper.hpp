#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <Windows.h>

namespace MulNX {
    class CrashDumper final :public MulNX::ModuleBase {
        static LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo);
        inline static wchar_t dumpPath[MAX_PATH];
        inline static LPTOP_LEVEL_EXCEPTION_FILTER previousFilter = nullptr;
    public:
        bool Init()override;
    };
}