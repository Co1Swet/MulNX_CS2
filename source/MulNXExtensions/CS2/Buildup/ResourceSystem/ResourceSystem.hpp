#pragma once
#include <Intro/CSModuleBase.hpp>

struct GetMaterialsArrayResult {
    uint64_t count;
    CMaterial2*** pArrMaterials; // 指向材质指针数组的指针（2）
    uint64_t unk;
};

struct CBufferStringForSky {
    CS2::CBufferString buf;
    uint8_t pad[0xE0 - sizeof(CS2::CBufferString)];
};

class ResourceSystem final :public CSModuleBase {
    MulNX::Memory::DllModule resourcesystem{};
    void** ppGameResourcesystem = nullptr;
    bool Init()override;
public:
    void GetMaterials(uint64_t magic, GetMaterialsArrayResult* out, uint8_t unk);
    CMaterial2** PreCache(CBufferStringForSky* name, const char* unk);
    // 这是封装的
    CMaterial2** PreCache(const char* name);
};