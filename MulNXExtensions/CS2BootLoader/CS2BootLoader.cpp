#include "CS2BootLoader.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <yaml-cpp/yaml.h>

bool CS2BootLoader::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("CS2 Boot Loader", this->showWindow);
    if (!w)return true;

    // 显示当前游戏路径
    ImGui::Text(std::format("Game Path: {}", gamePath.string()).c_str());

    if (ImGui::Button("Launch CS2")) {
        if (LaunchAndInject()) {
            // 启动成功后可以关闭窗口或显示状态
            this->showWindow = false;
        }
    }

    return true;
}

bool CS2BootLoader::Init() {
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});
    auto configPath = this->ISys().PathGet("Config");
    auto config = YAML::LoadFile((configPath / "config.yaml").string());
    this->gamePath = config["path"].as<std::string>();

    for (const auto& option : config["launchOptions"]) {
        this->launchOptions.push_back(option.as<std::string>());
    }

    this->showWindow = true;
    return true;
}

bool CS2BootLoader::LaunchAndInject() {
    // 1. 验证游戏路径
    if (gamePath.empty() || !std::filesystem::exists(gamePath)) {
        this->ISys().LogError(std::format("Invalid CS2 path: {}", gamePath.string()));
        return false;
    }

    // 2. 检查游戏是否已在运行
    if (IsGameRunning()) {
        this->ISys().LogError(std::format("CS2 is already running. Please close it first."));
        return false;
    }

    // 3. 确定 DLL 路径（假定与注入器同目录）
    // std::filesystem::path dllFullPath = this->ISys().PathGet(".") / "CS2OBTool.dll";
    // if (!std::filesystem::exists(dllFullPath)) {
    //     this->ISys().LogError(std::format("CS2OBTool.dll not found: {}", dllFullPath.string()));
    //     return false;
    // }

    // 4. 构建命令行参数
    std::wstring cmdLine = L"\"" + gamePath.wstring() + L"\"";
    for (const auto& option : this->launchOptions) {
        cmdLine += L" " + std::wstring(option.begin(), option.end());
    }

    // 5. 以 CREATE_SUSPENDED 方式创建游戏进程
    STARTUPINFOW si{ sizeof(si) };
    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessW(
        nullptr,                // 命令行中已包含可执行文件路径
        cmdLine.data(),
        nullptr, nullptr, FALSE,
        CREATE_SUSPENDED,       // 关键：挂起主线程
        nullptr, nullptr,
        &si, &pi
    );
    if (!ok) {
        this->ISys().LogError(std::format("Failed to create CS2 process. Error: {}", GetLastError()));
        return false;
    }

    // 6. 注入 DLL
    // bool injected = InjectDll(pi.hProcess, dllFullPath.wstring());
    // if (!injected) {
    //     TerminateProcess(pi.hProcess, 0);  // 注入失败则终止进程
    //     CloseHandle(pi.hThread);
    //     CloseHandle(pi.hProcess);
    //     return false;
    // }

    // 7. 恢复游戏主线程
    ResumeThread(pi.hThread);

    // 8. 清理句柄
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    this->ISys().LogInfo(std::format("CS2 launched and injected successfully."));
    return true;
}

bool CS2BootLoader::IsGameRunning() {
    HWND hwnd = FindWindowW(nullptr, L"Counter-Strike 2");
    return hwnd != nullptr;
}

bool CS2BootLoader::InjectDll(HANDLE hProcess, const std::wstring& dllPath) {
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

void CS2BootLoader::Deinit() {

    return;
}
