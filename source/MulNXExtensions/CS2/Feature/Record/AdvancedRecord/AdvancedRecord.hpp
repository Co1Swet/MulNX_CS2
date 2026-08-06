#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <MulNXExtensions/WinBaseHooks/FileRedirector/FileListenMixin.hpp>

class AdvancedRecord final :public CSModuleBase, public MediaModuleMixin<AdvancedRecord>, public FileListenMixin<AdvancedRecord> {
    class MediaParamManager* pMediaParamManager = nullptr;
    std::filesystem::path dirVideos;
    std::string outputFile = "record";
    // 基于时间槽的帧率限制状态
    std::atomic<std::chrono::steady_clock::time_point> recordStartTime;
    std::atomic<int64_t> minIntervalUs = 16667;     // captureFpsCap 换算（µs）
    std::atomic<int64_t> lastSlot = -1;             // 上次捕获所在时间槽序号

    std::atomic<int> frameCount = 0;

    void PublishNormal();
    void PublishAdvanced();
    void PublishStop(bool isAdvanced);

    void Menu();
    bool Init()override;
    void SetRecordStart(std::chrono::steady_clock::time_point t);
    void HandleBeforeCopyBackbuffer(MulNX::Message& msg);
    bool OnAdvanceRecord(MulNX::VFrameExInfo& info);

    std::optional<MulNX::Hook::Then> OnCreateFileW(CreateFileWControl* pfc)override;
};