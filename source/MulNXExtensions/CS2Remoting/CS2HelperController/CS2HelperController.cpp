#include "CS2HelperController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2Remoting/DLLInjectHelper/DLLInjectHelper.hpp>

bool CS2HelperController::Init() {
    this->pInjectHelper = this->Core->ModuleManager()->FindModule<DLLInjectHelper>("DLLInjectHelper");

    auto dirRoot = this->Path()->GetRoot();
    this->CS2OBToolPath = dirRoot / "CS2OBTool" / "CS2OBTool.dll";
    this->dirTools = dirRoot / "Tools";
    this->dirFFmpeg = this->dirTools / "ffmpeg";

    auto ffmpegListPath = this->dirFFmpeg / "list.yaml";
    if (!std::filesystem::exists(ffmpegListPath)) {
        MulNX::ErrorTerminate(std::format("ffmpeg list.yaml not found: {}", ffmpegListPath.string()));
    }
    auto ffmpegList = YAML::LoadFile(ffmpegListPath.string());
    if (!ffmpegList["DLLs"]) {
        MulNX::ErrorTerminate(std::format("ffmpeg list.yaml missing 'DLLs' key: {}", ffmpegListPath.string()));
    }
    for (const auto& dllNode : ffmpegList["DLLs"]) {
        if (!dllNode.IsScalar()) {
            MulNX::ErrorTerminate(std::format("ffmpeg list.yaml contains non-scalar DLL entry: {}", ffmpegListPath.string()));
        }
        std::string dllName = dllNode.as<std::string>();
        this->ffmpegsDll.push_back(std::move(dllName));
    }

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
    // Reshade注入
    auto reshadedll = this->dirTools / "dxgi.dll";
    if (std::filesystem::exists(reshadedll) && this->injectReshade) {
        this->DoInject(pi, reshadedll);
    }
    // ffmpeg注入
    for (const auto& ffmpegDll : this->ffmpegsDll) {
        auto fullPath = this->dirFFmpeg / ffmpegDll;
        // 在自己进程加载Dll，这里千万不要删掉，不然算不出地址，没办法远程初始化了
        LoadLibraryW(fullPath.c_str());
        this->DoInject(pi, fullPath);
    }
    // CS2OBTool注入，同样先在自己进程加载Dll
    LoadLibraryW(this->CS2OBToolPath.wstring().c_str());
    this->DoInject(pi, this->CS2OBToolPath);
    // 远程初始化CS2OBTool.dll
    if (!this->pInjectHelper->InitDLL(pi.hProcess, L"CS2OBTool.dll", "MulNX_CS2_Start")) {
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return false;
    }
    return true;
}