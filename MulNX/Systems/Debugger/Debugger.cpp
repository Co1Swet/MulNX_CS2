#include "Debugger.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>
#include <MulNX/Systems/Logger/Logger.hpp>
#include <MulNX/Systems/I18nManager/I18nManager.hpp>

bool MulNX::Debugger::Window(MulNX::UINode* ThisNode) {
    auto w = MulNX::UI::RAIIWindow("调试器", this->showWindow);
    if (!w)return true;
    std::shared_lock lock(this->smutex);

    // 在标签页内创建一个子窗口
    ImVec2 childSize = ImGui::GetContentRegionAvail();
    childSize.y -= ImGui::GetStyle().ItemSpacing.y; // 留出一点空间

    // 开始子窗口，占据标签页的剩余空间
    ImGui::BeginChild("信息", childSize, true, ImGuiWindowFlags_HorizontalScrollbar);

    // 使用虚拟列表优化性能
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(this->DebugMsg.size()));
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            const auto& msg = this->DebugMsg[i];

            // 根据消息类型着色
            if (msg.find(this->kInfo) != std::string::npos) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(50, 50, 255, 255));
            }
            else if (msg.find(this->kSucc) != std::string::npos) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 100, 0, 255));
            }
            else if (msg.find(this->kWarning) != std::string::npos) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 0, 255));
            }
            else if (msg.find(this->kError) != std::string::npos) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 50, 50, 255));
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
            }

            ImGui::TextUnformatted(msg.c_str());

            // 弹出
            ImGui::PopStyleColor();
        }
    }

    // 自动滚动到最新消息
    if (this->NeedAutoScroll) {
        ImGui::SetScrollHereY(1.0f);
        this->NeedAutoScroll = false;
    }

    // 结束子窗口
    ImGui::EndChild();
    return true;
}

bool MulNX::Debugger::Init() {
    this->pLogger = this->Core->ModuleManager()->FindModule<MulNX::Logger>("Logger");

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});

    this->ISys().SendTask("Main", "MulNXMain", [this]()->bool {
        this->Main();
        return true;
        });

    this->ISys()
        .SubscribeAsync("Log/Info")
        .SubscribeAsync("Log/Succ")
        .SubscribeAsync("Log/Warning")
        .SubscribeAsync("Log/Error")
        .SubscribeAsync("Debugger/SetMaxInfoCount")
        ;

    this->kInfo = I18n("log.info");
    this->kSucc = I18n("log.succ");
    this->kWarning = I18n("log.warning");
    this->kError = I18n("log.error");

    return true;
}
void MulNX::Debugger::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Debugger/SetMaxInfoCount"_hash: {
        this->ResetMaxMsgCount(msg.p1.low<float>());
        break;
    }
    case "Log/Info"_hash: {
        MulNX::NetExt ext = std::move(*msg.asp.get<MulNX::NetExt>());
        this->PushBack(std::move(ext), this->kInfo);
        break;
    }
    case "Log/Succ"_hash: {
        MulNX::NetExt ext = std::move(*msg.asp.get<MulNX::NetExt>());
        this->PushBack(std::move(ext), this->kSucc);
        break;
    }
    case "Log/Warning"_hash: {
        MulNX::NetExt ext = std::move(*msg.asp.get<MulNX::NetExt>());
        this->PushBack(std::move(ext), this->kWarning);
        break;
    }
    case "Log/Error"_hash: {
        MulNX::NetExt ext = std::move(*msg.asp.get<MulNX::NetExt>());
        if (this->ShowWhenError) {
            this->showWindow = true;
            this->IfShowStream = true;
        }
        this->PushBack(std::move(ext), this->kError);
        break;
    }
    }
}
void MulNX::Debugger::Main() {
    this->Update();
}

void MulNX::Debugger::ResetMaxMsgCount(const int Max) {
    std::unique_lock lock(this->smutex);
    if (Max < 1) {
        //this->AddError("最大信息条数不能小于一1!");
        return;
    }
    if (DebugMsg.size() > Max) {
        DebugMsg.erase(DebugMsg.begin(), DebugMsg.end() - Max);
    }
    this->MaxMsgCount = Max;
    lock.unlock();
    //this->AddInfo("已重置最大信息条数为 " + std::to_string(Max) + " 条");

    return;
}

void MulNX::Debugger::PushBack(MulNX::NetExt&& pack, const std::string& strLevel) {
    // 调用者已加锁，此处无锁

    // 取出结构化字段
    std::string moduleName = std::move(pack.str1);
    std::string rawMsg = std::move(pack.str2);
    auto eventTime = MulNX::FromUnixUs(pack.timestamp_us);

    // 获取整行格式模板
    const std::string& lineFmt = I18n("log.fmt");
    // 例："[{}] [{}] [{}] {}"  占位顺序：时间、级别、模块、消息

    // 按换行符拆分消息体
    std::istringstream iss(rawMsg);
    std::string line;
    bool first = true;
    while (std::getline(iss, line)) {
        // 清理行尾回车
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) continue;  // 跳过纯空行

        // 用整行模板拼接出最终日志
        std::string formatted = std::vformat(lineFmt, std::make_format_args(
            eventTime,      // {} 对应时间
            strLevel,     // {} 对应级别
            moduleName,   // {} 对应模块
            line          // {} 对应本行消息
        ));

        this->pLogger->logs.enqueue(formatted);
        // 存入双端队列（保持容量限制）
        std::unique_lock lock(this->smutex);
        if (this->DebugMsg.size() == this->MaxMsgCount) {
            this->DebugMsg.pop_front();
        }
        this->DebugMsg.push_back(std::move(formatted));
        first = false;
    }

    // 自动滚动标记
    if (this->AutoScroll) {
        this->NeedAutoScroll = true;
    }
}

void MulNX::Debugger::ShowStream() {
    this->IfShowStream = true;
}
void MulNX::Debugger::HideStream() {
    this->IfShowStream = false;
}