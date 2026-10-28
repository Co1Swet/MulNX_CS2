#include "DemoRecorder.hpp"
#include <Intro/HookConsole/HookConsole.hpp>
#include <Support/TimeController/TimeController.hpp>

MulNX::CoTask DemoRecorder::CoSetOBTarget(bool& ok, Steam64UID target) {
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
            if (!obUID || (obUID.value() != target))return false;
            return true;
            });
        if (ok)break;
        this->LogWarning("观战设置失败，重新尝试");
    }
}

MulNX::CoTask DemoRecorder::EnsurePassChange() {
    std::shared_ptr<Demo::RecordPass> tryPass = nullptr;
    {
        std::unique_lock lock(this->smutex);
        if (this->recordPasses.empty())co_return;
        tryPass = this->recordPasses.front();
    }
    // 尝试发生Pass切换
    auto pNowPlaying = this->pDemSys->GetCurrentPlayingDemoName();
    bool needChange = false;
    if (!pNowPlaying) {
        needChange = true;
    }
    else if (*pNowPlaying != tryPass->demoFileName) {
        needChange = true;
        this->AsyncCommand("disconnect");
        std::chrono::steady_clock::time_point lastSent = std::chrono::steady_clock::now();
        co_await this->WaitUntil([&]() -> bool {
            if (!this->CS2Time->IsPlayingDemo()) return true;
            auto now = std::chrono::steady_clock::now();
            if (now - lastSent >= std::chrono::seconds(5)) {
                this->AsyncCommand("disconnect");
                lastSent = now;
            }
            return false;
            });
    }
    if (needChange) {
        for (int i = 0;;++i) {
            this->LogInfo(std::format("尝试为新Pass切换到指定Demo：{}", tryPass->demoFileName));
            if (!this->CS2Time->IsPlayingDemo()) { // 这个检验是防止在触发过程中发新playdemo
                this->AsyncCommand(std::format("playdemo {}", tryPass->demoFileName));
            }
            bool ok;
            auto nowTick = this->CS2Time->GetDemoTick();
            co_await this->WaitTimed(ok, 3000, [&]() {
                auto pNow = this->pDemSys->GetCurrentPlayingDemoName();
                if (!pNow)return false;
                if (*pNow != tryPass->demoFileName)return false;
                auto curTick = this->CS2Time->GetDemoTick();
                if (curTick < 17 || std::abs(curTick - nowTick) < 64)return false; // 0x11
                if (!this->CS2Time->IsPlayingDemo())return false;
                return true;
                });
            if (ok) {
                this->LogInfo("切换Demo成功");
                break;
            }
            else if (i < 3) {
                this->LogWarning("切换尚未完成，继续尝试");
                continue;
            }
            else {
                this->LogError("多次尝试切换失败");
                co_return;
            }
        }
    }
    {
        std::unique_lock lock(this->smutex);
        if (this->recordPasses.empty() || this->recordPasses.front() != tryPass) {
            co_return;
        }
        this->recordPasses.pop_front();
        this->currentPass.store(tryPass, std::memory_order_release);
    }
    co_await this->WaitMilliseconds(1000);
}

