#include "AdvancedRecord.hpp"
#include <MulNXExtensions/MediaSystem/MediaParamManager/MediaParamManager.hpp>

bool AdvancedRecord::Init() {
    this->pMediaParamManager = this->FindModule<MediaParamManager>("MediaParamManager");

    this->SubscribeSync("MediaSync/BeforeCopyBackbuffer", [this](MulNX::Message& msg) {
        this->HandleBeforeCopyBackbuffer(msg);
        });

    this->SubscribeSync("MediaSync/SetOn", [this](MulNX::Message& msg) {
        auto&& [info] = msg.Access<MulNX::AVStartInfo>();
        this->SetRecordStart(info.startTime);
        });

    return true;
}

void AdvancedRecord::SetRecordStart(std::chrono::steady_clock::time_point t) {
    this->recordStartTime = t;
    this->lastSlot = -1;
}

void AdvancedRecord::HandleBeforeCopyBackbuffer(MulNX::Message& msg) {
    auto&& [info] = msg.Access<MulNX::VFrameExInfo>();
    // 基于时间槽的帧率上限：捕获落在当前时间槽的首帧，并量化 PTS 为槽边界
    int cap = this->pMediaParamManager->targetFPS.load(std::memory_order_acquire);
    int64_t quantizedPtsUs = -1; // -1 表示不量化，使用实际 now
    if (cap > 0) {
        auto now = std::chrono::steady_clock::now();
        int64_t elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(
            now - this->recordStartTime.load()).count();
        int64_t slot = elapsedUs / this->minIntervalUs;
        if (slot == this->lastSlot) {
            info.needDrop = true;
            return; // 同一时间槽内不再重复捕获
        }
        this->lastSlot = slot;
        quantizedPtsUs = slot * this->minIntervalUs;
    }

    // PTS：有帧率上限时量化为时间槽边界，否则取实际 now
    if (quantizedPtsUs >= 0) {
        info.captureTime = this->recordStartTime.load() + std::chrono::microseconds(quantizedPtsUs);
    }
    else {
        info.captureTime = std::chrono::steady_clock::now();
    }
    info.needDrop = false;
}