#pragma once
#include <MulNX/MulNX.hpp>
#include <MulNXUtils/WinExt/Memory/Memory.hpp>
#include <unordered_set>

class DLLLoadDispatcher final : public MulNX::Module<DLLLoadDispatcher> {
    // LoadLibrary 函数钩子（用于DLL注入检测）
    std::unique_ptr<MulNX::Hook> hkLoadLibraryExW = nullptr;
    std::unordered_set<std::string> targets{};// 要拦截的模块
    std::set<std::filesystem::path> loadedModules{}; // 记录已加载的模块路径，避免重复记录
    void OnModuleLoaded(MulNX::Message& msg);
    void DispatchModuleLoadMessage(const std::filesystem::path& modulePath);
public:
    bool Init()override;
    void Deinit()override;
};