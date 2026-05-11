#pragma once

#include <MulNX/MulNX.hpp>

class DLLLoadDispatcher final : public MulNX::ModuleBase {
    std::set<std::filesystem::path> loadedModules; // 记录已加载的模块路径，避免重复记录
    void OnModuleLoaded(MulNX::Message& msg);
    void DispatchModuleLoadMessage(const std::filesystem::path& modulePath);
public:
    bool Init()override;
};