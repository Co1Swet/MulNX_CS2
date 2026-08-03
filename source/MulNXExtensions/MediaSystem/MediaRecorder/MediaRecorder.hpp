#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class MediaRecorder final :public MediaModuleBase {

    enum class RecordState: int {
        Free,
        Recording
    };

    class AEncodeHelper* pAEncodeHelper = nullptr;
    class VEncodeHelper*    pVEncodeHelper    = nullptr;
    class VCD3D11Manager*   pVCD3D11Manager   = nullptr;
    class MediaParamManager* pMediaParamManager = nullptr;

    std::atomic<RecordState> recordState = RecordState::Free;

    av::FormatContext ofctx;
    std::chrono::steady_clock::time_point recordStartTime;

    moodycamel::ConcurrentQueue<av::Packet> bufferPackets{};

    void Menu();

    bool StartRecording(const std::string& filename, bool advance);
    bool StopRecording();
    void CaptureCallback();
    void Encode();
    void ProcessMsg(MulNX::Message& msg);
    void Main();
    void WritePacket();

    bool Init()override;
};