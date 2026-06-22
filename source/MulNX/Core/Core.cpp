#include "Core.hpp"
#include "Driver/Driver.hpp"
#include "ModuleManager/ModuleManager.hpp"
#include <MulNX/Systems/Systems.hpp>

MulNX::Core::Core::Core(std::string&& name) :
    createTime(std::chrono::steady_clock::now()) {
    this->ModuleName = std::move(name);
    // 设置启动器
    this->pDriver = std::make_unique<MulNX::Core::Driver>();
    this->pDriver->SetName("Driver");
    // 创建模块管理器
    this->pModuleManager = std::make_unique<MulNX::Core::ModuleManager>();
    this->pModuleManager->SetName("ModuleManager");
    this->backInits->clear();
    this->delayInits->push_back([this]() {
        this->LogSucc("核心就绪");
        return true;
        });
}
std::unique_ptr<MulNX::Core::Core> MulNX::Core::Core::Create(std::string&& coreName) {
    auto core = std::make_unique<MulNX::Core::Core>(std::move(coreName));
    return core;
}
MulNX::Core::Driver* MulNX::Core::Core::Driver() {
    return this->pDriver.get();
}
MulNX::Core::ModuleManager* MulNX::Core::Core::ModuleManager() {
    return this->pModuleManager.get();
}
// 专用初始化函数
bool MulNX::Core::Core::Init() {
    this->pDriver->EntryInit(this);
    return true;
}