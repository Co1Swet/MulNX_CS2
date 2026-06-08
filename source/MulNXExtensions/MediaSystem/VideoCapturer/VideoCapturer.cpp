#include "VideoCapturer.hpp"
#include <MulNXExtensions/MediaSystem/VCD3D11Manager/VCD3D11Manager.hpp>

bool VideoCapturer::Init() {
    this->pVCD3D11Manager = this->GetCore()->ModuleManager()->FindModule<VCD3D11Manager>("VCD3D11Manager");
    this->ISys()
        .SendTask("Capture","Capture", [this]() -> bool {
            this->Captuer();
            return true; // 保持keep，反复轮询
        });

    return true;
}

void VideoCapturer::ReleaseStagingTexture() {
    if (this->pStagingTex) {
        this->pStagingTex->Release();
        this->pStagingTex = nullptr;
    }
    this->stagingWidth = 0;
    this->stagingHeight = 0;
    this->stagingFormat = DXGI_FORMAT_UNKNOWN;
}

void VideoCapturer::Reset() {
    std::unique_lock lock(this->smutex);
    this->srcPixelFormat = AV_PIX_FMT_NONE;
    this->ReleaseStagingTexture();
    this->recordStartTime.reset();
    this->runFlag1.store(false);
}

std::optional<av::VideoFrame> VideoCapturer::TryPop() {
    av::VideoFrame outFrame;
    if (this->buffer.try_dequeue(outFrame)) {
        return outFrame;
    }
    return std::nullopt;
}

void VideoCapturer::StartCapture(const std::chrono::steady_clock::time_point& startTime) {
    std::unique_lock lock(this->smutex);
    this->recordStartTime = startTime;
    this->runFlag1.store(true, std::memory_order_release);
}

void VideoCapturer::StopCapture() {
    std::unique_lock lock(this->smutex);
    this->runFlag1.store(false);
}

void VideoCapturer::ClearBuffer() {
    av::VideoFrame discard;
    while (this->buffer.try_dequeue(discard)) {
        // drain stale video data
    }
    std::unique_lock lock(this->smutex);
}

void VideoCapturer::Captuer() {
    this->Update();

    if (!this->runFlag1.load(std::memory_order_acquire)) {
        this->pVCD3D11Manager->runFlag1.store(false, std::memory_order_release);
        return;
    }
    this->pVCD3D11Manager->runFlag1.store(true, std::memory_order_release);
    if(!this->pVCD3D11Manager->buffer1.hasNewFrame.load(std::memory_order_acquire)){
        return;
    }

    std::unique_lock lock(this->smutex);

    D3D11_TEXTURE2D_DESC desc;
    this->pVCD3D11Manager->buffer1.shareTex.pTex->GetDesc(&desc);
    
    av::PixelFormat srcFormat = DXGIFormatToAvPixelFormat(desc.Format);
    if (srcFormat == AV_PIX_FMT_NONE) {
        this->ISys().LogError("当前后备缓冲区格式不受支持，无法录制");
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

        HRESULT hr = this->pVCD3D11Manager->pDevice->CreateTexture2D(&stagingDesc, nullptr, &this->pStagingTex);
        if (FAILED(hr)) {
            this->ISys().LogError("创建 D3D11 staging 纹理失败，录制中断");
            return;
        }

        this->stagingWidth = desc.Width;
        this->stagingHeight = desc.Height;
        this->stagingFormat = desc.Format;
        this->srcPixelFormat = srcFormat;
    }

    this->pVCD3D11Manager->buffer1.shareTex.pMutex->AcquireSync(1, INFINITE); // 等待新帧就绪（key = 1）
    if (desc.SampleDesc.Count > 1) {
        this->pVCD3D11Manager->pContext->ResolveSubresource(this->pStagingTex, 0, this->pVCD3D11Manager->buffer1.shareTex.pTex.Get(), 0, desc.Format);
    }
    else {
        this->pVCD3D11Manager->pContext->CopyResource(this->pStagingTex, this->pVCD3D11Manager->buffer1.shareTex.pTex.Get());
    }
    this->pVCD3D11Manager->buffer1.hasNewFrame.store(false);
    this->pVCD3D11Manager->buffer1.shareTex.pMutex->ReleaseSync(0); // 释放资源（key = 0）

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = this->pVCD3D11Manager->pContext->Map(this->pStagingTex, 0, D3D11_MAP_READ, 0, &mapped);
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
    this->pVCD3D11Manager->pContext->Unmap(this->pStagingTex, 0);

    av::VideoFrame srcFrame(rawData.data(), rawData.size(), srcFormat, this->stagingWidth, this->stagingHeight);
    if (this->recordStartTime.has_value()) {
        auto now = this->pVCD3D11Manager->buffer1.captureTime.load(std::memory_order_acquire);
        int64_t pts = std::chrono::duration_cast<std::chrono::microseconds>(now - *this->recordStartTime).count();
        srcFrame.setTimeBase({ 1, 1000000 });
        srcFrame.setPts(av::Timestamp(pts, srcFrame.timeBase()));
    }
    this->buffer.enqueue(std::move(srcFrame));

    return;
}