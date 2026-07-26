#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <MulNXExtensions/MediaSystem/MediaParamManager/RecordParams.hpp>

class VEncodeHelper final :public MediaModuleBase {
    av::Stream vstream;
    av::VideoEncoderContext encoder;
    av::VideoRescaler rescaler;

    std::string chosenEncoder;
    int width = 0, height = 0;
    av::PixelFormat dstPixFmt = AV_PIX_FMT_NV12;   // 软编用 NV12
    int64_t ptsCounter = 0;
    AVRational timeBase{ 1, 1000000 };

    bool OpenEncoder(av::FormatContext* oCtx, const RecordParams& rp,
                     const av::Codec& codec, int fps, int srcW, int srcH);
    void CheckRescaler(int srcW, int srcH, av::PixelFormat srcFmt);
public:
    bool Init() override;

    void SetOn(av::FormatContext* oCtx, const RecordParams& rp,
               int srcW, int srcH, av::PixelFormat srcFmt,
               ID3D11Device* device, int fps);
    std::optional<av::Packet> Encode(av::VideoFrame srcFrame);

    std::optional<av::Packet> TrySetOff();
    void Reset();
};
