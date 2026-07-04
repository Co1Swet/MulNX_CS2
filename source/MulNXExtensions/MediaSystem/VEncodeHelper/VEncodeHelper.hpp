#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <MulNXExtensions/MediaSystem/MediaParamManager/RecordParams.hpp>

class VEncodeHelper final :public MediaModuleBase {
    av::Stream vstream;
    av::VideoEncoderContext encoder;
    av::VideoRescaler rescaler;

    std::string chosenEncoder;
    bool hwAccel = false;
    int width = 0, height = 0;
    av::PixelFormat dstPixFmt = AV_PIX_FMT_NV12;   // 软编用 NV12
    int64_t ptsCounter = 0;
    AVRational timeBase{ 1, 1000000 };

    // 硬件上下文
    AVBufferRef* hwD3D11DeviceRef = nullptr;   // D3D11VA 设备（包装捕获设备）
    AVBufferRef* hwD3D11FramesRef = nullptr;   // D3D11VA 帧池（所有 hw 路径的源）
    AVBufferRef* hwTargetDeviceRef = nullptr;  // 目标设备（CUDA/QSV，AMF 时为 null）
    AVBufferRef* hwTargetFramesRef = nullptr;  // 目标帧池（CUDA/QSV，AMF 时为 null）
    av::PixelFormat hwInputPixFmt = AV_PIX_FMT_NONE;  // 编码器接受的 hw 像素格式

    bool OpenEncoder(av::FormatContext* oCtx, const RecordParams& rp,
                     const av::Codec& codec, int fps, int srcW, int srcH);
    void CheckRescaler(int srcW, int srcH, av::PixelFormat srcFmt);
    void ApplyOpts(av::Dictionary& opts, const RecordParams& rp, const std::string& encName);
    bool SetupHwContext(ID3D11Device* device, int w, int h);

public:
    bool Init() override;

    void SetOn(av::FormatContext* oCtx, const RecordParams& rp,
               int srcW, int srcH, av::PixelFormat srcFmt,
               ID3D11Device* device, int fps);
    std::optional<av::Packet> Encode(av::VideoFrame srcFrame);
    std::optional<av::VideoFrame> AllocHwFrame();

    std::optional<av::Packet> TrySetOff();
    void Reset();
    bool IsHwAccel() const { return this->hwAccel; }
};
