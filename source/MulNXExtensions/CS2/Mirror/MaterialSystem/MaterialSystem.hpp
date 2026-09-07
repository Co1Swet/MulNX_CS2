#pragma once
#include <Intro/CSModuleBase.hpp>

class MaterialSystem final :public CSModuleBase {
    void* pGameMaterialSystem = nullptr;
    MulNX::Memory::DllModule materialsystem2{};
    bool Init()override;
public:
    void* FindMaterial(CMaterial2*** out, const char* materialName);
};