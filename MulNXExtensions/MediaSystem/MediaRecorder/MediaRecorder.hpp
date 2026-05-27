#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>
#include <MulNXExtensions/MediaSystem/D3D11AV.hpp>

class FrameCapturer;
class MediaRecorder final :public MulNX::ModuleBase {
    MulNX::GraphicsManager* pGraphicsManager = nullptr;
    FrameCapturer* pCapturer = nullptr;

    // 录制相关
    av::FormatContext   ofctx;
    av::Stream          vstream;
    av::VideoEncoderContext encoder;
    av::VideoRescaler   rescaler;
    
    bool isRecording = false;
    int width = 0, height = 0;
    AVRational timeBase = { 1, 30 };  // 30 fps
    int64_t ptsCounter = 0;

    bool StartRecording(const std::string& filename, int w, int h);
    bool StopRecording();

    std::filesystem::path dirVedios;

    void HandleOnPresent();
    void Encode();
    void ProcessMsg(MulNX::Message& msg);
public:
    bool Init()override;
};