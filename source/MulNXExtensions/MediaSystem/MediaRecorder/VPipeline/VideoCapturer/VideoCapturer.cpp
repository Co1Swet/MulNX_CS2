#include "VideoCapturer.hpp"
#include <VPipeline/VCD3D11Manager/VCD3D11Manager.hpp>
#include <VPipeline/BufferCopier/BufferCopier.hpp>
#include <VPipeline/TextureMapper/TextureMapper.hpp>
#include <VPipeline/VEncodeController/VEncodeController.hpp>

bool VideoCapturer::Init() {
    this->pVCD3D11Manager = this->FindModule<VCD3D11Manager>();
    this->pVEncodeController = this->FindModule<VEncodeController>();
    this->pTextureMapper = this->FindModule<TextureMapper>();

    this->SendTask("VMap", "VMap", [this]() {
        this->Main();
        return true;
        });

    this->SubscribeSync("MediaSync/SetOn", [this](MulNX::Message& msg) {
        auto&& [info] = msg.Access<MulNX::AVStartInfo>();        
        this->SetOn(info.startTime);
        });

    this->SubscribeSync("MediaSync/SetOff", [this](auto&&...) {
        this->StopCapture();
        });

    return true;
}

void VideoCapturer::SetOn(const std::chrono::steady_clock::time_point& startTime) {
    std::unique_lock lock(this->smutex);
    this->recordStartTime.store({});
    this->recordStartTime = startTime;
}

void VideoCapturer::StopCapture() {
    std::unique_lock lock(this->smutex);
    this->pVEncodeController->bufferVFrames.enqueue(MulNX::AVControl(MulNX::AVControl::Kind::Stop));
}

void VideoCapturer::Main() {
    std::unique_lock lock(this->smutex);
    while (this->Captuer()) {
        
    }
}

bool VideoCapturer::Captuer() {
    auto view = this->pVCD3D11Manager->TryGetCaptureView();
    if (!view.has_value())return false;

    int64_t ptsUs = std::chrono::duration_cast<std::chrono::microseconds>(
        view->pFrameInfo->captureTime - this->recordStartTime.load()).count();
    if (ptsUs < 0) ptsUs = 0;

    auto oFrame = this->pTextureMapper->MapFrame(*view);
    if (!oFrame.has_value())return false;
    auto frame = oFrame.value();

    frame.setTimeBase({ 1, 1000000 });
    frame.setPts(av::Timestamp(ptsUs, frame.timeBase()));
    this->pVEncodeController->bufferVFrames.enqueue(std::move(frame));
    return true;
}