#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <avcpp/audioresampler.h>

class MediaRecorder final :public MediaModuleBase {
    class VideoCapturer* pVideoCapturer = nullptr;
    class AudioCapturer* pAudioCapturer = nullptr;

    // 录制相关
    av::FormatContext   ofctx;
    av::Stream          vstream;
    av::VideoEncoderContext encoder;
    av::VideoRescaler   rescaler;
    // audio encoder
    av::Stream          astream;
    av::AudioEncoderContext aencoder;
    av::AudioResampler  aresampler;
    // accumulator for audio samples to meet encoder frameSize requirements
    av::AudioSamples    audioAccum;
    int                 audioAccumCount = 0;

    std::deque<av::AudioSamples> audioFifo;

    int width = 0, height = 0;
    AVRational timeBase = { 1, 60 };  // 60 fps
    int64_t ptsCounter = 0;
    int64_t aptsCounter = 0;

    bool StartRecording(const std::string& filename, int w, int h);
    bool StopRecording();

    std::filesystem::path dirVedios;

    void Encode();
    void ProcessMsg(MulNX::Message& msg);

    void Main();
public:
    bool Init()override;
};