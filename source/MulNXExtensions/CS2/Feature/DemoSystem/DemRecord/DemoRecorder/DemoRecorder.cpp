#include "DemoRecorder.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Buildup/TimeController/TimeController.hpp>

void DemoRecorder::Window() {
    auto w = MulNX::UI::RAIIWindow("Demo Recorder");
    if (!w || !w.ShouldDraw())return;
    std::shared_lock lock(this->smutex);

    if (ImGui::Button("启动")) {
        this->PublishAsync("Demo/Record/Start"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button("清空队列")) {
        this->PublishAsync("Demo/Record/Clear"_hash);
    }
    ImGui::Text(
        std::format("当前输出文件夹：{}", (this->dirOutput / this->subOutput).string()).c_str()
    );
    static std::string buf;
    ImGui::InputText("新的输出子路径", &buf);
    if (ImGui::Button("更新子路径")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Record/SetOutput"_hash);
        rp->str1 = buf;
        this->PublishAsync(std::move(msg));
    }
    ImGui::Text(std::format("当前索引：{}", this->num.load()).c_str());

    int ticksToRecord = 0;

    for (const auto& recordTask : this->recordTaskBufferQueue) {
        ticksToRecord += (recordTask.tickEnd - recordTask.tickStart);
    }

    ImGui::Text("待录制时间：");
    MulNX::UI::ShowTime(ticksToRecord);
    ImGui::SeparatorText("当前队列");

    for (const auto& recordTask : this->recordTaskBufferQueue) {
        ImGui::Text(recordTask.desc.c_str());
    }
}

bool DemoRecorder::Init() {
    this->dirOutput = this->Path()->PathGetForShared("Output");
    (*this)
        .SubscribeAsync("Demo/Record/Enqueue")
        .SubscribeAsync("Demo/Record/Start")
        .SubscribeAsync("Demo/Record/Clear")
        .SubscribeAsync("Demo/Record/Stop")
        .SubscribeAsync("Demo/Record/SetOutput")
        .SubscribeAsync("Demo/GotoTick/Complete")
        ;

    this->Main().Fire();

    this->SubscribeSync("Hook/CSMainLoop", [this](auto&&...)->bool {
        this->Update();
        return true;
        });

    this->UIRegisterCallback("UI.Demos", [this](auto&&...) {
        return this->Window();
        });

    return true;
}

void DemoRecorder::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Record/Enqueue"_hash: {
        std::unique_lock lock(this->smutex);
        auto task = std::move(*msg.asp.get<RecordTask>());
        this->recordTaskBufferQueue.push_back(std::move(task));
        break;
    }
    case "Demo/Record/Clear"_hash: {
        std::unique_lock lock(this->smutex);
        this->recordTaskBufferQueue.clear();
        break;
    }
    case "Demo/Record/Start"_hash: {
        this->moduleActive.store(true, std::memory_order_release);
        this->LogInfo("Module activated.");
        break;
    }
    case "Demo/Record/Stop"_hash: {
        this->moduleActive.store(false, std::memory_order_release);
        break;
    }
    case "Demo/Record/SetOutput"_hash: {
        std::unique_lock lock(this->smutex);
        auto target = msg.asp.get<MulNX::NetExt>()->str1;
        if (target.empty()) {
            this->LogError("不能使用空子路径作为输出路径");
            return;
        }

        // 构造完整路径并确保文件夹存在
        auto fullPath = this->dirOutput / target;
        std::error_code ec;
        if (!std::filesystem::exists(fullPath, ec)) {
            if (!std::filesystem::create_directories(fullPath, ec)) {
                this->LogError("无法创建输出文件夹: " + fullPath.string() + " - " + ec.message());
                return;
            }
            this->LogInfo("已创建输出文件夹: " + fullPath.string());
        }
        else if (!std::filesystem::is_directory(fullPath, ec)) {
            this->LogError("指定路径已存在但并非文件夹: " + fullPath.string());
            return;
        }

        this->subOutput = std::move(target);
        break;
    }
    default:
        break;
    }
}

