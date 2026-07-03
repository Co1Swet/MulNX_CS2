#include "VideoCapturer.hpp"
#include <MulNXExtensions/MediaSystem/VCD3D11Manager/VCD3D11Manager.hpp>
#include <MulNXExtensions/MediaSystem/VEncodeHelper/VEncodeHelper.hpp>

bool VideoCapturer::Init() {
    this->pVCD3D11Manager = this->FindModule<VCD3D11Manager>("VCD3D11Manager");
    this->pVEncodeHelper  = this->FindModule<VEncodeHelper>("VEncodeHelper");
    this->SendTask("Capture", "Capture", [this]() -> bool { this->Captuer(); return true; });
    return true;
}

void VideoCapturer::ReleaseStagingTexture() {
    if (this->pStagingTex) { this->pStagingTex->Release(); this->pStagingTex = nullptr; }
    this->stagingWidth = this->stagingHeight = 0;
    this->stagingFormat = DXGI_FORMAT_UNKNOWN;
}

void VideoCapturer::Reset() {
    std::unique_lock lock(this->smutex);
    this->srcPixelFormat = AV_PIX_FMT_NONE;
    this->ReleaseStagingTexture();
    this->readbackBuf.clear();
    this->readbackBuf.shrink_to_fit();
    this->recordStartTime.reset();
    this->hwCapture = false;
    this->runFlag1.store(false);
}

void VideoCapturer::StartCapture(const std::chrono::steady_clock::time_point& startTime, bool hwPath) {
    std::unique_lock lock(this->smutex);
    this->recordStartTime = startTime;
    this->hwCapture = hwPath;
    this->runFlag1.store(true, std::memory_order_release);
}

void VideoCapturer::StopCapture() {
    std::unique_lock lock(this->smutex);
    this->runFlag1.store(false);
}

void VideoCapturer::ClearBuffer() {
    av::VideoFrame discard;
    while (this->buffer.try_dequeue(discard));
}

std::optional<av::VideoFrame> VideoCapturer::TryPop() {
    av::VideoFrame f;
    return this->buffer.try_dequeue(f) ? std::optional(std::move(f)) : std::nullopt;
}

void VideoCapturer::Captuer() {
    this->Update();
    if (!this->runFlag1.load(std::memory_order_acquire)) {
        if (this->pVCD3D11Manager) this->pVCD3D11Manager->runFlag1.store(false, std::memory_order_release);
        return;
    }
    if (this->pVCD3D11Manager) this->pVCD3D11Manager->runFlag1.store(true, std::memory_order_release);
    if (!this->pVCD3D11Manager || !this->pVCD3D11Manager->ringReady.load(std::memory_order_acquire)) return;

    std::unique_lock lock(this->smutex);
    int readIdx = this->pVCD3D11Manager->readIdx.load(std::memory_order_acquire);
    RingSlot& slot = this->pVCD3D11Manager->ring[readIdx];
    if (!slot.hasNewFrame.load(std::memory_order_acquire)) return;
    if (!this->recordStartTime.has_value()) return;

    int64_t ptsUs = std::chrono::duration_cast<std::chrono::microseconds>(
        slot.captureTime.load(std::memory_order_acquire) - *this->recordStartTime).count();
    if (ptsUs < 0) ptsUs = 0;

    bool ok = this->hwCapture ? this->ReadbackHw(readIdx, ptsUs) : this->ReadbackSw(readIdx, ptsUs);
    // ReadbackSw/ReadbackHw 内部已在 ReleaseSync(0) 前完成 hasNewFrame=false 和 readIdx 推进
}

