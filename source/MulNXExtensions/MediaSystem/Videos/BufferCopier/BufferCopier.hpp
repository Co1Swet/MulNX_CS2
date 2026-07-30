#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class BufferCopier final :public MediaModuleBase {
    class VCD3D11Manager* pVCD3D11Manager = nullptr;
    class MediaParamManager* pMediaParamManager = nullptr;
    MulNX::GraphicsManager* pGraphicsManager = nullptr;

    bool Init()override;
    void CopyTexture();
public:
    std::atomic<bool> shouldCopy = false;
};