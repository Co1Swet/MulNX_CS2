#include "TextureMapper.hpp"
#include <cstring>
#include <VPipeline/VCD3D11Manager/VCD3D11Manager.hpp>

bool TextureMapper::Init() {
    this->pVCD3D11Manager = this->FindModule<VCD3D11Manager>();

    this->SubscribeSync("MediaSync/SetOn", [this](MulNX::Message& msg) {
        this->nextFrameRelease = true;
        });

    return true;
}

void TextureMapper::ReleaseStagingTexture() {
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

    if (this->pStagingTex && this->stagingWidth == (int)desc.Width &&
        this->stagingHeight == (int)desc.Height && this->stagingFormat == desc.Format) {
        return true;
    }

    this->ReleaseStagingTexture();

    D3D11_TEXTURE2D_DESC sd = {};
    sd.Width = desc.Width;
    sd.Height = desc.Height;
    sd.MipLevels = sd.ArraySize = 1;
    sd.Format = desc.Format;
    sd.SampleDesc.Count = 1;
    sd.Usage = D3D11_USAGE_STAGING;
    sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    if (FAILED(this->pVCD3D11Manager->pCaptureD3D11Device->CreateTexture2D(&sd, nullptr, &this->pStagingTex)))
        return false;

    this->stagingWidth = (int)desc.Width;
    this->stagingHeight = (int)desc.Height;
    this->stagingFormat = desc.Format;
    this->srcPixelFormat = srcFmt;

    return true;
}

std::optional<av::VideoFrame> TextureMapper::MapFrame(VCD3D11Manager::CaptureTextureView& view) {
    D3D11_TEXTURE2D_DESC desc;
    view.pTex->GetDesc(&desc);

    if (!this->CheckStagingTexture(desc))return std::nullopt;
    {
        view.pMutex->AcquireSync(1, INFINITE);
        auto rel = scope_exit([&]() {
            view.pMutex->ReleaseSync(0);
            });
        if (desc.SampleDesc.Count > 1)
            this->pVCD3D11Manager->pCaptureD3D11DeviceContext->ResolveSubresource(this->pStagingTex, 0, view.pTex.Get(), 0, desc.Format);
        else
            this->pVCD3D11Manager->pCaptureD3D11DeviceContext->CopyResource(this->pStagingTex, view.pTex.Get());
    }
    D3D11_MAPPED_SUBRESOURCE map = {};
    if (FAILED(this->pVCD3D11Manager->pCaptureD3D11DeviceContext->Map(this->pStagingTex, 0, D3D11_MAP_READ, 0, &map)))
        return std::nullopt;
    auto unmap = scope_exit([&]() {
        this->pVCD3D11Manager->pCaptureD3D11DeviceContext->Unmap(this->pStagingTex, 0);
        });
    
    if (!map.pData)return std::nullopt;
    size_t rowBytes = (size_t)this->stagingWidth * 4;
    const auto* src = static_cast<const uint8_t*>(map.pData);

    av::VideoFrame frame(this->srcPixelFormat, this->stagingWidth, this->stagingHeight);
    auto* dst = frame.raw();
    for (int y = 0; y < this->stagingHeight; ++y) {
        std::memcpy(
            dst->data[0] + static_cast<size_t>(y) * dst->linesize[0],
            src + static_cast<size_t>(y) * map.RowPitch,
            rowBytes);
    }
    return frame;
}