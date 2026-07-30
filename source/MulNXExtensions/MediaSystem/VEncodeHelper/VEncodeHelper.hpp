#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class VEncodeHelper final :public MediaModuleBase {
    class MediaParamManager* pMediaParamManager = nullptr;
    class VCD3D11Manager* pVCD3D11Manager = nullptr;
    av::Stream vstream;
    av::VideoEncoderContext encoder;
    av::VideoRescaler rescaler;

    std::string chosenEncoder;
    std::atomic<int> width = 0, height = 0;
    av::PixelFormat dstPixFmt = AV_PIX_FMT_NV12;   // 软编用 NV12
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
