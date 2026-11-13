#pragma once
#include <MulNXExtensions/MediaSystem/AVCodecManager/AVCodecManager.hpp>

class VEncodeController final :public MediaModule<VEncodeController> {
    class MediaParamManager* pMediaParamManager = nullptr;
    class MediaRecorder* pMediaRecorder = nullptr;
    AVCodecManager* pAVCodecManager = nullptr;

    av::VideoEncoderContext vEncoderContext{};

    av::Stream vstream{};

    av::VideoRescaler rescaler{};
    std::optional<av::VideoFrame> rescaleFrame{};

    std::atomic<int> width = 0;
    std::atomic<int> height = 0;
    av::PixelFormat dstPixFmt{};
    AVRational timeBase{ 1, 1000000 };

    std::atomic<size_t> bufferSize = 0;

    void Menu(MulNX::UICoordinator* uico);
    bool Init() override;
    std::optional<av::VideoEncoderContext> OpenEncoder(const MediaParamManager& rp)const;
    void CheckRescaler(int srcW, int srcH, av::PixelFormat srcFmt);
    void SetOn(av::FormatContext* oCtx);

    bool OnEncode();
    void EncodeOne(av::VideoFrame&& srcFrame);
    bool ExitCode();
public:
    using CtrlVideoFrame = std::variant<av::VideoFrame, MulNX::AVControl>;
    moodycamel::ConcurrentQueue<CtrlVideoFrame> bufferVFrames{};
};
