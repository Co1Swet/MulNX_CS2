#include "DemoRecorder.hpp"

void DemoRecorder::Window()const {
    auto w = MulNX::UI::RAIIWindow("录制执行");
    if (!w || !w.ShouldDraw()) return;
    std::shared_lock lock(this->smutex);

    // ---- 顶部操作区 ----
    if (ImGui::Button("启动")) {
        this->PublishAsync("Demo/Pass/Start"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button("停止")) {
        this->PublishAsync("Demo/Pass/Stop"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button("清空剩余任务")) {
        this->PublishAsync("Demo/Pass/Clear"_hash);
    }

    // ---- 输出路径设置 ----
    ImGui::TextUnformatted(
        std::format("当前输出文件夹：{}", (this->dirOutput / this->subOutput).string()).c_str()
    );
    static std::string buf;
    ImGui::InputText("新的输出子路径", &buf);
    if (ImGui::Button("更新子路径")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Pass/SetOutput"_hash);
        rp->str1 = buf;
        this->PublishAsync(std::move(msg));
    }

    auto curPass = this->currentPass.load(std::memory_order_acquire);
    const int curIdx = this->indexTask.load(std::memory_order_acquire);

    // ---- 待录制总时间：当前 pass 剩余 + 后续所有 pass ----
    int ticksToRecord = 0;
    if (curPass) {
        const int start = std::max(0, curIdx);
        for (int i = start; i < static_cast<int>(curPass->rTasks.size()); ++i) {
            const auto& t = curPass->rTasks[i];
            ticksToRecord += (t.tickEnd - t.tickStart);
        }
    }
    for (const auto& pass : this->recordPasses) {
        for (const auto& t : pass->rTasks) {
            ticksToRecord += (t.tickEnd - t.tickStart);
        }
    }
    ImGui::TextUnformatted("待录制时间：");
    ImGui::SameLine();
    MulNX::UI::ShowTime(ticksToRecord);

    // 通用：渲染一个 pass 内所有任务的列表
    auto DrawTasks = [](const Demo::RecordPass& pass, int highlightIdx) {
        for (int i = 0; i < static_cast<int>(pass.rTasks.size()); ++i) {
            const auto& t = pass.rTasks[i];

            // 主行：desc (+ 正在录制标记)
            std::string line = t.desc;
            if (i == highlightIdx) line += "    ---正在录制";
            ImGui::TextUnformatted(line.c_str());

            // 次行：UID / tick 范围（灰色小字，用 "%s" 防止格式串注入）
            std::string sub = std::format("   UID:{}   tick:{}-{}",
                t.uid, t.tickStart, t.tickEnd);
            ImGui::TextDisabled("%s", sub.c_str());
        }
        };

    // ---- 当前执行批次 ----
    ImGui::SeparatorText("当前执行批次");
    if (!curPass) {
        ImGui::TextUnformatted("当前无执行中录制批次");
    }
    else {
        std::string title = std::format("{}  ({} 个任务)",
            curPass->demoFileName.empty() ? "<未命名批次>" : curPass->demoFileName,
            curPass->rTasks.size());
        if (ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            DrawTasks(*curPass, curIdx);
        }
    }

    // ---- 所有待录制 pass ----
    ImGui::SeparatorText(
        std::format("待录制批次队列（{}）", this->recordPasses.size()).c_str()
    );
    if (this->recordPasses.empty()) {
        ImGui::TextUnformatted("（队列为空）");
        return;
    }
    for (const auto& pass : this->recordPasses) {
        std::string title = std::format("#{}  ({} 个任务)",
            pass->demoFileName.empty() ? "<未命名批次>" : pass->demoFileName,
            pass->rTasks.size());
        if (ImGui::CollapsingHeader(title.c_str())) {
            DrawTasks(*pass, /*highlightIdx=*/-1);
        }
    }
}

bool DemoRecorder::Init() {
    this->dirOutput = this->Path()->PathGetForShared("Output");
    (*this)
        .SubscribeAsync("Demo/Pass/Commit")
        .SubscribeAsync("Demo/Pass/Start")
        .SubscribeAsync("Demo/Pass/Clear")
        .SubscribeAsync("Demo/Pass/Stop")
        .SubscribeAsync("Demo/Pass/SetOutput")
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
    case "Demo/Pass/Commit"_hash: {
        std::unique_lock lock(this->smutex);
        auto pass = std::move(*msg.asp.get<Demo::RecordPass>());
        this->recordPasses.push_back(std::make_shared<Demo::RecordPass>(std::move(pass)));
        break;
    }
    case "Demo/Pass/Clear"_hash: {
        std::unique_lock lock(this->smutex);
        this->recordPasses.clear();
        break;
    }
    case "Demo/Pass/Start"_hash: {
        this->moduleActive.store(true, std::memory_order_release);
        this->LogInfo("Module activated.");
        break;
    }
    case "Demo/Pass/Stop"_hash: {
        this->moduleActive.store(false, std::memory_order_release);
        break;
    }
    case "Demo/Pass/SetOutput"_hash: {
        std::unique_lock lock(this->smutex);
        auto target = msg.asp.get<MulNX::NetExt>()->str1;
        if (target.empty()) {
            this->LogError("不能使用空子路径作为输出路径");
            return;
        }
        // 构造完整路径并确保文件夹存在
        auto fullPath = this->dirOutput / target;
        std::error_code ec;
        if (!fs::exists(fullPath, ec)) {
            if (!fs::create_directories(fullPath, ec)) {
                this->LogError(std::format("无法创建输出文件夹: {}，错误：{}", fullPath.string(), ec.message()));
                return;
            }
            this->LogInfo(std::format("已创建输出文件夹: {}" , fullPath.string()));
        }
        else if (!fs::is_directory(fullPath, ec)) {
            this->LogError(std::format("指定路径已存在但并非文件夹: {}", fullPath.string()));
            return;
        }
        this->subOutput = std::move(target);
        break;
    }
    }
}
void DemoRecorder::StartRecord(const Demo::RecordTask& task) {
    std::shared_lock lock(this->smutex);
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Media/Record/Start"_hash);
    ++this->num;
    // str1为绝对文件夹路径，str2为无扩展的文件名
    rp->str1 = (this->dirOutput / this->subOutput).string();
    if (task.fileNameNoExt.empty()) {
        rp->str2 = std::to_string(this->num);
    }
    else {
        rp->str2 = task.fileNameNoExt;
    }
    this->PublishAsync(std::move(msg));
}