#include "CS2BootLoader.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Base/CharUtility/CharUtility.hpp>
#include <MulNXExtensions/CS2Remoting/CS2HelperController/CS2HelperController.hpp>
#include <commdlg.h>
#include <MulNXThirdParty/yaml.hpp>
#include <fstream>
#include <TlHelp32.h>
#include <nlohmann/json.hpp>
#include <charconv>

VOID CS2BootLoader::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow("CS2 Boot Loader");
    if (!w)return;
    uico->CallbackCall("CS2BootLoad"_hash, nullptr);
    if (!w.ShouldDraw())return;

    std::unique_lock lock(this->smutex);

    // 显示当前游戏路径
    ImGui::TextUnformatted(I18n("boot.game_path.show", gamePath.string()).c_str());

    if (this->environmentChecked) {
        if (ImGui::Button(I18n("boot.launch").c_str())) {
            this->PublishAsync(MulNX::Message::Create<bool>("CS2BootLoader/Launch"_hash, true).first);
        }
    }
    else {
        if (ImGui::Button(I18n("boot.check_env").c_str())) {
            LaunchResult result;
            this->environmentChecked = this->CheckEnvironment(result);
        }
    }


    if (ImGui::Button(I18n("boot.game_path.select").c_str())) {
        static wchar_t filePath[MAX_PATH] = {};

        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = GetActiveWindow();
        ofn.lpstrFilter = L"可执行文件\0*.exe\0所有文件\0*.*\0";
        ofn.lpstrFile = filePath;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        if (GetOpenFileNameW(&ofn)) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CS2BootLoader/PathUpdate"_hash);
            rp->str1 = MulNX::CharUtility::WToU8(filePath);
            this->PublishAsync(std::move(msg));
        }
    }

    ImGui::InputText(I18n("boot.launch_options").c_str(), &this->launchOptions);

    if (ImGui::Button(I18n("boot.save").c_str())) {
        this->PublishAsync("CS2BootLoader/Save"_hash);
    }
}

bool CS2BootLoader::Init() {
    this->pHelperController = this->Core->ModuleManager()->FindModule<CS2HelperController>("CS2HelperController");

    auto configPath = this->PathGet("Config");
    auto config = YAML::LoadFilePath(configPath / "config.yaml");
    this->gamePath = config["path"].as<std::string>();
    this->launchOptions = config["launchOptions"].as<std::string>();
    this->patternsCheckDangerous = config["patternsCheckDangerous"].as<std::vector<std::string>>();

    (*this)
        .SubscribeAsyncEx<void>("CS2BootLoader/Launch")
        .SubscribeAsyncEx<void>("CS2BootLoader/LaunchResult/Ack")
        .SubscribeAsync("CS2BootLoader/PathUpdate")
        .SubscribeAsync("CS2BootLoader/Save")
        ;

    auto autoCfg = YAML::LoadFilePath(configPath / "auto.yaml");
    auto autoLaunch = autoCfg["autoLaunch"].as<bool>();
#ifdef _DEBUG
    autoLaunch = true;
#endif
    if (autoLaunch) {
        this->PublishAsync(MulNX::Message::Create<bool>("CS2BootLoader/Launch"_hash, true).first);
    }
    
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
        // GUI / auto.yaml 使用内部标志；网络 factory 只能生成 NetExt。
        bool local = msg.asp.get<bool>() != nullptr;
        auto* pNetExt = msg.asp.get<MulNX::NetExt>();
        LaunchResult result;
        if (!local) {
            result.requestId = pNetExt ? pNetExt->str1 : "";
            uint64_t requestId = 0;
            auto& id = result.requestId;
            auto [end, ec] = std::from_chars(id.data(), id.data() + id.size(), requestId);
            if (ec != std::errc{} || end != id.data() + id.size() || requestId == 0 ||
                std::to_string(requestId) != id) {
                result.stage = "invalid_request";
                result.message = "requestId必须是非零uint64的十进制字符串";
                this->PostResult(result);
                break;
            }
            if (requestId == this->highestRequestId) {
                this->PostResult(this->lastResult);
                break;
            }
            if (requestId < this->highestRequestId) {
                result.stage = "stale_request";
                result.message = "requestId小于本启动器已接收的最高ID";
                this->PostResult(result);
                break;
            }
            this->highestRequestId = requestId;
        }
        try {
            result.ok = this->LaunchAndInject(result);
        }
        catch (const std::exception& e) {
            result.message = e.what();
            this->LogError(result.message);
        }
        catch (...) {
            result.message = "未知启动异常";
            this->LogError(result.message);
        }
        if (!result.ok && this->game.hProcess && !this->gameResumed) {
            result.cleanupPending = !this->CleanupGame(result.cleanupErrorCode);
        }
        if (!local) {
            this->lastResult = result;
            this->PostResult(result);
        }
        break;
    }
    case "CS2BootLoader/LaunchResult/Ack"_hash: {
        auto* pNetExt = msg.asp.get<MulNX::NetExt>();
        if (pNetExt && this->lastResult.ok && pNetExt->str1 == this->lastResult.requestId &&
            this->gameResumed && this->lastResult.pid == this->game.dwProcessId &&
            this->lastResult.created == this->gameCreated &&
            WaitForSingleObject(this->game.hProcess, 0) == WAIT_TIMEOUT) {
            this->PublishSync("Win32Starter/Close"_hash);
        }
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
        std::ofstream fout((configPath / "config.yaml").wstring());
        fout << config;
        fout.close();
        break;
    }
    default:break;
    }
}

