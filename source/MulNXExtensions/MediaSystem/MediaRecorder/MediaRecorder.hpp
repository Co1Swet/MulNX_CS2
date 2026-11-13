#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

using CtrlPacket = std::variant<av::Packet, MulNX::AVControl>;

class MediaRecorder final :public MediaModule<MediaRecorder> {
    class AEncodeHelper* pAEncodeHelper = nullptr;
    class MediaParamManager* pMediaParamManager = nullptr;
    std::atomic<bool> startAsAdvance = false;

    av::FormatContext ofctx;
    std::chrono::steady_clock::time_point recordStartTime;

    std::atomic<bool> exitFlag = false;

    void Menu();

    void ReportCtxState();

    bool StartRecording(const std::string& dirPath, const std::string& fileName, bool advance);
    bool StopRecording();
    void CaptureCallback();
    void Encode();
    void ProcessMsg(MulNX::Message& msg);
    void Main();
    void WritePacket();

    bool Init()override;
public:
    moodycamel::ConcurrentQueue<CtrlPacket> bufferPackets{};
};