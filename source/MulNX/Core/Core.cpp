#include "Core.hpp"
#include "Driver/Driver.hpp"
#include "ModuleManager/ModuleManager.hpp"
#include <MulNX/Systems/Systems.hpp>

MulNX::Core::Core::Core(std::string&& name) :
    createTime(std::chrono::steady_clock::now()),
    CoreName(name) {
    // 设置启动器
    this->pDriver = std::make_unique<MulNX::Core::Driver>();
    this->pDriver->SetName("Driver");
    // 创建模块管理器
    this->pModuleManager = std::make_unique<MulNX::Core::ModuleManager>();
    this->pModuleManager->SetName("ModuleManager");
}
MulNX::Core::Core* MulNX::Core::Core::Create(std::string&& coreName) {
    auto core = std::make_unique<MulNX::Core::Core>(std::move(coreName));
    auto& Myself = core->pMyself;
    core->pMyself = std::move(core);
    return Myself.get();
}
MulNX::Core::Driver* MulNX::Core::Core::Driver() {
    return this->pDriver.get();
}
MulNX::Core::ModuleManager* MulNX::Core::Core::ModuleManager() {
    return this->pModuleManager.get();
}
// 专用初始化函数
void MulNX::Core::Core::Init() {
    // 通过核心启动器进行系统初始化
    this->pDriver->EntryInit(this);
    return;
}
void MulNX::Core::Core::Close() {
    this->pMyself = nullptr;
    return;
}
std::string MulNX::Core::Core::GetName() {
    return this->CoreName;
}