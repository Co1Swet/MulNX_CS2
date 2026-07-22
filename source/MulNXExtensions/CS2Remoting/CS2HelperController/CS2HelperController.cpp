#include "CS2HelperController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2Remoting/DLLInjectHelper/DLLInjectHelper.hpp>

bool CS2HelperController::Init() {
    this->pInjectHelper = this->Core->ModuleManager()->FindModule<DLLInjectHelper>("DLLInjectHelper");

    auto rootPath = this->Path()->GetRoot();
    this->helperPath = rootPath / "CS2InternalHelper" / "CS2InternalHelper.dll";
    LoadLibraryW(this->helperPath.wstring().c_str());

    this->UIRegisterCallback("CS2BootLoad", [this](auto&&...) {
        MulNX::UI::Checkbox("当reshade被安装时，加载reshade", this->runFlag1);
        });
    this->runFlag1 = true;

    return true;
}

void CS2HelperController::DoInject(PROCESS_INFORMATION& pi,
    const std::filesystem::path& dllPath) {
    bool helperInjected = this->pInjectHelper->InjectDll(pi.hProcess, dllPath.wstring());
    if (!helperInjected) {
        TerminateProcess(pi.hProcess, 0);  // 注入失败则终止进程
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        MulNX::ErrorTerminate(std::format("Dll Load Failed : {}", dllPath.string()));
    }
}

bool CS2HelperController::Remoting(PROCESS_INFORMATION& pi) {
    // 注入 DLL
    this->DoInject(pi, this->helperPath);

    auto root = this->Path()->GetRoot();
    auto Tools = root / "Tools";
    auto dirFfmpeg = Tools / "ffmpeg";

    auto reshadedll = Tools / "dxgi.dll";
    if (std::filesystem::exists(reshadedll) && this->runFlag1) {
        this->DoInject(pi, reshadedll);
    }
    
    std::vector<std::string>ffmpegDllsName{
        "avutil-60.dll",
        "swresample-6.dll",
        "swscale-9.dll",
        "avcodec-62.dll",
        "avformat-62.dll",
        "avfilter-11.dll",
        "avdevice-62.dll"
    };

    for (const auto& ffmpegDll : ffmpegDllsName) {
        auto fullPath = dirFfmpeg / ffmpegDll;
        this->DoInject(pi, fullPath);
    }

    auto pthCS2OBTool = root / "CS2OBTool" / "CS2OBTool.dll";
    this->DoInject(pi, pthCS2OBTool);

    if (!this->pInjectHelper->InitDLL(pi.hProcess, L"CS2InternalHelper.dll", "MulNX_HelperStart")) {
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return false;
    }

    return true;
}