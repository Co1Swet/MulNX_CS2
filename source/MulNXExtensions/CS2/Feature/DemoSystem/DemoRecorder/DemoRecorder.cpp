#include "DemoRecorder.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/HookConsole/HookConsole.hpp>
#include <Buildup/TimeController/TimeController.hpp>
#include <Feature/DemoSystem/DemoJSONReader/DemoJSONReader.hpp>

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

    this->SendTask("Update", "DemoSys", [this]()->bool {
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

MulNX::CoTask DemoRecorder::Main() {
    while (true) {
        // 等待模块激活
        co_await this->WaitUntil([this]()->bool { return this->moduleActive.load(std::memory_order_acquire); });

        // 等待队列中有任务
        RecordTask task;
        co_await this->WaitUntil([&]()->bool { return this->PeekQueue(task); });

        this->currentRecordTaskStartTick = task.tickStart;
        this->currentRecordTaskEndTick = task.tickEnd;
        auto uid = task.uid;
        this->currentRecordTask = std::move(task);

        if (this->currentRecordTaskStartTick < 0) {
            this->LogWarning("Window start tick adjusted from "
                + std::to_string(this->currentRecordTaskStartTick) + " to 0.");
            this->currentRecordTaskStartTick = 0;
        }

        // 暂停并跳转
        this->AsyncCommand(std::format("demo_gototick {}", this->currentRecordTaskStartTick - 64));
        
        // 等待跳转完成
        MulNX::Message* gotoCplt = nullptr;
        co_await this->WaitMsgForever("Demo/GotoTick/Complete"_hash, gotoCplt);
        auto&& [jmped] = gotoCplt->Access<int>();
        if (jmped != this->currentRecordTaskStartTick - 64) {
            this->LogError(std::format("期望跳到tick:{}而接收到了跳转到tick:{}，已丢弃此片段",
                this->currentRecordTaskStartTick - 64, jmped));
            continue;
        }

        // 等待加载
        co_await this->WaitUntil([this] {
            return this->CS2Time->GetDemoTick() >= this->currentRecordTaskStartTick;
            });

        // 设置观察目标
        MulNX::Message specMsg("Observe/SpecSteam64UID"_hash);
        auto&& [uidRef] = specMsg.Access<Steam64UID>();
        uidRef = uid;
        this->PublishAsync(std::move(specMsg));

        this->LogInfo("Jumped to tick " + std::to_string(this->currentRecordTaskStartTick)
            + ", observing UID=" + std::to_string(uid));

        // 开始录制
        this->StartRecord();

        this->LogSucc("Recording started for UID="
            + std::to_string(uid)
            + " from tick " + std::to_string(this->currentRecordTaskStartTick)
            + " to " + std::to_string(this->currentRecordTaskEndTick));

        // 等待录制结束 tick
        co_await this->WaitUntil([this, &task] {
            auto curTick = this->CS2Time->GetDemoTick();
            if (task.onPlaying) {
                task.onPlaying(curTick, &task);
            }
            return curTick >= this->currentRecordTaskEndTick;
            });

        // 停止录制
        this->PublishAsync("Media/Record/Stop"_hash);

        this->LogSucc(std::format("Recording finished for UID={}", uid));
        this->currentRecordTask.reset();
    }
    co_return;
}