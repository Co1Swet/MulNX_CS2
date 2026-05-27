#include "FrameCapturer.hpp"

bool FrameCapturer::Init() {
    this->pGraphicsManager = this->Core->ModuleManager()->FindModule<MulNX::GraphicsManager>("GraphicsManager");

    return true;
}

void FrameCapturer::ReleaseStagingTexture() {
    if (this->pStagingTex) {
        this->pStagingTex->Release();
        this->pStagingTex = nullptr;
    }
    this->stagingWidth = 0;
    this->stagingHeight = 0;
    this->stagingFormat = DXGI_FORMAT_UNKNOWN;
}

void FrameCapturer::Reset() {
    this->srcPixelFormat = AV_PIX_FMT_NONE;
    this->ReleaseStagingTexture();
}

std::optional<av::VideoFrame> FrameCapturer::TryPop() {
    av::VideoFrame outFrame;
    if (this->buffer.try_dequeue(outFrame)) {
        return std::move(outFrame);
    }
    return std::nullopt;
}

void FrameCapturer::CheckCaptuer() {
    this->Update();

    static std::optional<std::chrono::steady_clock::time_point> lastCapture;
    auto now = std::chrono::steady_clock::now();
    constexpr std::chrono::duration<double> minInterval(1.0 / 60.0);

    if (lastCapture.has_value() && (now - *lastCapture < minInterval)) {
        return;
    }
    lastCapture = now;

    ID3D11Texture2D* backBuffer = nullptr;
    this->pGraphicsManager->pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (!backBuffer) {
        return;
    }

    D3D11_TEXTURE2D_DESC desc;
    backBuffer->GetDesc(&desc);

    av::PixelFormat srcFormat = DXGIFormatToAvPixelFormat(desc.Format);
    if (srcFormat == AV_PIX_FMT_NONE) {
        this->ISys().LogError("当前后备缓冲区格式不受支持，无法录制");
        backBuffer->Release();
        return;
    }

    if (!this->pStagingTex || this->stagingWidth != desc.Width || this->stagingHeight != desc.Height || this->stagingFormat != desc.Format) {
        this->ReleaseStagingTexture();

        D3D11_TEXTURE2D_DESC stagingDesc = {};
        stagingDesc.Width = desc.Width;
        stagingDesc.Height = desc.Height;
        stagingDesc.MipLevels = 1;
        stagingDesc.ArraySize = 1;
        stagingDesc.Format = desc.Format;
        stagingDesc.SampleDesc.Count = 1;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        stagingDesc.BindFlags = 0;

        HRESULT hr = this->pGraphicsManager->pd3dDevice->CreateTexture2D(&stagingDesc, nullptr, &this->pStagingTex);
        if (FAILED(hr)) {
            this->ISys().LogError("创建 D3D11 staging 纹理失败，录制中断");
            backBuffer->Release();
            return;
        }

        this->stagingWidth = desc.Width;
        this->stagingHeight = desc.Height;
        this->stagingFormat = desc.Format;
        this->srcPixelFormat = srcFormat;
    }

    // Copy back buffer into staging texture for CPU readback.
    if (desc.SampleDesc.Count > 1) {
        this->pGraphicsManager->pd3dContext->ResolveSubresource(this->pStagingTex, 0, backBuffer, 0, desc.Format);
    }
    else {
        this->pGraphicsManager->pd3dContext->CopyResource(this->pStagingTex, backBuffer);
    }
    backBuffer->Release();

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = this->pGraphicsManager->pd3dContext->Map(this->pStagingTex, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr) || !mapped.pData) {
        this->ISys().LogError("Map D3D11 staging 纹理失败，录制帧跳过");
        return;
    }

    size_t rowBytes = static_cast<size_t>(this->stagingWidth) * 4;
    std::vector<uint8_t> rawData(rowBytes * this->stagingHeight);
    uint8_t* dstRow = rawData.data();
    for (UINT row = 0; row < static_cast<UINT>(this->stagingHeight); ++row) {
        const uint8_t* srcRow = reinterpret_cast<const uint8_t*>(mapped.pData) + static_cast<size_t>(mapped.RowPitch) * row;
        memcpy(dstRow, srcRow, rowBytes);
        dstRow += rowBytes;
    }
    this->pGraphicsManager->pd3dContext->Unmap(this->pStagingTex, 0);

    av::VideoFrame srcFrame(rawData.data(), rawData.size(), srcFormat, this->stagingWidth, this->stagingHeight);
    this->buffer.enqueue(std::move(srcFrame));

    return;
}