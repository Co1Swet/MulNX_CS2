#pragma once
#include <MulNX/Core/Module/Module.hpp>
#include <map>

class ModuleInfo {
public:
    std::vector<std::pair<std::string_view, MulNXHandle>> Info;
};

namespace MulNX::Core {
    class ModuleManager final :public MulNX::Module<ModuleManager> {
        std::unordered_map<std::string_view, MulNXHandle> nameToHandleMap{};
        std::map<MulNXHandle, std::unique_ptr<MulNX::IModule>> modules{};

        bool Init()override;
        bool Deinit()override;
        void ProcessMsg(MulNX::Message& msg)override;
        bool RegisteModule(std::unique_ptr<MulNX::IModule> module);
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
        MulNX::IModule* FindModuleFromString(const std::string& name);
        // 按类型查找模块
        template<typename T>
        T* FindModuleFromString(const std::string& name) {
            return static_cast<T*>(this->FindModuleFromString(name));
        }

        // 初始化最后部分使用
        bool BootstrapSystems();
        bool ModulesInit();
        void DeinitModules();
    };
}