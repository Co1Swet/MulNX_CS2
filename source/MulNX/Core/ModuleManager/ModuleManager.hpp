#pragma once
#include <MulNX/Core/Module/Module.hpp>
#include <map>

class ModuleInfo {
public:
    std::vector<std::pair<std::string_view, MulNXHandle>> Info;
};

namespace MulNX {
    namespace Core {
        // 模块管理器类，负责加载、卸载和管理各个模块
        class ModuleManager final :public MulNX::Module<ModuleManager> {
            // 存储从字符串到模块句柄的映射，便于按名称查找
            std::unordered_map<std::string_view, MulNXHandle> NameToHandleMap{};
            std::map<MulNXHandle, std::unique_ptr<MulNX::ModuleBase>>modules{};

            bool Init()override;
            bool Deinit()override;
            void ProcessMsg(MulNX::Message& Msg)override;
            // 注册模块，需要传入模块指针和名称
            bool RegisteModule(std::unique_ptr<MulNX::ModuleBase>&& Module);
        public:
            // 创建模块
            template<MulNX::IsModule T>
            ModuleManager& CreateModule() {
                std::unique_ptr<T> module = std::make_unique<T>();
                this->RegisteModule(std::move(module));
                return *this;
            }
            ModuleManager& CreateSystemModules();
            // 根据名称获取模块指针
            MulNX::ModuleBase* FindModuleFromString(const std::string& name);
            // 按类型查找模块
            template<typename T>
            T* FindModuleFromString(const std::string& name) {
                return static_cast<T*>(this->FindModuleFromString(name));
            }

            // 初始化最后部分使用
            bool ModulesInit();
            void DeinitModules();
        };
    }
}