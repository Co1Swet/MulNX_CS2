#pragma once
#include <MulNX/MulNX.hpp>

class DLLInjectHelper final :public MulNX::Module<DLLInjectHelper> {
public:
    bool Init()override;
    bool InjectDll(HANDLE hProcess, const std::wstring& dllPath);
    bool InitDLL(HANDLE hProcess, const std::wstring& dllName, const char* initFuncName);
};