#include "MaterialSystem.hpp"

using FindMaterial_t = void** (*)(void* pThis, const char* name);

bool MaterialSystem::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/materialsystem2.dll", [this](MulNX::Message& msg) {
        this->materialsystem2 = MulNX::Memory::DllModule(L"materialsystem2.dll");
        this->ppGameMaterialSystem = (void**)(this->materialsystem2.GetBaseAddress() + cs2_dumper::interfaces::materialsystem2_dll::VMaterialSystem2_001);
        });
    
    return true;
}

void** MaterialSystem::FindMaterial(const std::string& name) {
    auto pFindMaterial = (FindMaterial_t)IVClass::Assume(*this->ppGameMaterialSystem)->GetVFuncPtr(13);
    return pFindMaterial(*this->ppGameMaterialSystem, name.c_str());
}