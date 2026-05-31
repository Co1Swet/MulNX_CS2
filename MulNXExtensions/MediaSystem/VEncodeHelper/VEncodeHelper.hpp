#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class VEncodeHelper final :public MediaModuleBase {
    av::Stream vstream;
    av::VideoEncoderContext encoder;
    av::VideoRescaler rescaler;

    int width = 0;
    int height = 0;

    int64_t ptsCounter = 0;

    AVRational timeBase;
public:
    bool Init()override;
    void SetOn(av::FormatContext* oCtx, int width, int height, AVRational timeBase);
    void CheckRescaler(int srcWidth, int srcHeight, av::PixelFormat srcFormat);
    std::optional<av::Packet> TrySetOff();
    std::optional<av::Packet> Encode(av::VideoFrame srcFrame);
};