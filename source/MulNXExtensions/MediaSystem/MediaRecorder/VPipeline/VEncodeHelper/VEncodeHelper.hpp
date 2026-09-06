#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class VEncodeHelper final :public MediaModuleBase {
    class MediaParamManager* pMediaParamManager = nullptr;
    av::Stream vstream;
    av::VideoEncoderContext encoder;
    av::VideoRescaler rescaler;

    std::atomic<int> width = 0;
    std::atomic<int> height = 0;
    av::PixelFormat dstPixFmt{};
    AVRational timeBase{ 1, 1000000 };

    bool Init() override;
    bool OpenEncoder(av::FormatContext* oCtx, const av::Codec& codec);
    void CheckRescaler(int srcW, int srcH, av::PixelFormat srcFmt);
    void Reset();
    void SetOn(av::FormatContext* oCtx);
public:
    std::optional<av::Packet> Encode();
    std::optional<av::Packet> TrySetOff();
    moodycamel::ConcurrentQueue<av::VideoFrame> bufferVFrames{};
};
