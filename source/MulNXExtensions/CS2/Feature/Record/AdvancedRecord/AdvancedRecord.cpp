#include "AdvancedRecord.hpp"
#include <MulNXExtensions/MediaSystem/MediaParamManager/MediaParamManager.hpp>

bool AdvancedRecord::Init() {
    this->pMediaParamManager = this->FindModule<MediaParamManager>("MediaParamManager");
    this->dirVideos = this->Path()->PathGetForShared("Videos");

    this->UIRegisterCallback("UI.MediaSys", [this](auto&&...) {
        MulNX::UI::SliderInt("高级录制帧率", this->advanceFPS, 1, 1000);

        if (ImGui::Button("尝试CS2录制")) {
            this->isAdvance = true;
            this->frameCount = 0;
            this->AsyncCommand(std::format("startmovie mymulnx wav framerate {}", this->advanceFPS.load()));
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Media/Record/StartAdvanced"_hash);
            rp->str1 = (this->dirVideos / "CS2test").string();
            this->PublishAsync(std::move(msg));
        }
        if (ImGui::Button("停止CS2录制")) {
            this->isAdvance = false;
            this->AsyncCommand("endmovie");
            this->PublishAsync("Media/Record/Stop"_hash);
        }

        });

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
    if (this->OnAdvanceRecord(info))return;
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

bool AdvancedRecord::OnAdvanceRecord(MulNX::VFrameExInfo& info) {
    if (!this->isAdvance) return false;

    info.needDrop = false;

    // 获取设定的固定帧率
    int fps = this->advanceFPS.load();
    // 录制起始时间（原子读取）
    auto startTime = this->recordStartTime.load();

    // 当前帧的序号（先取值再自增，第一帧序号为 0）
    int frameIdx = this->frameCount++;

    // 计算固定帧间隔（微秒）
    std::chrono::microseconds frameDuration(1000000 / fps);

    // 生成虚构的捕获时间：起始时间 + 帧序号 × 帧间隔
    info.captureTime = startTime + frameIdx * frameDuration;

    return true;  // 已自行处理，跳过常规的时间槽限帧逻辑
}