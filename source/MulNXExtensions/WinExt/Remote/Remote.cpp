#include "Remote.hpp"
#include <psapi.h>

HMODULE GetRemoteModuleHandle(HANDLE hProcess, const std::wstring& moduleName) {
    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (unsigned i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            wchar_t szModName[MAX_PATH];
            if (GetModuleBaseNameW(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(wchar_t))) {
                if (_wcsicmp(szModName, moduleName.c_str()) == 0) {
                    return hMods[i];
                }
            }
        }
    }
    return nullptr;
}

FARPROC GetRemoteProcAddress(HANDLE hProcess, const std::wstring& moduleName, const char* funcName) {
    // 1. 获取远程模块基址
    HMODULE hRemoteModule = GetRemoteModuleHandle(hProcess, moduleName);
    if (!hRemoteModule) return nullptr;

    // 2. 本地获取同样 DLL 的基址和函数地址（依赖本地已加载同一个 DLL）
    HMODULE hLocalModule = GetModuleHandleW(moduleName.c_str());
    if (!hLocalModule) {
        return nullptr;
    }
    FARPROC pLocalFunc = GetProcAddress(hLocalModule, funcName);
    if (!pLocalFunc) return nullptr;

    // 3. 计算 RVA
    uintptr_t rva = (uintptr_t)pLocalFunc - (uintptr_t)hLocalModule;

    // 4. 远程函数地址 = 远程基址 + RVA
    return (FARPROC)((uintptr_t)hRemoteModule + rva);
}