#include "ResourceSystem.hpp"

bool ResourceSystem::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/resourcesystem.dll", [this](MulNX::Message& msg) {
        this->resourcesystem = MulNX::Memory::DllModule(L"resourcesystem.dll");
        this->ppGameResourcesystem = (void**)(this->resourcesystem.GetBaseAddress() + cs2_dumper::interfaces::resourcesystem_dll::ResourceSystem013);
        if (*this->ppGameResourcesystem) {
            this->LogError("找到");
        }
        });

    return true;
}