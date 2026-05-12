#include "CS2BootLoader.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/WinExt/Remote/Remote.hpp>
#include <commdlg.h>
#include <yaml-cpp/yaml.h>
#include <fstream>

bool CS2BootLoader::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("CS2 Boot Loader", this->showWindow);
    if (!w)return true;
    std::unique_lock lock(this->smutex);

    // 显示当前游戏路径
    ImGui::Text(I18n("boot.game_path.show", gamePath.string()).c_str());

    if (ImGui::Button(I18n("boot.launch").c_str())) {
        this->ISys().PublishAsync("CS2BootLoader/Launch"_hash);
    }

    if (ImGui::Button(I18n("boot.game_path.select").c_str())) {
        static char filePath[MAX_PATH] = {};

        OPENFILENAMEA ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = GetActiveWindow();
        ofn.lpstrFilter = "可执行文件\0*.exe\0所有文件\0*.*\0";
        ofn.lpstrFile = filePath;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        if (GetOpenFileNameA(&ofn)) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CS2BootLoader/PathUpdate"_hash);
            rp->str1 = filePath;
            this->ISys().PublishAsync(std::move(msg));
        }
    }

    ImGui::InputText(I18n("boot.launch_options").c_str(), &this->launchOptions);

    if (ImGui::Button(I18n("boot.save").c_str())) {
        this->ISys().PublishAsync("CS2BootLoader/Save"_hash);
    }

    return true;
}

bool CS2BootLoader::Init() {
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});
    auto configPath = this->ISys().PathGet("Config");
    auto config = YAML::LoadFile((configPath / "config.yaml").string());
    this->gamePath = config["path"].as<std::string>();
    this->launchOptions= config["launchOptions"].as<std::string>();

    auto pIPCer = this->Core->ModuleManager()->FindModule<MulNX::IPCer>("IPCer");
    auto rootPath = pIPCer->GetRoot();
    this->dllPath = rootPath / "CS2OBTool" / "CS2OBTool.dll";
    LoadLibraryW(this->dllPath.wstring().c_str());

    if (!std::filesystem::exists(this->dllPath)) {
        this->ISys().LogError(std::format("CS2OBTool.dll not found at expected path: {}", this->dllPath.string()));
    }

    this->showWindow = true;

    this->ISys()
        .SubscribeAsync("CS2BootLoader/Launch")
        .SubscribeAsync("CS2BootLoader/PathUpdate")
        .SubscribeAsync("CS2BootLoader/Save")
        ;

    this->SendTask("MulNXMain", [this]()->bool {
        this->Update();
        return true;
        });

    return true;
}

void CS2BootLoader::ProcessMsg(MulNX::Message& msg) {
    std::unique_lock lock(this->smutex);
    switch (msg.type) {
    case "CS2BootLoader/Launch"_hash: {
        this->LaunchAndInject();
        break;
    }
    case "CS2BootLoader/PathUpdate"_hash: {
        auto* pNetExt = msg.asp.get<MulNX::NetExt>();
        this->gamePath = pNetExt->str1;
        break;
    }
    case "CS2BootLoader/Save"_hash: {
        auto configPath = this->ISys().PathGet("Config");
        YAML::Node config;
        config["path"] = this->gamePath.string();
        config["launchOptions"] = this->launchOptions;
        std::ofstream fout(configPath / "config.yaml");
        fout << config;
        fout.close();
        break;
    }
    default:break;
    }
}

bool CS2BootLoader::LaunchAndInject() {
    // 验证游戏路径
    if (gamePath.empty() || !std::filesystem::exists(gamePath)) {
        this->ISys().LogError(std::format("Invalid CS2 path: {}", gamePath.string()));
        return false;
    }

    if (gamePath.filename() != "cs2.exe") {
        this->ISys().LogError(std::format("The specified game path does not point to cs2.exe: {}", gamePath.string()));
        return false;
    }

    // 检查游戏是否已在运行
    if (IsGameRunning()) {
        this->ISys().LogError(std::format("CS2 is already running. Please close it first."));
        return false;
    }

    // 构建命令行参数
    std::wstring cmdLine = L"\"" + gamePath.wstring() + L"\"";
    cmdLine += L" -insecure ";// 强制insecure
    cmdLine += std::wstring(this->launchOptions.begin(), this->launchOptions.end());
    

    // 以 CREATE_SUSPENDED 方式创建游戏进程
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

    // 注入 DLL
    bool injected = InjectDll(pi.hProcess);
    if (!injected) {
        TerminateProcess(pi.hProcess, 0);  // 注入失败则终止进程
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return false;
    }

    // 调用初始化
    if (!InitDLL(pi.hProcess)) {
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return false;
    }

    // 恢复游戏主线程
    ResumeThread(pi.hThread);

    // 清理句柄
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    this->ISys().LogInfo(std::format("CS2 launched and injected successfully."));
    return true;
}

bool CS2BootLoader::IsGameRunning() {
    HWND hwnd = FindWindowW(nullptr, L"Counter-Strike 2");
    return hwnd != nullptr;
}

bool CS2BootLoader::InjectDll(HANDLE hProcess) {
    auto path = this->dllPath.wstring();
    // 在目标进程分配内存并写入 DLL 路径
    size_t pathSize = (path.size() + 1) * sizeof(wchar_t);
    LPVOID pRemoteMem = VirtualAllocEx(hProcess, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemoteMem) return false;

    if (!WriteProcessMemory(hProcess, pRemoteMem, path.c_str(), pathSize, nullptr)) {
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

bool CS2BootLoader::InitDLL(HANDLE hProcess) {
    // 获取远程初始化函数地址
    FARPROC pRemoteInit = GetRemoteProcAddress(hProcess, L"CS2OBTool.dll", "MulNX_CS2_Start");
    if (!pRemoteInit) {
        this->ISys().LogError("Failed to locate MulNX_CS2_Start in remote process");
        return false;
    }

    // 创建远程线程执行初始化
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
        (LPTHREAD_START_ROUTINE)pRemoteInit, nullptr, 0, nullptr);
    if (!hThread) {
        this->ISys().LogError("Failed to create remote init thread");
        return false;
    }

    // 等待初始化完成
    WaitForSingleObject(hThread, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    CloseHandle(hThread);

    // 检查返回码：你的 MulNX_CS2_Start 成功时返回 0
    if (exitCode != 0) {
        this->ISys().LogError(std::format("Remote initialization failed with code: {}", exitCode));
        return false;
    }

    this->ISys().LogInfo("Remote initialization completed successfully");
    return true;
}

void CS2BootLoader::Deinit() {

    return;
}
