#pragma once

#include <Windows.h>
#include <string>

FARPROC GetRemoteProcAddress(HANDLE hProcess, const std::wstring& moduleName, const char* funcName);