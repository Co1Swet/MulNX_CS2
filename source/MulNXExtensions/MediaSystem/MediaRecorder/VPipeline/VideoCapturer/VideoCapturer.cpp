#include "VideoCapturer.hpp"
#include <VPipeline/VCD3D11Manager/VCD3D11Manager.hpp>
#include <VPipeline/BufferCopier/BufferCopier.hpp>
#include <VPipeline/TextureMapper/TextureMapper.hpp>
#include <VPipeline/VEncodeHelper/VEncodeHelper.hpp>

bool VideoCapturer::Init() {
    this->pVCD3D11Manager = this->FindModule<VCD3D11Manager>("VCD3D11Manager");
    this->pVEncodeHelper = this->FindModule<VEncodeHelper>("VEncodeHelper");
    this->pTextureMapper = this->FindModule<TextureMapper>("TextureMapper");

    this->SendTask("VMap", "VMap", [this]() {
        this->Captuer();
        return true;
        });

    this->SubscribeSync("MediaSync/Reset", [this](auto&&...) {
        this->Reset();
        });

    this->SubscribeSync("MediaSync/SetOn", [this](MulNX::Message& msg) {
        auto&& [info] = msg.Access<MulNX::AVStartInfo>();
        this->StartCapture(info.startTime);
        });

    this->SubscribeSync("MediaSync/SetOff", [this](auto&&...) {
        this->StopCapture();
        });

    return true;
}

void VideoCapturer::Reset() {
    std::unique_lock lock(this->smutex);
    this->recordStartTime.store({});
}

void VideoCapturer::StartCapture(const std::chrono::steady_clock::time_point& startTime) {
    std::unique_lock lock(this->smutex);
    this->recordStartTime = startTime;
}

void VideoCapturer::StopCapture() {
    std::unique_lock lock(this->smutex);
}

void VideoCapturer::Captuer() {
    this->Update();

    std::unique_lock lock(this->smutex);
    int readIdx;
    if (auto h = this->pVCD3D11Manager->TryGetReadSide()) {
        readIdx = h.value();
    }
    else return;

    auto onExit = scope_exit([&]() {this->pVCD3D11Manager->ReleaseReadSide(readIdx);});

    auto& slot = this->pVCD3D11Manager->ring[readIdx].shareTex;

    int64_t ptsUs = std::chrono::duration_cast<std::chrono::microseconds>(
        slot.pFrameInfo->captureTime - this->recordStartTime.load()).count();
    if (ptsUs < 0) ptsUs = 0;

    auto oFrame = this->pTextureMapper->MapFrame(slot);
    if (!oFrame.has_value())return;
    auto frame = oFrame.value();

    frame.setTimeBase({ 1, 1000000 });
    frame.setPts(av::Timestamp(ptsUs, frame.timeBase()));
    this->pVEncodeHelper->bufferVFrames.enqueue(std::move(frame));
}