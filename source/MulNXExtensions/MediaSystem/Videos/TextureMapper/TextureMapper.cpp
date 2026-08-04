#include "TextureMapper.hpp"

bool TextureMapper::Init() {
    this->pVCD3D11Manager = this->FindModule<VCD3D11Manager>("VCD3D11Manager");

    this->SubscribeSync("MediaSync/Reset", [this](MulNX::Message& msg) {
        this->nextFrameRelease = true;
        });

    return true;
}

void TextureMapper::ReleaseStagingTexture() {
    this->readbackBuf.clear();
    this->readbackBuf.shrink_to_fit();
    this->srcPixelFormat = AV_PIX_FMT_NONE;
    if (this->pStagingTex) {
        this->pStagingTex->Release();
        this->pStagingTex = nullptr;
    }
    this->stagingWidth = 0;
    this->stagingHeight = 0;
    this->stagingFormat = DXGI_FORMAT_UNKNOWN;
}

bool TextureMapper::CheckStagingTexture(D3D11_TEXTURE2D_DESC& desc) {
    if (this->nextFrameRelease) {
        this->ReleaseStagingTexture();
        this->nextFrameRelease = false;
    }

    av::PixelFormat srcFmt = this->pVCD3D11Manager->srcAVFormat;
    if (srcFmt == AV_PIX_FMT_NONE) return false;

    if (!this->pStagingTex || this->stagingWidth != (int)desc.Width ||
        this->stagingHeight != (int)desc.Height || this->stagingFormat != desc.Format) {
        this->ReleaseStagingTexture();

        D3D11_TEXTURE2D_DESC sd = {};
        sd.Width = desc.Width;
        sd.Height = desc.Height;
        sd.MipLevels = sd.ArraySize = 1;
        sd.Format = desc.Format;
        sd.SampleDesc.Count = 1;
        sd.Usage = D3D11_USAGE_STAGING;
        sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        if (FAILED(this->pVCD3D11Manager->pReadSideDevice->CreateTexture2D(&sd, nullptr, &this->pStagingTex)))
            return false;

        this->stagingWidth = (int)desc.Width;
        this->stagingHeight = (int)desc.Height;
        this->stagingFormat = desc.Format;
        this->srcPixelFormat = srcFmt;
    }

    return true;
}

std::optional<av::VideoFrame> TextureMapper::MapFrame(MidTex& tex) {
    D3D11_TEXTURE2D_DESC desc;
    tex.pTex->GetDesc(&desc);

    if (!this->CheckStagingTexture(desc))return std::nullopt;

    tex.pMutex->AcquireSync(1, INFINITE);
    if (desc.SampleDesc.Count > 1)
        this->pVCD3D11Manager->pReadSideContext->ResolveSubresource(this->pStagingTex, 0, tex.pTex.Get(), 0, desc.Format);
    else
        this->pVCD3D11Manager->pReadSideContext->CopyResource(this->pStagingTex, tex.pTex.Get());
    tex.pMutex->ReleaseSync(0);

    D3D11_MAPPED_SUBRESOURCE map = {};
    if (FAILED(this->pVCD3D11Manager->pReadSideContext->Map(this->pStagingTex, 0, D3D11_MAP_READ, 0, &map)) || !map.pData)
        return std::nullopt;
    size_t rowBytes = (size_t)this->stagingWidth * 4;
    size_t total = rowBytes * (size_t)this->stagingHeight;
    if (this->readbackBuf.size() < total) this->readbackBuf.resize(total);
    for (UINT r = 0; r < (UINT)this->stagingHeight; ++r)
        memcpy(this->readbackBuf.data() + rowBytes * r,
            (const uint8_t*)map.pData + (size_t)map.RowPitch * r, rowBytes);
    this->pVCD3D11Manager->pReadSideContext->Unmap(this->pStagingTex, 0);

    av::VideoFrame frame(this->readbackBuf.data(), total, this->srcPixelFormat,
        this->stagingWidth, this->stagingHeight);

    return frame;
}