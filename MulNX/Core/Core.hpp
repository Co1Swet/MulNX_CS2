#pragma once

#include <MulNX/Config/Config.hpp>
#include <Memory>
#include <chrono>

namespace MulNX {
    namespace Core {
        class Core {
            friend class MulNX::Core::CoreStarterBase;
        private:
            // 自身指针
            std::unique_ptr<Core> pMyself = nullptr;
            // 核心名称（文件系统路径管理需要用）
            std::string CoreName;
            // 模块管理器指针
            std::unique_ptr<ModuleManager> pModuleManager;
			// 核心启动器指针
            std::unique_ptr<MulNX::Core::CoreStarterBase> pCoreStarter = nullptr;
            // 核心创建时间
            std::chrono::steady_clock::time_point createTime;

            Core() = delete;
            Core(const Core&) = delete;
            Core& operator=(const Core&) = delete;
        public:
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
            ModuleManager* ModuleManager();

            // 设置启动器
            template<typename T>
            T* CreateCoreStarter() {
                this->pCoreStarter = std::make_unique<T>();
                return static_cast<T*>(this->pCoreStarter.get());
            }

            // 获取核心名
            std::string GetName();
        };
    }
}