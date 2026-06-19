#pragma once

#include <MulNX/Config/Config.hpp>
#include <Memory>
#include <chrono>
#include <Windows.h>

namespace MulNX {
    namespace Core {
        class Driver;
        class ModuleManager;
        class Core {
            friend class MulNX::Core::Driver;
        private:
            // 自身指针
            std::unique_ptr<Core> pMyself = nullptr;
            // 核心名称（文件系统路径管理需要用）
            std::string CoreName;
            // 模块管理器指针
            std::unique_ptr<ModuleManager> pModuleManager;
			// 核心启动器指针
            std::unique_ptr<MulNX::Core::Driver> pDriver = nullptr;
            // 核心创建时间
            std::chrono::steady_clock::time_point createTime;

            Core() = delete;
            Core(const Core&) = delete;
            Core& operator=(const Core&) = delete;
        public:
            HMODULE hMyOriginModule = nullptr;   // 用于获取自身 DLL/EXE 的文件路径
            // 构造函数
            Core(std::string&& CoreName);
            // 析构函数
            ~Core() = default;
            
            // 只允许被调用一次
            static Core* Create(std::string&& CoreName);

            // 初始化
            void Init();
            void Close();

            // 获取模块的接口
            MulNX::Core::Driver* Driver();
            ModuleManager* ModuleManager();

            // 获取核心名
            std::string GetName();
        };
    }
}