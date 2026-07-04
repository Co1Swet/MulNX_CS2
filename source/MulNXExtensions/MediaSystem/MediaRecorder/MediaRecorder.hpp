#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class MediaRecorder final :public MediaModuleBase {
    class AEncodeHelper*    pAEncodeHelper    = nullptr;
    class VEncodeHelper*    pVEncodeHelper    = nullptr;
    class VideoCapturer*    pVideoCapturer    = nullptr;
    class AudioCapturer*    pAudioCapturer    = nullptr;
    class VCD3D11Manager*   pVCD3D11Manager   = nullptr;
    class MediaParamManager* pMediaParamManager = nullptr;

    std::filesystem::path dirVideos;
    av::FormatContext ofctx;
    std::chrono::steady_clock::time_point recordStartTime;

    bool StartRecording(const std::string& filename);
    bool StopRecording();
    void CaptureCallback(MulNX::UINode* node);
    void Encode();
    void ProcessMsg(MulNX::Message& msg);
    void Main();

public:
    bool Init()override;
};
