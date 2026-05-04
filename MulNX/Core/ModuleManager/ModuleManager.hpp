#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>

#include <unordered_map>
#include <map>

class ModuleInfo {
public:
    std::vector<std::pair<std::string, MulNXHandle>> Info;
};

namespace MulNX {
    namespace Core {
        // 模块管理器类，负责加载、卸载和管理各个模块
        class ModuleManager final :public MulNX::ModuleBase {
        private:
            // 存储从字符串到模块句柄的映射，便于按名称查找
            std::unordered_map<std::string, MulNXHandle> NameToHandleMap{};
            std::map<MulNXHandle, std::unique_ptr<MulNX::ModuleBase>>modules{};
        public:
            bool Init()override;
            void ProcessMsg(MulNX::Message& Msg)override;

            // 注册模块，需要传入模块指针和名称，以及优先级，从1开始，数字越小优先级越高
            bool RegisteModule(std::unique_ptr<MulNX::ModuleBase>&& Module);
            // 记录模块，需要传入

            // 创建模块
            template<typename T>
                requires std::derived_from<T, ModuleBase>
            ModuleManager& CreateModule(std::string&& Name) {
                std::unique_ptr<T>Module = std::make_unique<T>();
                Module->SetName(std::move(Name));
                this->RegisteModule(std::move(Module));
                return *this;
            }
            ModuleManager& CreateSystemModules();
            // 根据名称获取模块指针
            MulNX::ModuleBase* FindModule(const std::string& Name);
            // 按类型查找模块
            template<typename T>
            T* FindModule(const std::string& Name) {
                return reinterpret_cast<T*>(this->FindModule(Name));
            }

            // 初始化最后部分使用
            bool ModulesBaseInit();
            bool ModulesInit();
        };
    }

}