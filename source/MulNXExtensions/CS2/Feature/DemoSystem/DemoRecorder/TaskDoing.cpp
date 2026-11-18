#include "DemoRecorder.hpp"
#include <Support/TimeController/TimeController.hpp>

MulNX::CoTask DemoRecorder::Main() {
    while (true) {
        co_await this->WaitUntil([&]()->bool { return this->moduleActive.load(std::memory_order_acquire); });
        while (this->moduleActive.load(std::memory_order_acquire)) {
            co_await this->OnLoop();
        }
    }
}
MulNX::CoTask DemoRecorder::OnLoop() {
    std::optional<std::pair<std::shared_ptr<const Demo::Record::Pass>, size_t>> o = std::nullopt;
    co_await this->GoNextTask(o);
    if (!o.has_value()) {
        this->LogWarning("没有更多的录制任务，模块将自动关闭");
        this->moduleActive.store(false, std::memory_order_release);
        co_return;
    }
    const auto& [pass, index] = *o;
    const auto& rTask = pass->rTasks.at(index);
    fs::path outputDir{};
    {
        std::shared_lock lock(this->smutex);
        outputDir = this->dirOutput / this->subOutput;
    }
    MulNX::Message msg("Demo/Record/Task/End"_hash);
    auto&& [recordSucc, pPass, iIndex, drop, pOutputDir] =
        msg.Access<bool, const Demo::Record::Pass*, size_t, bool, const fs::path*>();
    recordSucc = false;
    pPass = pass.get();
    iIndex = index;
    drop = false;
    pOutputDir = &outputDir;

    // 检验窗口安全
    if (rTask.tickStart < 151) {
        this->LogError(std::format("错误的录制窗口起点：{} ，已丢弃：{}", rTask.tickStart, rTask.desc));
        drop = true;
        this->PublishSync(msg);
        co_return;
    }
    this->LogInfo("进行新片段录制");

    try {
        co_await this->RecordOne(rTask, recordSucc, outputDir);
    }
    catch (const std::exception& e) {
        this->LogError(std::format("录制过程中出现异常 {} ，已丢弃一个片段：{}", e.what(), rTask.desc));
    }
    this->PublishSync(msg);
}

MulNX::CoTask DemoRecorder::RecordOne(const Demo::Record::Task& rTask, bool& recordSucc, const fs::path& outputDir)const {
    this->AsyncCommand("spec_mode 4");
    bool mode4 = false;
    co_await this->WaitTimed(mode4, 3000, [&]() {
        auto o = this->CS2Entitys->TryGetObservingSteam64UID();
        if (o.has_value())return false;
        return true;
        });
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

    auto now = std::chrono::steady_clock::now();
    // 验证音视频系统状态，让这个验证和等待共享等待时间，最少总计两秒
    if (this->pMediaState->IsBusy()) {
        this->LogWarning("音视频系统忙碌，正在等待结束。如果在录制状态中，用户可能需要手动停止录制。");
        co_await this->WaitUntil([&]() {
            return !this->pMediaState->IsBusy();
            });
    }
    co_await this->WaitUntil([&]() {
        auto elapsed = std::chrono::steady_clock::now() - now;
        return elapsed >= std::chrono::seconds(2);
        });

    // 预备进入
    bool preEnterOK = false;
    co_await this->PreEnter(preEnterOK, rTask);
    if (!preEnterOK) {
        this->LogError(std::format("录制预准备失败，丢弃片段：{}", rTask.desc));
        co_return;
    }

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
    // 预留录制前钩子
    this->PublishSync("Demo/Record/PreRecord"_hash);
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
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Media/Record/Start"_hash);
    // str1为绝对文件夹路径，str2为无扩展的文件名
    rp->str1 = outputDir.string();
    rp->str2 = rTask.fileNameNoExt;
    this->PublishAsync(std::move(msg));
    auto StopRecord = scope_exit([&]() {
        // 停止录制
        this->PublishAsync("Media/Record/Stop"_hash);
        if (recordSucc) {
            this->LogSucc(std::format("{} 录制正常结束", rTask.desc));
        }
        else {
            this->LogError(std::format("{} 录制异常结束", rTask.desc));
        }
        });
    // 输出提示信息
    this->LogSucc(std::format("已经请求录制，{}", rTask.desc));
    // 等待录制结束
    co_await this->OnRecording(rTask, recordSucc);
}

MulNX::CoTask DemoRecorder::PreEnter(bool& ok, const Demo::Record::Task& rTask)const {
    if (rTask.IsPlayerType()) {
        co_await this->PreEnterPlayer(ok, rTask);
    }
    else if (rTask.type == Demo::Record::Task::Type::Campath) {
        co_return;
    }
    co_return;
}
MulNX::CoTask DemoRecorder::OnRecording(const Demo::Record::Task& curRecordTask, bool& recordSucc)const {
    if (curRecordTask.IsPlayerType()) {
        co_await this->OnRecordingPlayer(curRecordTask, recordSucc);
    }
    else if (curRecordTask.type == Demo::Record::Task::Type::Campath) {
        co_return;
    }
    co_return;
}
bool DemoRecorder::BaseRecordingCheck(const Demo::Record::Task& curRecordTask, bool& recordSucc)const {
    if (this->moduleActive.load(std::memory_order_acquire) == false) {
        this->LogWarning("模块已被外部关闭，停止录制");
        return false;
    }

    const int curTick = this->CS2Time->GetDemoTick();
    if (curTick < 5) {
        this->LogWarning(std::format("当前tick={}，意外地小于5，继续等待", curTick));
        return true;
    }
    if (curTick >= curRecordTask.tickEnd) {
        this->LogInfo(std::format("当前tick={}，已经超过录制终点tick={}，结束录制",
            curTick, curRecordTask.tickEnd));
        recordSucc = true;
        return false;
    }
    if (curTick < curRecordTask.tickStart - 5) {
        this->LogError(std::format("当前tick={}，意外地小于录制起点tick={}，丢弃录制",
            curTick, curRecordTask.tickStart));
        return false;
    }
    if (this->CS2Time->IsDemoPaused()) {
        auto endTick = this->CS2Time->GetDemoLastTick();
        if (std::abs(curTick - endTick) < 100) {
            this->LogInfo(std::format("当前Demo在Demo尾tick {} 附近 {} 暂停，完成此片段录制", endTick, curTick));
            recordSucc = true;
            return false;
        }
        else {
            this->LogError(std::format("当前Demo {} 暂停，注意Demo尾tick为 {}，终止此片段录制", curTick, endTick));
            return false;
        }
    }

    return true;
}