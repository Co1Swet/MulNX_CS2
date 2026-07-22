#include "DLLInjectHelper.hpp"
#include <MulNXUtils/WinExt/Remote/Remote.hpp>

bool DLLInjectHelper::Init() {

    return true;
}

bool DLLInjectHelper::InjectDll(HANDLE hProcess, const std::wstring& dllPath) {
    // 在目标进程分配内存并写入 DLL 路径
    size_t pathSize = (dllPath.size() + 1) * sizeof(wchar_t);
    LPVOID pRemoteMem = VirtualAllocEx(hProcess, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemoteMem) return false;

    if (!WriteProcessMemory(hProcess, pRemoteMem, dllPath.c_str(), pathSize, nullptr)) {
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        return false;
    }

    // 获取 LoadLibraryW 地址
    auto pLoadLibrary = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
    if (!pLoadLibrary) {
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        return false;
    }

    // 远程线程执行 LoadLibraryW
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
        (LPTHREAD_START_ROUTINE)pLoadLibrary, pRemoteMem, 0, nullptr);
    if (!hThread) {
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        return false;
    }

    // 等待注入完成
    WaitForSingleObject(hThread, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);

    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);

    return exitCode != 0;  // LoadLibrary 返回非零表示成功
}

bool DLLInjectHelper::InitDLL(HANDLE hProcess, const std::wstring& dllName, const char* initFuncName) {
    // 获取远程初始化函数地址
    FARPROC pRemoteInit = GetRemoteProcAddress(hProcess, dllName, initFuncName);
    if (!pRemoteInit) {
        this->LogError("Failed to locate MulNX_CS2_Start in remote process");
        return false;
    }

    // 创建远程线程执行初始化
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
        (LPTHREAD_START_ROUTINE)pRemoteInit, nullptr, 0, nullptr);
    if (!hThread) {
        this->LogError("Failed to create remote init thread");
        return false;
    }

    // 等待初始化完成
    WaitForSingleObject(hThread, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    CloseHandle(hThread);

    // 检查返回码：你的 MulNX_CS2_Start 成功时返回 0
    if (exitCode != 0) {
        this->LogError(std::format("Remote initialization failed with code: {}", exitCode));
        return false;
    }

    this->LogInfo("Remote initialization completed successfully");
    return true;
}