#include "Core.hpp"
#include "Driver/Driver.hpp"
#include "ModuleManager/ModuleManager.hpp"
#include <MulNX/Systems/Systems.hpp>

MulNX::Core::Core::Core(std::string&& name) {
    this->originName = name;
    
    this->pDriver = std::make_unique<MulNX::Core::Driver>();
    this->pModuleManager = std::make_unique<MulNX::Core::ModuleManager>();

    this->preInits.push_back([this]() {
        this->LogSucc("核心就绪");
        this->LogInfo(MulNXVersion);
        return true;
        });

    this->postInits.clear();
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