#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class MediaRecorder final :public MediaModuleBase {
    class AEncodeHelper* pAEncodeHelper = nullptr;
    class VEncodeHelper* pVEncodeHelper = nullptr;
    class VideoCapturer* pVideoCapturer = nullptr;
    class AudioCapturer* pAudioCapturer = nullptr;

    // 音视频输出
    av::FormatContext   ofctx;

    AVRational timeBase = { 1, 60 };  // 60 fps
    
    bool StartRecording(const std::string& filename, int w, int h);
    bool StopRecording();

    std::filesystem::path dirVedios;

    void Encode();
    void ProcessMsg(MulNX::Message& msg);

    void Main();
public:
    bool Init()override;
};