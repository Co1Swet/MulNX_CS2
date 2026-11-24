#include "ModuleManager.hpp"
#include <MulNX/Common/Message.hpp>
#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/Systems.hpp>
#include <MulNX/Systems/WebSocketManager/WebSocketManager.hpp>
#include <MulNX/Systems/SysController/SysController.hpp>
#include <ranges>

bool MulNX::Core::ModuleManager::Init() {
    this->postDeinits.clear();

    (*this)
        .SubscribeAsync("ModuleManager/ModuleInfo/Request");

    this->SendTask("Update", "MulNXMain", [this]()->bool {
        this->Update();
        return true;
        });
    return true;
}
void MulNX::Core::ModuleManager::ProcessMsg(MulNX::Message& msg) {
    std::unique_lock lock(this->smutex);
    switch (msg.type) {
    case "ModuleManager/ModuleInfo/Request"_hash: {
        auto [response, rp] = MulNX::Message::Create<ModuleInfo>("ModuleManager/ModuleInfo/Response"_hash);
        for (const auto& [name, handle] : this->nameToHandleMap) {
            rp->Info.push_back(std::make_pair(name, handle));
        }
        this->PublishAsync(std::move(response));
    }
    }
}

bool MulNX::Core::ModuleManager::RegisteModule(std::unique_ptr<MulNX::IModule> pModule) {
    std::unique_lock lock(this->smutex);
    MulNXHandle hModule = MulNXHandle::CreateHandle();
    pModule->SetHandle(hModule);
    this->nameToHandleMap[pModule->GetNameView()] = hModule;
    this->modules[hModule] = std::move(pModule);
    return true;
}
MulNX::Core::ModuleManager& MulNX::Core::ModuleManager::CreateSystemModules() {
    (*this)
        .CreateModule<MulNX::PathManager>()// 路径管理器模块
        .CreateModule<MulNX::I18nManager>()
        .CreateModule<MulNX::VarReflSystem>()// 消息管理器模块
        .CreateModule<MulNX::MessageManager>()// 消息管理器模块
        .CreateModule<MulNX::UICoordinator>()
        .CreateModule<MulNX::UISystem>()// UI系统模块
        .CreateModule<MulNX::TaskSystem>()// 任务系统
        .CreateModule<MulNX::Logger>()
        .CreateModule<MulNX::Debugger>()// 调试器模块
        .CreateModule<MulNX::InputSystem>()// 输入系统模块
        .CreateModule<ShortcutManager>()// 快捷键管理器模块
        .CreateModule<MulNX::GlobalVars>()// 全局变量模块
        .CreateModule<WebSocketManager>()
        .CreateModule<SysController>()
        ;

    return *this;
}
MulNX::IModule* MulNX::Core::ModuleManager::FindModuleFromString(const std::string& name) {
    std::shared_lock lock(this->smutex);
    auto it = this->nameToHandleMap.find(name);
    if (it == this->nameToHandleMap.end()) {
        MulNX::ErrorTerminate(std::format("查找模块错误 : {}", name));
        return nullptr;
    }
    MulNXHandle HModule = it->second;
    auto it2 = this->modules.find(HModule);
    if (it2 == this->modules.end()) {
        MulNX::ErrorTerminate(std::format("查找模块错误 : {}", name));
        return nullptr;
    }
    return it2->second.get();
}
bool MulNX::Core::ModuleManager::BootstrapSystems() {
    std::shared_lock lock(this->smutex);
    this->LogInfo("正在执行系统引导");
    for (auto& [hModule, pModule] : this->modules) {
        if (!pModule->DrainBootstraps(this->Core)) {
            MulNX::ErrorTerminate(std::format("在模块引导时出现错误，模块名：{}", pModule->GetNameView()));
            return false;
        }
    }
    this->LogSucc("引导完毕");
    return true;
}
bool MulNX::Core::ModuleManager::ModulesInit() {
    std::shared_lock lock(this->smutex);
    for (auto& [hModule, pModule] : this->modules) {
        if (!pModule->EntryInit()) {
            MulNX::ErrorTerminate(std::format("在模块初始化时出现错误，模块名：{}", pModule->GetNameView()));
            return false;
        }
    }
    this->LogSucc(I18n("sys.inited_info", this->modules.size() + 3)); // 核心，驱动，自身
    return true;
}

void MulNX::Core::ModuleManager::DeinitModules() {
    std::unique_lock lock(this->smutex);
    for (auto& [hModule, pModule] : this->modules | std::views::reverse) {
        if (pModule->GetNameView() == "TaskSystem")continue;
        if (!pModule->EntryDeinit()) {
            MulNX::ErrorTerminate(std::format("在模块关闭时出现错误，模块名：{}", pModule->GetNameView()));
        }
    }
    this->LogSucc(I18n("sys.bye"));
}

bool MulNX::Core::ModuleManager::Deinit() {
    std::unique_lock lock(this->smutex);
    for (auto it = this->modules.rbegin();it != this->modules.rend();++it) {
        if (it->second->GetNameView() == "TaskSystem")continue;
        it->second = nullptr;
    }
    return true;
}