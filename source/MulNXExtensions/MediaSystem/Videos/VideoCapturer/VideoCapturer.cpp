#include "VideoCapturer.hpp"
#include <MulNXExtensions/MediaSystem/Videos/VCD3D11Manager/VCD3D11Manager.hpp>
#include <MulNXExtensions/MediaSystem/Videos/BufferCopier/BufferCopier.hpp>
#include <MulNXExtensions/MediaSystem/VEncodeHelper/VEncodeHelper.hpp>

bool VideoCapturer::Init() {
    this->pVCD3D11Manager = this->FindModule<VCD3D11Manager>("VCD3D11Manager");
    this->pVEncodeHelper = this->FindModule<VEncodeHelper>("VEncodeHelper");
    this->pBufferCopier = this->FindModule<BufferCopier>("BufferCopier");
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
    this->runFlag1.store(false);
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
    while (this->buffer.try_dequeue(discard));
}

std::optional<av::VideoFrame> VideoCapturer::TryPop() {
    av::VideoFrame f;
    return this->buffer.try_dequeue(f) ? std::optional(std::move(f)) : std::nullopt;
}

void VideoCapturer::Captuer() {
    this->Update();
    if (!this->runFlag1.load(std::memory_order_acquire)) {
        this->pBufferCopier->runFlag1.store(false, std::memory_order_release);
        return;
    }
    this->pBufferCopier->runFlag1.store(true, std::memory_order_release);

    std::unique_lock lock(this->smutex);
    int readIdx;
    if (auto h = this->pVCD3D11Manager->TryGetReadSide()) {
        readIdx = h.value();
    }
    else return;

    scope_exit([&]() {this->pVCD3D11Manager->ReleaseReadSide(readIdx);});

    auto& slot = this->pVCD3D11Manager->ring[readIdx].shareTex;
    if (!this->recordStartTime.has_value()) return;

    int64_t ptsUs = std::chrono::duration_cast<std::chrono::microseconds>(
        slot.captureTime->load(std::memory_order_acquire) - *this->recordStartTime).count();
    if (ptsUs < 0) ptsUs = 0;

    bool ok = this->ReadbackSw(readIdx, ptsUs);
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
        if (FAILED(this->pVCD3D11Manager->pReadSideDevice->CreateTexture2D(&sd, nullptr, &this->pStagingTex)))
            return false;
        this->stagingWidth = (int)desc.Width; this->stagingHeight = (int)desc.Height;
        this->stagingFormat = desc.Format; this->srcPixelFormat = srcFmt;
    }

    slot.shareTex.pMutex->AcquireSync(1, INFINITE);
    if (desc.SampleDesc.Count > 1)
        this->pVCD3D11Manager->pReadSideContext->ResolveSubresource(this->pStagingTex, 0, slot.shareTex.pTex.Get(), 0, desc.Format);
    else
        this->pVCD3D11Manager->pReadSideContext->CopyResource(this->pStagingTex, slot.shareTex.pTex.Get());

    slot.shareTex.pMutex->ReleaseSync(0);

    D3D11_MAPPED_SUBRESOURCE map = {};
    if (FAILED(this->pVCD3D11Manager->pReadSideContext->Map(this->pStagingTex, 0, D3D11_MAP_READ, 0, &map)) || !map.pData)
        return false;
    size_t rowBytes = (size_t)this->stagingWidth * 4;
    size_t total = rowBytes * (size_t)this->stagingHeight;
    if (this->readbackBuf.size() < total) this->readbackBuf.resize(total);
    for (UINT r = 0; r < (UINT)this->stagingHeight; ++r)
        memcpy(this->readbackBuf.data() + rowBytes * r,
            (const uint8_t*)map.pData + (size_t)map.RowPitch * r, rowBytes);
    this->pVCD3D11Manager->pReadSideContext->Unmap(this->pStagingTex, 0);

    av::VideoFrame frame(this->readbackBuf.data(), total, this->srcPixelFormat,
        this->stagingWidth, this->stagingHeight);
    frame.setTimeBase({ 1, 1000000 });
    frame.setPts(av::Timestamp(ptsUs, frame.timeBase()));
    this->buffer.enqueue(std::move(frame));
    return true;
}