bool CS2BootLoader::CheckEnvironment(LaunchResult& result) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        result.errorCode = GetLastError();
        result.message = std::format("无法创建进程快照. Error: {}", result.errorCode);
        this->LogError(result.message);
        return false;
    }
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(pe32);

    if (!Process32FirstW(hSnapshot, &pe32)) {
        result.errorCode = GetLastError();
        result.message = std::format("Process32First失败，Error: {}", result.errorCode);
        this->LogError(result.message);
        CloseHandle(hSnapshot);
        return false;
    }

    do {
        std::wstring ws(pe32.szExeFile);
        std::string exeNameUtf8 = MulNX::CharUtility::WToU8(ws);

        for (const auto& pattern : patternsCheckDangerous) {
            if (exeNameUtf8.find(pattern) != std::string::npos) {
                result.message = "为防止反作弊冲突，请先自行检验环境，关闭可能冲突的软件后再使用";
                this->LogError(result.message);
                CloseHandle(hSnapshot);
                return false;
            }
        }
    } while (Process32NextW(hSnapshot, &pe32));

    auto error = GetLastError();
    CloseHandle(hSnapshot);
    if (error != ERROR_NO_MORE_FILES) {
        result.errorCode = error;
        result.message = std::format("Process32Next失败，Error: {}", error);
        this->LogError(result.message);
        return false;
    }
    return true;
}

