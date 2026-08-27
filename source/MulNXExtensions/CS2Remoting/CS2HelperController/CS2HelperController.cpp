#include "CS2HelperController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2Remoting/DLLInjectHelper/DLLInjectHelper.hpp>

bool CS2HelperController::Init() {
    this->pInjectHelper = this->Core->ModuleManager()->FindModule<DLLInjectHelper>("DLLInjectHelper");

    auto rootPath = this->Path()->GetRoot();
    this->CS2OBToolPath = rootPath / "CS2OBTool" / "CS2OBTool.dll";

    this->UIRegisterCallback("CS2BootLoad", [this](auto&&...) {
        MulNX::UI::Checkbox("当reshade被安装时，加载reshade", this->injectReshade);
        });

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
    auto root = this->Path()->GetRoot();
    auto Tools = root / "Tools";
    auto dirFfmpeg = Tools / "ffmpeg";

    auto reshadedll = Tools / "dxgi.dll";
    if (std::filesystem::exists(reshadedll) && this->injectReshade) {
        this->DoInject(pi, reshadedll);
    }
    
    std::vector<std::string>ffmpegDllsName{
        "avutil-61.dll",
        "swresample-7.dll",
        "swscale-10.dll",
        "avcodec-63.dll",
        "avformat-63.dll",
        "avfilter-12.dll",
        "avdevice-63.dll"
    };

    for (const auto& ffmpegDll : ffmpegDllsName) {
        auto fullPath = dirFfmpeg / ffmpegDll;
        LoadLibraryW(fullPath.c_str());
        this->DoInject(pi, fullPath);
    }

    LoadLibraryW(this->CS2OBToolPath.wstring().c_str());
    this->DoInject(pi, this->CS2OBToolPath);

    if (!this->pInjectHelper->InitDLL(pi.hProcess, L"CS2OBTool.dll", "MulNX_CS2_Start")) {
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return false;
    }

    return true;
}