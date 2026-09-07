#include "ResourceSystem.hpp"

// 64-bit hash for 32-bit platforms
// Credit
// https://www.ncbi.nlm.nih.gov/IEB/ToolBox/CPP_DOC/lxr/source/src/util/checksum/murmurhash/MurmurHash2.cxx#0140
uint64_t MurmurHash64B(const void* key, int len, uint64_t seed) {
    const uint32_t m = 0x5bd1e995;
    const int r = 24;

    uint32_t h1 = uint32_t(seed) ^ len;
    uint32_t h2 = uint32_t(seed >> 32);

    const uint32_t* data = (const uint32_t*)key;

    while (len >= 8) {
        uint32_t k1 = *data++;
        k1 *= m; k1 ^= k1 >> r; k1 *= m;
        h1 *= m; h1 ^= k1;
        len -= 4;

        uint32_t k2 = *data++;
        k2 *= m; k2 ^= k2 >> r; k2 *= m;
        h2 *= m; h2 ^= k2;
        len -= 4;
    }

    if (len >= 4) {
        uint32_t k1 = *data++;
        k1 *= m; k1 ^= k1 >> r; k1 *= m;
        h1 *= m; h1 ^= k1;
        len -= 4;
    }

    switch (len) {
    case 3: h2 ^= ((unsigned char*)data)[2] << 16;
    case 2: h2 ^= ((unsigned char*)data)[1] << 8;
    case 1: h2 ^= ((unsigned char*)data)[0];
        h2 *= m;
    };

    h1 ^= h2 >> 18; h1 *= m;
    h2 ^= h1 >> 22; h2 *= m;
    h1 ^= h2 >> 17; h1 *= m;
    h2 ^= h1 >> 19; h2 *= m;

    uint64_t h = h1;

    h = (h << 32) | h2;

    return h;
}

bool ResourceSystem::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/resourcesystem.dll", [this](MulNX::Message& msg) {
        this->resourcesystem = MulNX::Memory::DllModule(L"resourcesystem.dll");
        this->pGameResourcesystem = (void*)(this->resourcesystem.GetBaseAddress() +
            cs2_dumper::interfaces::resourcesystem_dll::ResourceSystem013);
        if (!this->pGameResourcesystem)MulNX::ErrorTerminate("找不到游戏资源系统");
        this->LogSucc("查找到游戏资源系统");
        });

    return true;
}

void ResourceSystem::GetMaterials(uint64_t magic, GetMaterialsArrayResult* out, uint8_t unk) {
    using GetMaterials_t = void(*)(void* This, uint64_t magic, GetMaterialsArrayResult* out, uint8_t unk);
    auto pGetMaterials_t = (GetMaterials_t)IVClass::Assume(this->pGameResourcesystem)->GetVFuncPtr(32);
    return pGetMaterials_t(this->pGameResourcesystem, magic, out, unk);
}
CMaterial2** ResourceSystem::PreCache(CBufferStringForSky* name, const char* unk) {
    using PreCacheFn_t = CMaterial2 * *(*)(void* pThis, CBufferStringForSky* name, const char* unk);
    auto pPreCache = (PreCacheFn_t)IVClass::Assume(this->pGameResourcesystem)->GetVFuncPtr(40);
    return pPreCache(this->pGameResourcesystem, name, unk);
}
CMaterial2** ResourceSystem::PreCache(const char* name) {

    CBufferStringForSky wrapper{ name };
    wrapper.buf.FixupPathName(0x5C); // back slash
    wrapper.buf.ToLowerFast(0);
    wrapper.buf.FixSlashes(0x2F); // forward slash

    CS2::CBufferString extension;
    extension.ExtractFileExtension(wrapper.buf.c_str());

    auto hash = MurmurHash64B(wrapper.buf.c_str(), wrapper.buf.Length(), 0xEDABCDEF);

    auto pWrapper = (unsigned char*)&wrapper;
    memcpy(pWrapper + 0xD0, &hash, 8);
    memcpy(pWrapper + 0xD8, extension.c_str(), 8);

    return this->PreCache(&wrapper, "");
}