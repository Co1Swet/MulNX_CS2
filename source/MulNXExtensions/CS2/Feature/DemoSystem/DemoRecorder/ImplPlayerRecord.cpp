#include "DemoRecorder.hpp"
#include <Support/TimeController/TimeController.hpp>

MulNX::CoTask DemoRecorder::CoSetOBTarget(bool& ok, Steam64UID target)const {
    ok = false;
    // 设置观战目标
    for (int i = 0;i < 3;++i) {
        auto obUID = this->CS2Entitys->TryGetObservingSteam64UID();
        if (!obUID || *obUID != target) {
            // 发送设置观察目标消息
            MulNX::Message specMsg("Observe/SpecSteam64UID"_hash);
            auto&& [uidRef] = specMsg.Access<Steam64UID>();
            uidRef = target;
            this->PublishAsync(std::move(specMsg));
        }
        co_await this->WaitTimed(ok, 500, [&]() {
            auto obUID = this->CS2Entitys->TryGetObservingSteam64UID();
            if (!obUID || *obUID != target)return false;
            return true;
            });
        if (ok)break;
        this->LogWarning("观战设置失败，重新尝试");
    }
}

MulNX::CoTask DemoRecorder::PreEnterPlayer(bool& ok, const Demo::Record::Task& rTask)const {
    co_await this->CoSetOBTarget(ok, rTask.uid);
    if (!ok) {
        this->LogError("多次尝试设置观战失败！丢弃一个片段");
        co_return;
    }
    this->LogInfo(std::format("已经设置观战目标：{}", rTask.uid));
}

MulNX::CoTask DemoRecorder::OnRecordingPlayer(const Demo::Record::Task& curRecordTask, bool& recordSucc)const {
    std::optional<int> observingMismatchSinceTick;
    co_await this->WaitUntil([&] {
        return !this->OnRecordingPlayer(curRecordTask, recordSucc, observingMismatchSinceTick);
        });
}

// 返回true以表示保持录制状态，返回false以表示录制结束
bool DemoRecorder::OnRecordingPlayer(const Demo::Record::Task& curRecordTask, bool& recordSucc,
    std::optional<int>& observingMismatchSinceTick)const {
    
    const int curTick = this->CS2Time->GetDemoTick();
    if (curRecordTask.uid != 0) {
        auto obUID = this->CS2Entitys->TryGetObservingSteam64UID();
        if (!obUID || *obUID != curRecordTask.uid) {
            constexpr int mismatchTimeoutTicks = 2.25f * 64;
            if (!observingMismatchSinceTick) {
                observingMismatchSinceTick = curTick;
                this->LogWarning(std::format("当前观战目标={}，意外地不等于录制目标={}，等待目标恢复（超时{} tick）",
                    obUID ? std::to_string(*obUID) : "无", curRecordTask.uid, mismatchTimeoutTicks));
            }
            else if (curTick - *observingMismatchSinceTick >= mismatchTimeoutTicks) {
                this->LogError(std::format("当前观战目标持续不等于录制目标超过{} tick，当前目标={}，录制目标={}，结束当前片段录制",
                    mismatchTimeoutTicks,
                    obUID ? std::to_string(*obUID) : "无", curRecordTask.uid));
                return false;
            }
        }
        else {
            observingMismatchSinceTick.reset();
        }
    }

    return this->BaseRecordingCheck(curRecordTask, recordSucc);
}