#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>
#include <d3d11.h>
#include <avcpp/av.h>
#include <avcpp/format.h>
#include <avcpp/codec.h>
#include <avcpp/frame.h>
#include <avcpp/packet.h>
#include <avcpp/codeccontext.h>
#include <avcpp/formatcontext.h>
#include <avcpp/videorescaler.h>

class MediaResourceManager final :public MulNX::ModuleBase {
    void HandleOnPresent();
    MulNX::GraphicsManager* pGraphicsManager = nullptr;



    // 录制相关
    av::FormatContext   ofctx;          // 代替原来的 av::OutputFormat
    av::Stream          vstream;
    av::VideoEncoderContext encoder;
    av::VideoRescaler   rescaler;
    av::PixelFormat     srcPixelFormat = AV_PIX_FMT_NONE;
    bool isRecording = false;
    int width = 0, height = 0;
    AVRational timeBase = { 1, 30 };  // 30 fps
    int64_t ptsCounter = 0;

    ID3D11Texture2D*    pStagingTex = nullptr;
    DXGI_FORMAT         stagingFormat = DXGI_FORMAT_UNKNOWN;
    int                 stagingWidth = 0;
    int                 stagingHeight = 0;

    bool StartRecording(const std::string& filename, int w, int h);
    bool StopRecording();
    void ReleaseStagingTexture();

    std::filesystem::path dirVedios;
public:
    bool Init()override;
    void Deinit()override;

    void ProcessMsg(MulNX::Message& msg)override;
};