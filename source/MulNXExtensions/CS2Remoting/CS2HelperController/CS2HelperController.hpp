#pragma once
#include <MulNX/MulNX.hpp>
#include <filesystem>

class CS2HelperController final :public MulNX::Module<CS2HelperController> {
    class DLLInjectHelper* pInjectHelper = nullptr;
    std::filesystem::path CS2OBToolPath;
    std::atomic<bool> injectReshade = true;
    bool Init()override;
    void DoInject(PROCESS_INFORMATION& pi, const std::filesystem::path& dllPath);
public:
    bool Remoting(PROCESS_INFORMATION& pi);
};