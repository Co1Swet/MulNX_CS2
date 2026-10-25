#pragma once
#include <MulNX/MulNX.hpp>
#include <MulNX/Base/fs.hpp>

class CS2HelperController final :public MulNX::Module<CS2HelperController> {
    class DLLInjectHelper* pInjectHelper = nullptr;
    fs::path CS2OBToolPath;
    fs::path dirTools;
    fs::path dirFFmpeg;
    std::vector<std::string> ffmpegsDll{};
    MulReflBool(injectReshade, true);
    bool Init()override;
    void DoInject(PROCESS_INFORMATION& pi, const fs::path& dllPath);
public:
    bool Remoting(PROCESS_INFORMATION& pi);
};