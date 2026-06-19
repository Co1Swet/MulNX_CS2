#include "ModuleManager.hpp"
#include <MulNX/Common/Message.hpp>
#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/Systems.hpp>

bool MulNX::Core::ModuleManager::Init() {
    (*this)
        .SubscribeAsync("ModuleManager/ModuleInfo/Request");

    this->SendTask("Update", "MulNXMain", [this]()->bool {
        this->Update();
        return true;
        });
    return true;
}
void MulNX::Core::ModuleManager::ProcessMsg(MulNX::Message& Msg) {
    std::unique_lock lock(this->smutex);
    switch (Msg.type) {
    case "ModuleManager/ModuleInfo/Request"_hash: {
        auto [pInfo, raw] = MulNX::make_any_shared<ModuleInfo>();
        for (const auto& [Name, Handle] : this->NameToHandleMap) {
            raw->Info.push_back(std::make_pair(Name, Handle));
        }
        MulNX::Message msg("ModuleManager/ModuleInfo/Response"_hash);
        msg.asp = std::move(pInfo);
        this->PublishAsync(std::move(msg));
    }
    }
}

bool MulNX::Core::ModuleManager::RegisteModule(std::unique_ptr<MulNX::ModuleBase>&& Module) {
    std::unique_lock lock(this->smutex);
    MulNXHandle hModule = MulNXHandle::CreateHandle();
    Module->HModule = hModule;
    this->NameToHandleMap[Module->GetName()] = hModule;
    this->modules[hModule] = std::move(Module);
    return true;
}
MulNX::Core::ModuleManager& MulNX::Core::ModuleManager::CreateSystemModules() {
    (*this)
        .CreateModule<MulNX::IPCer>("IPCer")// IPC模块
        .CreateModule<MulNX::PathManager>("PathManager")// 路径管理器模块
        .CreateModule<MulNX::CrashDumper>("CrashDumper")
        .CreateModule<MulNX::I18nManager>("I18nManager")
        .CreateModule<MulNX::MessageManager>("MessageManager")// 消息管理器模块
        .CreateModule<MulNX::UICoordinator>("UICoordinator")
        .CreateModule<MulNX::UISystem>("UISystem")// UI系统模块
        .CreateModule<MulNX::TaskSystem>("TaskSystem")// 任务系统
        .CreateModule<MulNX::Logger>("Logger")
        .CreateModule<MulNX::Debugger>("Debugger")// 调试器模块
        .CreateModule<MulNX::HandleSystem>("HandleSystem")// 句柄系统模块
        .CreateModule<MulNX::InputSystem>("InputSystem")// 输入系统模块
        .CreateModule<ShortcutManager>("ShortcutManager")// 快捷键管理器模块
        .CreateModule<MulNX::GlobalVars>("GlobalVars")// 全局变量模块
        ;

    return *this;
}
MulNX::ModuleBase* MulNX::Core::ModuleManager::FindModule(const std::string& Name) {
    std::shared_lock lock(this->smutex);
    auto it = this->NameToHandleMap.find(Name);
    if (it == this->NameToHandleMap.end()) {
        MulNX::ErrorTerminate("查找模块错误 0x1");
        return nullptr;
    }
    MulNXHandle HModule = it->second;
    auto it2 = this->modules.find(HModule);
    if (it2 == this->modules.end()) {
        MulNX::ErrorTerminate("查找模块错误 0x2");
        return nullptr;
    }
    return it2->second.get();
}

bool MulNX::Core::ModuleManager::ModulesInit() {
    std::shared_lock lock(this->smutex);
    // 通过有序的初始化任务列表进行初始化，尽管Modules是无序的
    for (auto& [hModule, pModule] : this->modules) {
        if (!pModule->EntryInit(this->Core)) {
            MulNX::ErrorTerminate("在模块初始化时出现错误，模块名：" + pModule->GetName());
            return false;
        }
    }
    this->LogSucc(I18n("sys.inited_info", this->modules.size() + 2)); // 模块管理器自身和核心启动器
    return true;
}

void MulNX::Core::ModuleManager::DeinitModules() {
    std::unique_lock lock(this->smutex);
    for (auto it = this->modules.rbegin();it != this->modules.rend();++it) {
        if (it->second->GetName() == "TaskSystem")continue;
        it->second->Deinit();
        this->LogInfo(I18n("module.deinited", it->second->GetName()));
    }
    this->LogSucc(I18n("sys.bye"));
}

void MulNX::Core::ModuleManager::Deinit() {
    std::unique_lock lock(this->smutex);
    for (auto it = this->modules.rbegin();it != --this->modules.rend();++it) {
        if (it->second->GetName() == "TaskSystem")continue;
        it->second = nullptr;
    }
}