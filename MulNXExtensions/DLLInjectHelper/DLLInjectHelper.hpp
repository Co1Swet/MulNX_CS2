#pragma once

#include <MulNX/MulNX.hpp>

class DLLInjectHelper final :public MulNX::ModuleBase {
public:
    bool Init()override;
    bool InjectDll(HANDLE hProcess, const std::wstring& dllPath);
    bool InitDLL(HANDLE hProcess, const std::wstring& dllName, const char* initFuncName);
};