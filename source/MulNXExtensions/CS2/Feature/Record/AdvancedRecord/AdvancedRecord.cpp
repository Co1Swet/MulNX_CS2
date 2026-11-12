#include "AdvancedRecord.hpp"
#include <Intro/HookConsole/HookConsole.hpp>
#include <MulNXExtensions/MediaSystem/MediaRecorder/MediaRecords.hpp>
#include <MulNXUtils/Times/Times.hpp>

void AdvancedRecord::Menu() {
    static std::string name{};
    ImGui::TextUnformatted("当前输出文件名：");
    ImGui::SameLine();
    ImGui::Text("%s", this->outputFile.load(std::memory_order_acquire)->c_str());
    ImGui::InputText("新文件名", &name);
    if (ImGui::Button("确认修改")) {
        this->outputFile.store(std::make_shared<std::string>(name), std::memory_order_release);
    }

    MulNX::UI::Checkbox("自动补充后缀时间戳", this->useTimeStamp);
    MulNX::UI::Checkbox("高级录制模式", this->pMediaState->nextStartUseAdvancedMode);

    if (ImGui::Button("开始录制")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Media/Record/Start"_hash);
        rp->str1 = this->dirVideos.string();
        const auto name = this->outputFile.load(std::memory_order_acquire);
        rp->str2 = this->useTimeStamp.load(std::memory_order_acquire) ?
            std::format("{}_{}", *name, MulNX::Time::GetTimestamp()) : *name;
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    if (ImGui::Button("结束录制")) {
        this->PublishAsync("Media/Record/Stop"_hash);
    }
}

bool AdvancedRecord::Init() {
    this->dirVideos = this->Path()->PathGetForShared("Videos");

    this->SubscribeSync("MediaSync/BeforeCopyBackbuffer", [this](MulNX::Message& msg) {
        this->HandleBeforeCopyBackbuffer(msg);
        });

    this->SubscribeSync("MediaSync/SetOn", [this](MulNX::Message& msg) {
        auto&& [info] = msg.Access<MulNX::AVStartInfo>();
        this->SetRecordStart(info);
        });

    this->SubscribeSync("Media/Record/Stop/FastNotify", [this](MulNX::Message&) {
        if (this->startAsAdvanced.load(std::memory_order_acquire)) {
            this->AsyncCommand("endmovie");
            this->AsyncCommand("host_framerate 0");
            this->startAsAdvanced.store(false, std::memory_order_release);
        }
        });

    this->UIRegisterCallback("UI.MediaSys/Control", [this](auto&&...) {
        this->Menu();
        });

    return true;
}
void AdvancedRecord::SetRecordStart(const MulNX::AVStartInfo& sInfo) {
    auto t = sInfo.startTime;
    this->recordStartTime = t;
    this->lastSlot = -1;

    this->frameCount = 0;
    this->startAsAdvanced = sInfo.startAsAdvancedMode;
    int fps = sInfo.targetFPS;
    if (fps > 0) {
        this->minIntervalUs.store(1000000LL / fps, std::memory_order_release);
    }
    else {
        this->minIntervalUs.store(0, std::memory_order_release);
    }

    if (!this->startAsAdvanced)return;
    this->AsyncCommand(std::format("host_framerate {}", fps));
    this->AsyncCommand(std::format("startmovie {} wav framerate {}", *sInfo.pFilenameWithoutStem, fps));
}

void AdvancedRecord::HandleBeforeCopyBackbuffer(MulNX::Message& msg) {
    auto&& [info] = msg.Access<MulNX::VFrameExInfo>();
    if (!this->pMediaState->MediaSystemGlobalWorkFlag) {
        info.needDrop = true;
        return;
    }
    if (this->startAsAdvanced.load(std::memory_order_acquire)) {
        this->OnAdvanceRecord(info);
    }
    else {
        this->OnNormalRecord(info);
    }
}

void AdvancedRecord::OnNormalRecord(MulNX::VFrameExInfo& info) {
    // 基于时间槽的帧率上限：捕获落在当前时间槽的首帧，并量化 PTS 为槽边界
    const auto intervalUs = this->minIntervalUs.load(std::memory_order_acquire);
    int64_t quantizedPtsUs = -1; // -1 表示不量化，使用实际 now
    if (intervalUs > 0) {
        auto now = std::chrono::steady_clock::now();
        int64_t elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(
            now - this->recordStartTime.load()).count();
        int64_t slot = elapsedUs / intervalUs;
        if (slot == this->lastSlot) {
            info.needDrop = true;
            return; // 同一时间槽内不再重复捕获
        }
        this->lastSlot = slot;
        quantizedPtsUs = slot * intervalUs;
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

void AdvancedRecord::OnAdvanceRecord(MulNX::VFrameExInfo& info) {
    while (this->pMediaState->encodingOverload) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    info.needDrop = false;

    // 录制起始时间（原子读取）
    auto startTime = this->recordStartTime.load();

    // 当前帧的序号（先取值再自增，第一帧序号为 0）
    int frameIdx = this->frameCount++;

    // 计算固定帧间隔（微秒）
    std::chrono::microseconds frameDuration(this->minIntervalUs.load(std::memory_order_acquire));

    // 生成虚构的捕获时间：起始时间 + 帧序号 × 帧间隔
    info.captureTime = startTime + frameIdx * frameDuration;
}