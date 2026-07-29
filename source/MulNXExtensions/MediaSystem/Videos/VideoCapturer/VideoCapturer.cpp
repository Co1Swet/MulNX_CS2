#include "VideoCapturer.hpp"
#include <MulNXExtensions/MediaSystem/Videos/VCD3D11Manager/VCD3D11Manager.hpp>
#include <MulNXExtensions/MediaSystem/Videos/BufferCopier/BufferCopier.hpp>
#include <MulNXExtensions/MediaSystem/Videos/TextureMapper/TextureMapper.hpp>
#include <MulNXExtensions/MediaSystem/VEncodeHelper/VEncodeHelper.hpp>

bool VideoCapturer::Init() {
    this->pVCD3D11Manager = this->FindModule<VCD3D11Manager>("VCD3D11Manager");
    this->pVEncodeHelper = this->FindModule<VEncodeHelper>("VEncodeHelper");
    this->pBufferCopier = this->FindModule<BufferCopier>("BufferCopier");
    this->pTextureMapper = this->FindModule<TextureMapper>("TextureMapper");
    this->SendTask("Capture", "Capture", [this]() -> bool { this->Captuer(); return true; });
    return true;
}

void VideoCapturer::Reset() {
    std::unique_lock lock(this->smutex);
    this->recordStartTime.reset();
    this->vCapturing.store(false);
}

void VideoCapturer::StartCapture(const std::chrono::steady_clock::time_point& startTime) {
    std::unique_lock lock(this->smutex);
    this->recordStartTime = startTime;
    this->vCapturing.store(true, std::memory_order_release);
}

void VideoCapturer::StopCapture() {
    std::unique_lock lock(this->smutex);
    this->vCapturing.store(false);
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
    if (!this->vCapturing.load(std::memory_order_acquire)) {
        this->pBufferCopier->shouldCopy.store(false, std::memory_order_release);
        return;
    }
    this->pBufferCopier->shouldCopy.store(true, std::memory_order_release);

    std::unique_lock lock(this->smutex);
    int readIdx;
    if (auto h = this->pVCD3D11Manager->TryGetReadSide()) {
        readIdx = h.value();
    }
    else return;

    auto onExit = scope_exit([&]() {this->pVCD3D11Manager->ReleaseReadSide(readIdx);});

    auto& slot = this->pVCD3D11Manager->ring[readIdx].shareTex;
    if (!this->recordStartTime.has_value()) return;

    int64_t ptsUs = std::chrono::duration_cast<std::chrono::microseconds>(
        slot.captureTime->load(std::memory_order_acquire) - *this->recordStartTime).count();
    if (ptsUs < 0) ptsUs = 0;

    auto oFrame = this->pTextureMapper->MapFrame(slot);
    if (!oFrame.has_value())return;
    auto frame = oFrame.value();

    frame.setTimeBase({ 1, 1000000 });
    frame.setPts(av::Timestamp(ptsUs, frame.timeBase()));
    this->buffer.enqueue(std::move(frame));
}