MulNX::CoTask DemoRecorder::GoNextTask(const Demo::RecordTask*& pTask) {
    auto curPass = this->currentPass.load(std::memory_order_acquire);
    while (true) {
        if (!curPass) {
            co_await this->EnsurePassChange();
            curPass = this->currentPass.load(std::memory_order_acquire);
            if (!curPass)co_return;
            this->indexTask.store(-1, std::memory_order_release);
        }

        int curIdx = this->indexTask.load(std::memory_order_acquire);
        int nextIdx = curIdx + 1;

        if (nextIdx < curPass->rTasks.size()) {
            pTask = &curPass->rTasks[nextIdx];
            this->indexTask.store(nextIdx, std::memory_order_release);
            co_return;
        }
        else {
            this->AsyncCommand("demo_pause");
            this->LogSucc(std::format("录制Pass已经完整结束：{}", curPass->demoFileName));
            this->currentPass.store(nullptr, std::memory_order_release);
            this->indexTask.store(-1, std::memory_order_release);
            curPass = nullptr;
            continue;
        }
    }
}
MulNX::CoTask DemoRecorder::Main() {
    while (true) {
        // 等待模块激活
        co_await this->WaitUntil([this]()->bool { return this->moduleActive.load(std::memory_order_acquire); });

        while (this->moduleActive.load(std::memory_order_acquire)) {
            const Demo::RecordTask* pTask = nullptr;
            co_await this->GoNextTask(pTask);
            if (!pTask) {
                this->moduleActive.store(false, std::memory_order_release);
                break;
            }
            const auto& rTask = *pTask;
            // 检验窗口安全
            if (rTask.tickStart < 151) {
                this->LogError(std::format("错误的录制窗口起点：{} ，已丢弃：{}", rTask.tickStart, rTask.desc));
                continue;
            }
            this->LogInfo("进行新片段录制");
            co_await this->RecordOne(rTask);
        }
    }
}
MulNX::CoTask DemoRecorder::RecordOne(const Demo::RecordTask& rTask) {
    this->LogInfo(std::format("汇报当前片段信息---UID:{},开始tick:{},结束tick:{}",
        rTask.uid, rTask.tickStart, rTask.tickEnd));

    // 首先暂停Demo
    if (!this->CS2Time->IsDemoPaused()) {
        this->LogInfo("当前Demo未暂停，尝试暂停");
        for (int i = 0;;++i) {
            this->AsyncCommand("demo_pause");
            bool ok;
            co_await this->WaitTimed(ok, 500, [&]() {
                return this->CS2Time->IsDemoPaused();
                });
            if (ok) {
                this->LogInfo("已经暂停，下一步");
                break;
            }
            else if (i < 3) {
                this->LogWarning("暂停失败，继续尝试暂停");
                continue;
            }
            else {
                this->LogError("多次暂停尝试失败，丢弃一个片段");
                co_return;
            }
        }
    }
    else {
        this->LogInfo("当前Demo已暂停，继续步骤");
    }
    // 验证音视频系统状态
    if (this->pMediaState->IsBusy()) {
        this->LogWarning("音视频系统忙碌，正在等待结束。如果在录制状态中，用户可能需要手动停止录制。");
        co_await this->WaitUntil([this]() {
            return !this->pMediaState->IsBusy();
            });
    }
    // 跳转时间点，以设置观战目标
    auto tick10before = rTask.tickStart - 10;
    this->AsyncCommand(std::format("demo_gototick {}", tick10before));
    // 等待跳转完成
    MulNX::Message* gotoCplt = nullptr;
    co_await this->WaitMsgForever("Demo/GotoTick/Complete"_hash, gotoCplt);
    auto&& [jmped] = gotoCplt->Access<int>();
    if (jmped != tick10before) {
        this->LogError(std::format("期望跳到tick:{} 而接收到了跳转到tick:{}，已丢弃此片段",
            tick10before, jmped));
        co_return;
    }

    this->AsyncCommandHighPriority("spec_mode 4");

    co_await this->WaitMilliseconds(750);

    // 跳转到稍微靠前的时间点，刷新雷达状态等等
    int beforeRecord = rTask.tickStart - 150;
    this->AsyncCommand(std::format("demo_gototick {}", beforeRecord));
    // 等待跳转完成
    MulNX::Message* gotoCplt2 = nullptr;
    co_await this->WaitMsgForever("Demo/GotoTick/Complete"_hash, gotoCplt2);
    auto&& [jmped2] = gotoCplt2->Access<int>();
    if (jmped2 != beforeRecord) {
        this->LogError(std::format("期望跳到tick:{}而接收到了跳转到tick:{}，已丢弃此片段",
            beforeRecord, jmped2));
        co_return;
    }

    co_await this->WaitMilliseconds(750);

    bool OBOK = false;
    co_await this->CoSetOBTarget(OBOK, rTask.uid);
    if (!OBOK) {
        this->LogError("多次尝试设置观战失败！丢弃一个片段");
        co_return;
    }

    this->LogInfo(std::format("已经设置观战目标：{}", rTask.uid));

    // 恢复Demo播放
    for (int i = 0;;++i) {
        bool ok = false;
        this->AsyncCommand("demo_resume");
        co_await this->WaitTimed(ok, 500, [&]() {
            // 这里直接验证有没有向下走，如果走了自然播放了
            return this->CS2Time->GetDemoTick() > beforeRecord;
            });
        if (ok) {
            break;
        }
        else if (i < 3) {
            this->LogWarning("正在等待回复播放");
            continue;
        }
        else {
            this->LogError("多次尝试恢复播放未成功，丢弃一个片段");
            co_return;
        }
    }

    // 等待播放到片段起点
    for (int i = 0;;++i) {
        bool ok = false;
        co_await this->WaitTimed(ok, 2500, [&]() {
            return this->CS2Time->GetDemoTick() > rTask.tickStart - 5 &&
                std::abs(rTask.tickStart - this->CS2Time->GetDemoTick()) <= 32;
            });
        if (ok) {
            break;
        }
        else if (i < 2) {
            this->LogWarning("正在等待播放到指定位置");
            continue;
        }
        else {
            this->LogError("多次等待播放到指定位置尝试均未成功，丢弃一个片段");
            co_return;
        }
    }

    // 开始录制
    this->StartRecord(rTask);
    // 输出提示信息
    this->LogSucc(std::format("已经请求录制，观战UID：{}，tick起点：{}，tick终点：{}",
        rTask.uid, rTask.tickStart, rTask.tickEnd));
    // 等待录制结束 tick
    co_await this->WaitUntil([&] {
        auto curTick = this->CS2Time->GetDemoTick();
        return curTick >= rTask.tickEnd;
        });
    // 停止录制
    this->PublishAsync("Media/Record/Stop"_hash);
    // 输出成功信息并重置状态
    this->LogSucc(std::format("录制结束，UID={}", rTask.uid));
}