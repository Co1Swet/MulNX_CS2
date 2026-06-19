#include "IModule.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>

MulNX::IModule* MulNX::IModule::FindModule(const std::string& name) {
    return this->Core->ModuleManager()->FindModule(name);
}