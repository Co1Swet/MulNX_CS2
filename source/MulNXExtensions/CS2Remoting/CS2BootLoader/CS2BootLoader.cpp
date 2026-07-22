#include "CS2BootLoader.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Base/CharUtility/CharUtility.hpp>
#include <MulNXExtensions/CS2Remoting/CS2HelperController/CS2HelperController.hpp>
#include <commdlg.h>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <TlHelp32.h>

bool CS2BootLoader::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow("CS2 Boot Loader");
    if (!w)return true;
    uico->CallbackCall("CS2BootLoad"_hash, nullptr);

    std::unique_lock lock(this->smutex);

    // 显示当前游戏路径
    ImGui::Text(I18n("boot.game_path.show", gamePath.string()).c_str());

    if (this->environmentChecked) {
        if (ImGui::Button(I18n("boot.launch").c_str())) {
            this->PublishAsync("CS2BootLoader/Launch"_hash);
        }
    }
    else {
        if (ImGui::Button(I18n("boot.check_env").c_str())) {
            this->environmentChecked = this->CheckEnvironment();
        }
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
            this->PublishAsync(std::move(msg));
        }
    }

    ImGui::InputText(I18n("boot.launch_options").c_str(), &this->launchOptions);

    if (ImGui::Button(I18n("boot.save").c_str())) {
        this->PublishAsync("CS2BootLoader/Save"_hash);
    }

    return true;
}

bool CS2BootLoader::Init() {
    this->pHelperController = this->Core->ModuleManager()->FindModule<CS2HelperController>("CS2HelperController");

    auto configPath = this->PathGet("Config");
    auto config = YAML::LoadFile((configPath / "config.yaml").string());
    this->gamePath = config["path"].as<std::string>();
    this->launchOptions = config["launchOptions"].as<std::string>();
    this->patternsCheckDangerous = config["patternsCheckDangerous"].as<std::vector<std::string>>();

    this->showWindow = true;

    (*this)
        .SubscribeAsync("CS2BootLoader/Launch")
        .SubscribeAsync("CS2BootLoader/PathUpdate")
        .SubscribeAsync("CS2BootLoader/Save")
        ;

    this->SendTask("Update", "MulNXMain", [this]()->bool {
        this->Update();
        return true;
        });

    this->SendUIRoot(this->GetName(), [this](auto uico, auto&&...) {return this->Window(uico);});

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
        auto configPath = this->PathGet("Config");
        YAML::Node config;
        config["path"] = this->gamePath.string();
        config["launchOptions"] = this->launchOptions;
        config["patternsCheckDangerous"] = this->patternsCheckDangerous;
        std::ofstream fout(configPath / "config.yaml");
        fout << config;
        fout.close();
        break;
    }
    default:break;
    }
}

bool CS2BootLoader::CheckEnvironment() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        this->LogError(std::format("无法创建进程快照. Error: {}", GetLastError()));
        return false;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(pe32);

    if (!Process32FirstW(hSnapshot, &pe32)) {
        this->LogError(std::format("Process32First failed. Error: {}", GetLastError()));
        CloseHandle(hSnapshot);
        return false;
    }

    do {
        std::wstring ws(pe32.szExeFile);
        std::string exeNameUtf8 = MulNX::CharUtility::WToU8(ws);

        for (const auto& pattern : patternsCheckDangerous) {
            if (exeNameUtf8.find(pattern) != std::string::npos) {
                this->LogError("为防止反作弊冲突，请先自行检验环境");
                CloseHandle(hSnapshot);
                return false;
            }
        }
    } while (Process32NextW(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return true;
}

bool CS2BootLoader::LaunchAndInject() {
    // 验证游戏路径
    if (gamePath.empty() || !std::filesystem::exists(gamePath)) {
        this->LogError(std::format("Invalid CS2 path: {}", gamePath.string()));
        return false;
    }

    if (gamePath.filename() != "cs2.exe") {
        this->LogError(std::format("The specified game path does not point to cs2.exe: {}", gamePath.string()));
        return false;
    }

    // 检查游戏是否已在运行
    if (IsGameRunning()) {
        this->LogError(std::format("CS2 is already running. Please close it first."));
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
        this->LogError(std::format("Failed to create CS2 process. Error: {}", GetLastError()));
        return false;
    }

    if (!this->pHelperController->Remoting(pi))return false;

    // 恢复游戏主线程
    ResumeThread(pi.hThread);

    // 清理句柄
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    this->LogInfo(std::format("CS2 launched and injected successfully."));
    return true;
}

bool CS2BootLoader::IsGameRunning() {
    HWND hwnd = FindWindowW(nullptr, L"Counter-Strike 2");
    return hwnd != nullptr;
}

void CS2BootLoader::Deinit() {

    return;
}