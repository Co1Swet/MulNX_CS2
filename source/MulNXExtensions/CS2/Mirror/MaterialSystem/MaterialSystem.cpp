#include "MaterialSystem.hpp"

bool MaterialSystem::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/materialsystem2.dll", [this](MulNX::Message& msg) {
        this->materialsystem2 = MulNX::Memory::DllModule(L"materialsystem2.dll");
        this->pGameMaterialSystem = (void*)(this->materialsystem2.GetBaseAddress() +
            cs2_dumper::interfaces::materialsystem2_dll::VMaterialSystem2_001);
        });
    
    return true;
}

void* MaterialSystem::FindMaterial(CMaterial2*** out, const char* materialName) {
    using FindMaterial_t = void* (*)(void* This, CMaterial2*** out, const char* materialName);
    auto pFindMaterial = (FindMaterial_t)IVClass::Assume(this->pGameMaterialSystem)->GetVFuncPtr(14);
    return pFindMaterial(this->pGameMaterialSystem, out, materialName);
}