bool VideoCapturer::ReadbackSw(int slotIdx, int64_t ptsUs) {
    RingSlot& slot = this->pVCD3D11Manager->ring[slotIdx];
    D3D11_TEXTURE2D_DESC desc;
    slot.shareTex.pTex->GetDesc(&desc);

    av::PixelFormat srcFmt = DXGIFormatToAvPixelFormat(desc.Format);
    if (srcFmt == AV_PIX_FMT_NONE) return false;

    if (!this->pStagingTex || this->stagingWidth != (int)desc.Width ||
        this->stagingHeight != (int)desc.Height || this->stagingFormat != desc.Format) {
        this->ReleaseStagingTexture();
        D3D11_TEXTURE2D_DESC sd = {};
        sd.Width = desc.Width; sd.Height = desc.Height; sd.MipLevels = sd.ArraySize = 1;
        sd.Format = desc.Format; sd.SampleDesc.Count = 1;
        sd.Usage = D3D11_USAGE_STAGING; sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        if (FAILED(this->pVCD3D11Manager->pDevice->CreateTexture2D(&sd, nullptr, &this->pStagingTex)))
            return false;
        this->stagingWidth = (int)desc.Width; this->stagingHeight = (int)desc.Height;
        this->stagingFormat = desc.Format; this->srcPixelFormat = srcFmt;
    }

    slot.shareTex.pMutex->AcquireSync(1, INFINITE);
    if (desc.SampleDesc.Count > 1)
        this->pVCD3D11Manager->pContext->ResolveSubresource(this->pStagingTex, 0, slot.shareTex.pTex.Get(), 0, desc.Format);
    else
        this->pVCD3D11Manager->pContext->CopyResource(this->pStagingTex, slot.shareTex.pTex.Get());
    slot.hasNewFrame.store(false, std::memory_order_release);
    this->pVCD3D11Manager->readIdx.store(
        (slotIdx + 1) % this->pVCD3D11Manager->ringCapacity, std::memory_order_release);
    slot.shareTex.pMutex->ReleaseSync(0);

    D3D11_MAPPED_SUBRESOURCE map = {};
    if (FAILED(this->pVCD3D11Manager->pContext->Map(this->pStagingTex, 0, D3D11_MAP_READ, 0, &map)) || !map.pData)
        return false;
    size_t rowBytes = (size_t)this->stagingWidth * 4;
    size_t total   = rowBytes * (size_t)this->stagingHeight;
    if (this->readbackBuf.size() < total) this->readbackBuf.resize(total);
    for (UINT r = 0; r < (UINT)this->stagingHeight; ++r)
        memcpy(this->readbackBuf.data() + rowBytes * r,
               (const uint8_t*)map.pData + (size_t)map.RowPitch * r, rowBytes);
    this->pVCD3D11Manager->pContext->Unmap(this->pStagingTex, 0);

    av::VideoFrame frame(this->readbackBuf.data(), total, this->srcPixelFormat,
                         this->stagingWidth, this->stagingHeight);
    frame.setTimeBase({ 1, 1000000 });
    frame.setPts(av::Timestamp(ptsUs, frame.timeBase()));
    this->buffer.enqueue(std::move(frame));
    return true;
}

bool VideoCapturer::ReadbackHw(int slotIdx, int64_t ptsUs) {
    RingSlot& slot = this->pVCD3D11Manager->ring[slotIdx];
    if (!this->pVEncodeHelper || !this->pVEncodeHelper->IsHwAccel()) return false;

    auto ohwf = this->pVEncodeHelper->AllocHwFrame();
    if (!ohwf) return false;

    ID3D11Texture2D* poolTex = reinterpret_cast<ID3D11Texture2D*>(ohwf->raw()->data[0]);
    if (!poolTex) return false;

    // 持有共享纹理（key=1 可读），将捕获纹理拷入帧池纹理
    slot.shareTex.pMutex->AcquireSync(1, INFINITE);
    this->pVCD3D11Manager->pContext->CopyResource(poolTex, slot.shareTex.pTex.Get());
    slot.hasNewFrame.store(false, std::memory_order_release);
    this->pVCD3D11Manager->readIdx.store(
        (slotIdx + 1) % this->pVCD3D11Manager->ringCapacity, std::memory_order_release);
    slot.shareTex.pMutex->ReleaseSync(0);

    ohwf->setTimeBase({ 1, 1000000 });
    ohwf->setPts(av::Timestamp(ptsUs, ohwf->timeBase()));
    this->buffer.enqueue(std::move(*ohwf));
    return true;
}
