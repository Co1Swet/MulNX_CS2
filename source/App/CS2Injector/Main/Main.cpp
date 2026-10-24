#include <MulNXExtensions/Win32Starter/Win32Starter.hpp>
#include <MulNXExtensions/MulNXController/MulNXController.hpp>
#include <MulNXExtensions/CS2Remoting/CS2Remotings.hpp>
#include <App/CS2Injector/UIDocker/UIDocker.hpp>
#include <nlohmann/json.hpp>
#include <mutex>
#include <system_error>
#include <cstdlib>
#include <string_view>
#include <thread>
#include <atomic>

// Main code
int wmain(int argc, wchar_t** argv) {
    const bool external = argc > 1 && std::wstring_view(argv[1]) == L"--launch-result";
    HANDLE resultFile = INVALID_HANDLE_VALUE, game = nullptr;
    DWORD pid = 0;
    std::string created;
    std::atomic<const char*> stage = "initialize";
    HANDLE stopWatchdog = nullptr;
    std::thread watchdog;
    std::mutex resultMutex;
    DWORD cleanupCode = 0;
    auto cleanup = [&]() {
        if (!game)return true;
        auto wait = WaitForSingleObject(game, 0);
        if (wait == WAIT_OBJECT_0)return true;
        if (!TerminateProcess(game, 1)) { cleanupCode = GetLastError(); return false; }
        wait = WaitForSingleObject(game, 5000);
        if (wait != WAIT_OBJECT_0) { cleanupCode = wait == WAIT_FAILED ? GetLastError() : WAIT_TIMEOUT; return false; }
        return true;
    };
    auto report = [&](bool ok, DWORD code, const std::string& message) {
        std::lock_guard lock(resultMutex);
        bool cleaned = ok || cleanup();
        nlohmann::json result = { {"protocol", 1}, {"ok", ok}, {"stage", stage.load()},
            {"errorCode", code}, {"message", message}, {"cleanupPending", !cleaned} };
        if (pid)result["pid"] = pid;
        if (!created.empty())result["created"] = created;
        if (!cleaned)result["cleanupErrorCode"] = cleanupCode;
        auto text = result.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
        DWORD written = 0;
        bool saved = resultFile != INVALID_HANDLE_VALUE
            && WriteFile(resultFile, text.data(), static_cast<DWORD>(text.size()), &written, nullptr)
            && written == text.size() && FlushFileBuffers(resultFile);
        // 成功回执写入失败时，也要清理本次启动的游戏。
        if (!saved)cleanup();
        if (resultFile != INVALID_HANDLE_VALUE)CloseHandle(resultFile);
        resultFile = INVALID_HANDLE_VALUE;
        if (game)CloseHandle(game);
        game = nullptr;
        return ok && saved ? 0 : 1;
    };
    auto fail = [&](DWORD code, const std::string& message) {
        try { report(false, code, message); }
        catch (...) { std::lock_guard lock(resultMutex); cleanup(); }
        std::_Exit(1);
    };
    if (external) {
        if (argc != 3 || !fs::path(argv[2]).is_absolute())return 2;
        resultFile = CreateFileW(argv[2], GENERIC_WRITE, FILE_SHARE_READ, nullptr,
            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (resultFile == INVALID_HANDLE_VALUE)return 2;
        if (GetFileType(resultFile) != FILE_TYPE_DISK) { CloseHandle(resultFile); return 2; }
        // 仅非交互入口接管致命错误，普通启动和 auto.yaml 保持原处理。
        MulNX::errorTerminateHandler = [&](const std::string& message) { fail(0, message); };
        stopWatchdog = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!stopWatchdog) { report(false, GetLastError(), "无法准备启动超时保护"); return 1; }
    }
    try {
        if (external)watchdog = std::thread([&]() {
            if (WaitForSingleObject(stopWatchdog, 90000) == WAIT_TIMEOUT) {
                fail(WAIT_TIMEOUT, "启动超时，请按cleanupPending核对本次游戏收尾结果");
            }
        });
        // 创建核心
        auto core = MulNX::Core::Core::Create("CS2Injector");
        // 将模块句柄传递给核心，以便后续使用
        core->hMyOriginModule = GetModuleHandleW(NULL);
        // 注册所有模块
        (*core->ModuleManager())
            .CreateSystemModules()// 创建所有系统模块，这是框架运行的基础
            .CreateModule<Win32Starter>("Win32Starter")
            .CreateModule<DLLInjectHelper>("DLLInjectHelper")
            .CreateModule<CS2HelperController>("CS2HelperController")
            .CreateModule<CS2BootLoader>("CS2BootLoader")
            .CreateModule<MulNXController>("MulNXController")
            .CreateModule<UIDocker>("UIDocker")
            ;

        auto pStarter = core->ModuleManager()->FindModule<Win32Starter>("Win32Starter");

        if (external) {
            pStarter->quiet = true;
            auto pBoot = core->ModuleManager()->FindModule<CS2BootLoader>("CS2BootLoader");
            pBoot->externalLaunch = true;
            // 核心仍存活时处理异常，避免初始化未完成就析构仍有工作线程的核心。
            try {
                core->EntryInit(core.get());
                bool ok = pBoot->LaunchChecked(stage, [&](const PROCESS_INFORMATION& pi) {
                    std::lock_guard lock(resultMutex);
                    stage = "identity";
                    pid = pi.dwProcessId;
                    game = pi.hProcess;
                    HANDLE retained = nullptr;
                    if (!DuplicateHandle(GetCurrentProcess(), pi.hProcess, GetCurrentProcess(),
                        &retained, 0, FALSE, DUPLICATE_SAME_ACCESS))
                        throw std::system_error(GetLastError(), std::system_category(), "保留本次CS2句柄失败");
                    game = retained;
                    FILETIME creation{}, exit{}, kernel{}, user{};
                    if (!GetProcessTimes(game, &creation, &exit, &kernel, &user))
                        throw std::system_error(GetLastError(), std::system_category(), "读取CS2创建时间失败");
                    auto ticks = (static_cast<uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime;
                    created = std::to_string(ticks);
                    stage = "load";
                });
                if (ok)stage = "shutdown";
                PostQuitMessage(0);
                pStarter->Run();
                core->Driver()->WaitEnd();
                if (ok)stage = "resumed";
                SetEvent(stopWatchdog);
                watchdog.join();
                CloseHandle(stopWatchdog);
                auto code = report(ok, 0, ok ? "" : "启动未完成，详细原因见MulNX启动日志");
                MulNX::errorTerminateHandler = {};
                return code;
            } catch (const std::system_error& error) {
                fail(error.code().value(), error.what());
            } catch (const std::exception& error) {
                fail(0, error.what());
            } catch (...) {
                fail(0, "未知启动异常");
            }
        }

        // 启动核心
        core->EntryInit(core.get());
        // 启动主循环
        pStarter->Run();
        core->Driver()->WaitEnd();
        return 0;
    }
    catch (std::exception& e) {
        if (external)fail(0, e.what());
        MulNX::ErrorTerminate("在启动时发生异常！异常描述：" + std::string(e.what()));
    }
    catch (...) {
        if (external)fail(0, "未知启动异常");
        MulNX::ErrorTerminate("在启动时发生未知异常！");
    }
    return 0;
}