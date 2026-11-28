#pragma once
#include <Intro/Intros.hpp>
#include <Mirror/MaterialSystem/MaterialSystem.hpp>
#include <Game/CSUtils/CBufferString.hpp>

struct GetMaterialsArrayResult {
    uint64_t count;
    CS2::CMaterial2*** pArrMaterials; // 指向材质指针数组的指针（2）
    uint64_t unk;
};

struct CBufferStringForSky {
    CS2::CBufferString buf;
    uint8_t pad[0xE0 - sizeof(CS2::CBufferString)];
};

class ResourceSystem final :public CSModule<ResourceSystem> {
    MulNX::Memory::DllModule resourcesystem{};
    void* pGameResourcesystem = nullptr;
    bool Init()override;
public:
    void GetMaterials(uint64_t magic, GetMaterialsArrayResult* out, uint8_t unk);
    CS2::CMaterial2** PreCache(CBufferStringForSky* name, const char* unk);
    // 这是封装的
    CS2::CMaterial2** PreCache(const char* name);
};