bool CS2BootLoader::LaunchAndInject(LaunchResult& result) {
    auto fail = [&](const std::string& message, DWORD errorCode = 0) {
        result.message = message;
        result.errorCode = errorCode;
        this->LogError(message);
        return false;
    };
    if (this->game.hProcess) {
        auto wait = WaitForSingleObject(this->game.hProcess, 0);
        auto error = wait == WAIT_FAILED ? GetLastError() : 0;
        if (wait != WAIT_OBJECT_0) {
            result.pid = this->game.dwProcessId;
            result.created = this->gameCreated;
            result.cleanupPending = !this->gameResumed || wait == WAIT_FAILED;
            result.stage = result.cleanupPending ? "cleanup_pending" : "already_running";
            return fail("上次游戏仍在运行或尚未确认清理", error);
        }
        this->CloseGame();
    }

    // 每次实际启动都重新检查，按钮检查不能代替此处检查。
    this->environmentChecked = this->CheckEnvironment(result);
    if (!this->environmentChecked)return false;
    result.stage = "launch";
    // 验证游戏路径
    if (gamePath.empty() || !fs::exists(gamePath)) {
        return fail(std::format("无效CS2路径: {}", gamePath.string()));
    }

    if (gamePath.filename() != "cs2.exe") {
        return fail(std::format("指定的游戏路径不指向cs2.exe: {}", gamePath.string()));
    }

    // 检查游戏是否已在运行
    HWND hwnd = FindWindowW(nullptr, L"Counter-Strike 2");
    if (hwnd != nullptr) {
        result.stage = "already_running";
        return fail(std::format("CS2 已在运行，请先关闭游戏: {}", gamePath.string()));
    }

    // 构建命令行参数
    std::wstring cmdLine = L"\"" + gamePath.wstring() + L"\"";
    cmdLine += L" -insecure ";// 强制insecure
    cmdLine += std::wstring(this->launchOptions.begin(), this->launchOptions.end());

    // 以 CREATE_SUSPENDED 方式创建游戏进程
    STARTUPINFOW si{ sizeof(si) };
    auto& pi = this->game;
    BOOL ok = CreateProcessW(
        nullptr,                // 命令行中已包含可执行文件路径
        cmdLine.data(),
        nullptr, nullptr, FALSE,
        CREATE_SUSPENDED,       // 挂起主线程
        nullptr, nullptr,
        &si, &pi
    );
    if (!ok) {
        auto error = GetLastError();
        return fail(std::format("创建CS2进程失败. Error: {}", error), error);
    }

    result.pid = pi.dwProcessId;
    result.stage = "identity";
    FILETIME creation{}, exit{}, kernel{}, user{};
    if (!GetProcessTimes(pi.hProcess, &creation, &exit, &kernel, &user)) {
        auto error = GetLastError();
        return fail("读取CS2创建时间失败", error);
    }
    this->gameCreated = std::to_string((static_cast<uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime);
    result.created = this->gameCreated;
    result.stage = "load";
    if (!this->pHelperController->Remoting(pi, result.message))return fail(result.message);

    // 恢复游戏主线程；只有挂起计数从1变为0才报告resumed。
    result.stage = "resume";
    auto suspended = ResumeThread(pi.hThread);
    if (suspended == static_cast<DWORD>(-1))return fail("恢复CS2主线程失败", GetLastError());
    if (suspended != 1)return fail("CS2主线程挂起计数异常");
    this->gameResumed = true;
    result.stage = "resumed";

    // 保留进程句柄用于去重和确认ACK，正常退出时不终止游戏。
    CloseHandle(pi.hThread);
    pi.hThread = nullptr;

    this->LogInfo(std::format("CS2启动并注入成功: {}", gamePath.string()));
    return true;
}

void CS2BootLoader::PostResult(const LaunchResult& result) {
    nlohmann::json payload = { {"protocol", 2}, {"requestId", result.requestId},
        {"ok", result.ok}, {"stage", result.stage}, {"pid", result.pid}, {"created", result.created},
        {"errorCode", result.errorCode}, {"message", result.message},
        {"cleanupPending", result.cleanupPending}, {"cleanupErrorCode", result.cleanupErrorCode} };
    nlohmann::json msg = { {"type", "CS2BootLoader/LaunchResult"},
        {"payload", payload.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace)} };
    this->WebPost(msg.dump());
}

void CS2BootLoader::CloseGame() {
    if (this->game.hThread)CloseHandle(this->game.hThread);
    if (this->game.hProcess)CloseHandle(this->game.hProcess);
    this->game = {};
    this->gameCreated.clear();
    this->gameResumed = false;
}

bool CS2BootLoader::CleanupGame(DWORD& errorCode) {
    if (WaitForSingleObject(this->game.hProcess, 0) != WAIT_OBJECT_0) {
        if (!TerminateProcess(this->game.hProcess, 1)) {
            errorCode = GetLastError();
            return false;
        }
        auto wait = WaitForSingleObject(this->game.hProcess, 5000);
        if (wait != WAIT_OBJECT_0) {
            errorCode = wait == WAIT_FAILED ? GetLastError() : WAIT_TIMEOUT;
            return false;
        }
    }
    this->CloseGame();
    return true;
}

CS2BootLoader::~CS2BootLoader() {
    this->CloseGame();
}
