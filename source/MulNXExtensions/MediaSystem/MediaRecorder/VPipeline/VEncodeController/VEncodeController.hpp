#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include "IVEncoder.hpp"

class VEncodeController final :public MediaModuleBase {
    class MediaParamManager* pMediaParamManager = nullptr;
    class MediaRecorder* pMediaRecorder = nullptr;

    av::VideoEncoderContext encoder{};

    av::Stream vstream{};
    av::VideoRescaler rescaler{};

    std::atomic<int> width = 0;
    std::atomic<int> height = 0;
    av::PixelFormat dstPixFmt{};
    AVRational timeBase{ 1, 1000000 };

    std::atomic<size_t> bufferSize = 0;

    void Menu(MulNX::UICoordinator* uico);
    bool Init() override;
    void SetEncoderParams(av::VideoEncoderContext* encoder);
    bool OpenEncoder(av::FormatContext* oCtx, const av::Codec& codec);
    void CheckRescaler(int srcW, int srcH, av::PixelFormat srcFmt);
    void SetOn(av::FormatContext* oCtx);

    bool OnEncode();
public:
    MulNX::IVEncoder* pEncoder;

    using CtrlVideoFrame = std::variant<av::VideoFrame, MulNX::AVControl>;
    moodycamel::ConcurrentQueue<CtrlVideoFrame> bufferVFrames{};
};
