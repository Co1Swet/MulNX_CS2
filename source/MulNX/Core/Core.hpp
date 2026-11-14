#pragma once
#include <MulNX/Core/Module/Module.hpp>
#include <Memory>
#include <chrono>
#include <Windows.h>

namespace MulNX {
    namespace Core {
        class Driver;
        class ModuleManager;
        class Core final :public Module<Core> {
            friend class MulNX::Core::Driver;

            std::unique_ptr<ModuleManager> pModuleManager = nullptr;
            std::unique_ptr<MulNX::Core::Driver> pDriver = nullptr;
            std::string originName{};
            std::chrono::steady_clock::time_point createTime = std::chrono::steady_clock::now();

            Core() = delete;
            Core(const Core&) = delete;
            Core& operator=(const Core&) = delete;
            // 初始化
            bool Init()override;
        public:
            HMODULE hMyOriginModule = nullptr;   // 用于获取自身 DLL/EXE 的文件路径
            Core(std::string&& name);
            // 只允许被调用一次
            static std::unique_ptr<Core> Create(std::string&& coreName);
            MulNX::Core::Driver* Driver();
            ModuleManager* ModuleManager();
            inline const std::string& GetOriginName()const noexcept { return this->originName; }
        };
    }
}