#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class AdvancedRecord final :public CSModuleBase, public MediaModuleMixin<AdvancedRecord> {
    std::atomic<bool> startAsAdvanced = false;
    fs::path dirVideos;
    std::string outputFile = "record";
    // 基于时间槽的帧率限制状态
    std::atomic<std::chrono::steady_clock::time_point> recordStartTime;
    std::atomic<int64_t> minIntervalUs = 0;         // 本次录制帧间隔（µs），0 表示不限帧
    std::atomic<int64_t> lastSlot = -1;             // 上次捕获所在时间槽序号

    std::atomic<int> frameCount = 0;

    void Menu();
    bool Init()override;
    void SetRecordStart(const MulNX::AVStartInfo& sInfo);
    void HandleBeforeCopyBackbuffer(MulNX::Message& msg);
    bool OnAdvanceRecord(MulNX::VFrameExInfo& info);
};