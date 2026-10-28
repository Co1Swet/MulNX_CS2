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
    if (!fs::exists(ffmpegListPath)) {
        MulNX::ErrorTerminate(std::format("ffmpeg list.yaml not found: {}", ffmpegListPath.string()));
    }
    auto ffmpegList = YAML::LoadFilePath(ffmpegListPath);
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

bool CS2HelperController::DoInject(PROCESS_INFORMATION& pi,
    const fs::path& dllPath, std::string& error) {
    if (!this->pInjectHelper->InjectDll(pi.hProcess, dllPath.wstring())) {
        error = std::format("Dll Load Failed : {}", dllPath.string());
        return false;
    }
    return true;
}

bool CS2HelperController::Remoting(PROCESS_INFORMATION& pi, std::string& error) {
    // Reshade注入
    auto reshadedll = this->dirTools / "reshade" / "dxgi.dll";
    if (fs::exists(reshadedll) && this->injectReshade) {
        if (!this->DoInject(pi, reshadedll, error))return false;
    }
    // ffmpeg注入
    for (const auto& ffmpegDll : this->ffmpegsDll) {
        auto fullPath = this->dirFFmpeg / ffmpegDll;
        // 在自己进程加载Dll，这里千万不要删掉，不然算不出地址，没办法远程初始化了
        if (!LoadLibraryW(fullPath.wstring().c_str())) {
            auto code = GetLastError();
            error = std::format("本地加载DLL失败: {}, Error: {}", fullPath.string(), code);
            return false;
        }
        if (!this->DoInject(pi, fullPath, error))return false;
    }
    // CS2OBTool注入，同样先在自己进程加载Dll
    if (!LoadLibraryW(this->CS2OBToolPath.wstring().c_str())) {
        auto code = GetLastError();
        error = std::format("本地加载DLL失败: {}, Error: {}", this->CS2OBToolPath.string(), code);
        return false;
    }
    if (!this->DoInject(pi, this->CS2OBToolPath, error))return false;
    // 远程初始化CS2OBTool.dll
    if (!this->pInjectHelper->InitDLL(pi.hProcess, L"CS2OBTool.dll", "MulNX_CS2_Start")) {
        error = "CS2OBTool远程初始化调用失败，详细原因见日志";
        return false;
    }
    return true;
}