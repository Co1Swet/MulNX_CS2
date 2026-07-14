#pragma once

namespace CS2 {
    class CUtlSymbolLarge {
    public:
        char* pStr = nullptr;
    };

    template<typename T>
    struct C_UtlVectorEmbeddedNetworkVar {
        size_t m_nSize;
        T* m_pData;
        // 后面未知
    };


    enum class ui8TeamNum :uint8_t {
        T = 2,
        CT = 3
    };

    class CViewSetup {
    public:
        // 定位关键数据
        int* pWidth() { return reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0x434); }
        int* pHeight() { return reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0x43C); }

        float* pFov() { return reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + 0x498); }
        DirectX::XMFLOAT3* pViewOrigin() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + 0x4a0); }
        DirectX::XMFLOAT3* pViewAngles() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + 0x4b8); }
    };
}