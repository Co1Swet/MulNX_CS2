#include "BufferCopier.hpp"
#include <MulNXExtensions/MediaSystem/Videos/VCD3D11Manager/VCD3D11Manager.hpp>

bool BufferCopier::Init() {
    this->pVCD3D11Manager = this->FindModule<VCD3D11Manager>("VCD3D11Manager");
    this->pGraphicsManager = this->FindModule<MulNX::GraphicsManager>("GraphicsManager");

    (*this)
        .SubscribeSync("Hook/BeforePresent", [this](MulNX::Message& msg) {this->CopyTexture();})
        .SubscribeSync("MediaSync/BeforeCopyBackbuffer", [this](MulNX::Message& msg) {})
        ;

    return true;
}

void BufferCopier::SetCaptureFpsCap(int cap) {
    this->captureFpsCap.store(cap < 0 ? 0 : cap, std::memory_order_release);
    if (cap > 0) this->minIntervalUs = 1'000'000 / cap;
}

void BufferCopier::SetRecordStart(std::chrono::steady_clock::time_point t) {
    this->recordStartTime = t;
    this->lastSlot = -1;
}

void BufferCopier::CopyTexture() {
    if (!this->shouldCopy.load(std::memory_order_acquire)) return;

    // 基于时间槽的帧率上限：捕获落在当前时间槽的首帧，并量化 PTS 为槽边界
    int cap = this->captureFpsCap.load(std::memory_order_acquire);
    int64_t quantizedPtsUs = -1; // -1 表示不量化，使用实际 now
    if (cap > 0) {
        auto now = std::chrono::steady_clock::now();
        int64_t elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(
            now - this->recordStartTime).count();
        int64_t slot = elapsedUs / this->minIntervalUs;
        if (slot == this->lastSlot) {
            return; // 同一时间槽内不再重复捕获
        }
        this->lastSlot = slot;
        quantizedPtsUs = slot * this->minIntervalUs;
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = this->pGraphicsManager->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) return;

    auto h = this->pVCD3D11Manager->GetWriteSide();
    auto OnExit = scope_exit([&]() {
        this->pVCD3D11Manager->ReleaseWriteSide(h);
        });

    auto& slot = this->pVCD3D11Manager->ring[h].rawTex;

    hr = slot.pMutex->AcquireSync(0, INFINITE);
    if (hr != S_OK) {
        MulNX::ErrorTerminate("锁获取出错");
        return;
    }

    // 获取后备缓冲描述以确定是否 MSAA
    D3D11_TEXTURE2D_DESC bbDescR;
    backBuffer->GetDesc(&bbDescR);

    // 执行拷贝，并在拷贝完成时刻打 PTS
    if (bbDescR.SampleDesc.Count > 1) {
        this->pGraphicsManager->pd3dContext->ResolveSubresource(
            slot.pTex.Get(), 0, backBuffer.Get(), 0,
            static_cast<DXGI_FORMAT>(this->pVCD3D11Manager->srcDxgiFormat));
    }
    else {
        this->pGraphicsManager->pd3dContext->CopyResource(
            slot.pTex.Get(), backBuffer.Get());
    }

    // PTS：有帧率上限时量化为时间槽边界，否则取实际 now
    if (quantizedPtsUs >= 0) {
        slot.pFrameInfo->captureTime = this->recordStartTime + std::chrono::microseconds(quantizedPtsUs);
    }
    else {
        slot.pFrameInfo->captureTime = std::chrono::steady_clock::now();
    }

    hr = slot.pMutex->ReleaseSync(1);
    if (FAILED(hr)) {
        this->LogError("ReleaseSync(1) 失败");
    }
}