#pragma once
#include <Intro/Intros.hpp>

namespace CS2 {
    class CMaterial2 {
    public:
        virtual const char* GetName() = 0;
        virtual const char* GetShareName() = 0;
    };
}

class MaterialSystem final :public CSModule<MaterialSystem> {
    void* pGameMaterialSystem = nullptr;
    MulNX::Memory::DllModule materialsystem2{};
    bool Init()override;
public:
    void* FindMaterial(CS2::CMaterial2*** out, const char* materialName);
};