bool DemoRecorder::PeekQueue(RecordTask& task) {
    std::unique_lock lock(this->smutex);
    if (this->recordTaskBufferQueue.empty()) return false;
    task = std::move(this->recordTaskBufferQueue.front());
    this->recordTaskBufferQueue.pop_front();
    if (this->recordTaskBufferQueue.empty()) {
        this->moduleActive.store(false, std::memory_order_release);
    }
    return true;
}
void DemoRecorder::StartRecord() {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Media/Record/Start"_hash);
    ++this->num;
    rp->str1 = (this->dirOutput / this->subOutput / std::to_string(this->num)).string();
    this->PublishAsync(std::move(msg));
}

MulNX::CoTask DemoRecorder::WaitTimed(bool& flag, const float milliseconds, const std::function<bool()>& f) {
    auto start = std::chrono::steady_clock::now();
    auto timeout = std::chrono::duration<float, std::milli>(milliseconds);
    co_await this->WaitUntil([&]() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(now - start);
        if (f()) {
            flag = true;
            return true;
        }
        if (elapsed >= timeout) {
            flag = false;
            return true;
        }
        return false;
        });
}

MulNX::CoTask DemoRecorder::Main() {
    while (true) {
        // 等待模块激活
        co_await this->WaitUntil([this]()->bool { return this->moduleActive.load(std::memory_order_acquire); });

        // 等待队列中有任务
        RecordTask rTask;
        co_await this->WaitUntil([&]()->bool { return this->PeekQueue(rTask); });
        this->LogInfo("进行新片段录制");

        // 检验窗口安全
        if (rTask.tickStart < 1) {
            this->LogWarning(std::format("录制窗口起点从 {} 修正至1", rTask.tickStart));
            rTask.tickStart = 1;
        }

        // 首先暂停Demo
        if (!this->CS2Time->IsDemoPaused()) {
            this->LogInfo("当前Demo未暂停，尝试暂停");
            for (int i = 0;;++i) {
                this->AsyncCommand("demo_pause");
                bool ok;
                co_await this->WaitTimed(ok, 500.0f, [&]() {
                    return this->CS2Time->IsDemoPaused();
                    });
                if (ok) {
                    this->LogInfo("已经暂停，下一步");
                    break;
                }
                else if (i < 3) {
                    this->LogWarning("暂停失败，继续尝试暂停");
                    ++i;
                    continue;
                }
                else {
                    this->LogError("多次暂停尝试失败");
                    break;
                }
            }
        }
        else {
            this->LogInfo("当前Demo已暂停，继续步骤");
        }
        // 验证暂停状态
        if (!this->CS2Time->IsDemoPaused()) {
            this->LogError("当前Demo不处于暂停状态，已丢弃一个片段");
            continue;
        }
        // 验证音视频系统状态
        if (this->pMediaState->MediaSystemGlobalWorkFlag || this->pMediaState->recordState == RecordState::Recording) {
            this->LogError("音视频系统忙碌，正在等待结束。用户可能需要手动停止录制，如果在录制状态中。");
            co_await this->WaitUntil([this]() {
                return this->pMediaState->MediaSystemGlobalWorkFlag == false && this->pMediaState->recordState == RecordState::Free;
                });
        }
        // 跳转到稍微靠后的时间点，以设置观战目标
        auto backTick = rTask.tickStart + 1;
        this->AsyncCommand(std::format("demo_gototick {}", backTick));
        // 等待跳转完成
        MulNX::Message* gotoCplt = nullptr;
        co_await this->WaitMsgForever("Demo/GotoTick/Complete"_hash, gotoCplt);
        auto&& [jmped] = gotoCplt->Access<int>();
        if (jmped != backTick) {
            this->LogError(std::format("期望跳到tick:{}而接收到了跳转到tick:{}，已丢弃此片段",
                backTick, jmped));
            continue;
        }
        // 设置观战目标
        for (int i = 0;;++i) {
            auto obUID = this->CS2Entitys->TryGetObservingSteam64UID();
            if (!obUID || (obUID.value() != rTask.uid)) {
                // 发送设置观察目标消息
                MulNX::Message specMsg("Observe/SpecSteam64UID"_hash);
                auto&& [uidRef] = specMsg.Access<Steam64UID>();
                uidRef = rTask.uid;
                this->PublishAsync(std::move(specMsg));
            }
            bool ok = false;
            co_await this->WaitTimed(ok, 500.0f, [&]() {
                auto obUID = this->CS2Entitys->TryGetObservingSteam64UID();
                if (!obUID || (obUID.value() != rTask.uid))return false;
                return true;
                });
            if (ok) {
                break;
            }
            else if (i < 3) {
                this->LogWarning("观战设置失败，重新尝试");
                ++i;
                continue;
            }
            else {
                this->LogError("多次尝试设置观战失败！");
                break;
            }
        }
        // 验证观战目标
        auto obUID = this->CS2Entitys->TryGetObservingSteam64UID();
        if (!obUID || (obUID.value() != rTask.uid)) {
            this->LogError("由于观战设置尝试失败，丢弃一个片段");
            continue;
        }
        this->LogInfo(std::format("已经设置观战目标：{}", rTask.uid));

        // 跳转到稍微靠前的时间点，刷新雷达状态等等
        int beforeRecord = rTask.tickStart - 100;
        this->AsyncCommand(std::format("demo_gototick {}", beforeRecord));
        // 等待跳转完成
        MulNX::Message* gotoCplt2 = nullptr;
        co_await this->WaitMsgForever("Demo/GotoTick/Complete"_hash, gotoCplt2);
        auto&& [jmped2] = gotoCplt2->Access<int>();
        if (jmped2 != beforeRecord) {
            this->LogError(std::format("期望跳到tick:{}而接收到了跳转到tick:{}，已丢弃此片段",
                beforeRecord, jmped2));
            continue;
        }
        // 恢复Demo播放
        for (int i = 0;;++i) {
            bool ok = false;
            this->AsyncCommand("demo_resume");
            co_await this->WaitTimed(ok, 500.0f, [&]() {
                // 这里直接验证有没有向下走，如果走了自然播放了
                return this->CS2Time->GetDemoTick() >= beforeRecord - 5;
                });
            if (ok) {
                break;
            }
            else if (i < 3) {
                ++i;
                this->LogWarning("正在等待回复播放");
                continue;
            }
            else {
                this->LogError("多次尝试恢复播放未成功");
                break;
            }
        }
        // 验证一下是不是确实走了
        if (this->CS2Time->GetDemoTick() < beforeRecord - 5) {
            this->LogError("播放未成功，已丢弃一个片段");
            continue;
        }
        // 开始录制
        this->StartRecord();
        // 输出提示信息
        this->LogInfo(std::format("当前tick：{} ，正在观战目标UID:{}",
            rTask.tickStart, rTask.uid));
        this->LogSucc(std::format("已经请求录制，观战UID：{}，tick起点：{}，tick终点：{}",
            rTask.uid, rTask.tickStart, rTask.tickEnd));
        // 等待录制结束 tick
        co_await this->WaitUntil([&] {
            auto curTick = this->CS2Time->GetDemoTick();
            if (rTask.onPlaying) {
                rTask.onPlaying(curTick, &rTask);
            }
            return curTick >= rTask.tickEnd;
            });
        // 停止录制
        this->PublishAsync("Media/Record/Stop"_hash);
        // 输出成功信息并重置状态
        this->LogSucc(std::format("录制结束，UID={}", rTask.uid));
        continue;
    }
    co_return;
}