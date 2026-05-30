#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class FrameCapturer;
class MediaRecorder final :public MediaModuleBase {
    FrameCapturer* pCapturer = nullptr;

    // 录制相关
    av::FormatContext   ofctx;
    av::Stream          vstream;
    av::VideoEncoderContext encoder;
    av::VideoRescaler   rescaler;
    
    int width = 0, height = 0;
    AVRational timeBase = { 1, 60 };  // 60 fps
    int64_t ptsCounter = 0;

    bool StartRecording(const std::string& filename, int w, int h);
    bool StopRecording();

    std::filesystem::path dirVedios;

    void Encode();
    void ProcessMsg(MulNX::Message& msg);

    void Main();
public:
    bool Init()override;
};