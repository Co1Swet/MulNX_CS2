#include "Debugger.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>
#include <MulNX/Systems/Logger/Logger.hpp>
#include <MulNX/Systems/I18nManager/I18nManager.hpp>
#include <MulNX/Systems/MessageManager/MessageManager.hpp>

void MulNX::Debugger::DeMe() {
    MulNX::UI::Checkbox("调试器窗口", this->showWindow);
    ImGui::Checkbox("当有错误信息时弹出调试器", &this->showWhenError);
    ImGui::Checkbox("自动滚动到最新消息", &this->autoScroll);
    static int MaxDebugMsgs = 1000;
    ImGui::Text("设置最大消息数量（至少100）:");
    ImGui::SameLine();
    ImGui::InputInt("##最大消息数量", &MaxDebugMsgs);
    ImGui::SameLine();
    if (ImGui::Button("应用")) {
        MulNX::Message msg("Debugger/SetMaxInfoCount"_hash);
        auto&& [access] = msg.Access<int>();
        access = MaxDebugMsgs;
        this->PublishAsync(std::move(msg));
    }
}

void MulNX::Debugger::Window() {
    auto w = MulNX::UI::RAIIWindow("调试器", this->showWindow);
    if (!w || !w.ShouldDraw())return;
    std::shared_lock lock(this->smutex);

    // 在标签页内创建一个子窗口
    ImVec2 childSize = ImGui::GetContentRegionAvail();
    childSize.y -= ImGui::GetStyle().ItemSpacing.y; // 留出一点空间

    // 开始子窗口，占据标签页的剩余空间
    auto c = MulNX::UI::RAIIChild("信息", childSize, true, ImGuiWindowFlags_HorizontalScrollbar);

    // 使用虚拟列表优化性能
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(this->debugMsg.size()));
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            const auto& msg = this->debugMsg[i];

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
    if (this->needAutoScroll) {
        ImGui::SetScrollHereY(1.0f);
        this->needAutoScroll = false;
    }

    return;
}

bool MulNX::Debugger::Init() {
    this->SendUIRoot(this->GetName(), [this](auto&&...) {return this->Window();});
    this->UIRegisterCallback("UI.MulNXControl", [this](auto&&...) {return this->DeMe();});

    this->SendTask("Main", "MulNXMain", [this]()->bool {
        this->Main();
        return true;
        });

    (*this)
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
        auto&& [count] = msg.Access<int>();
        this->ResetMaxMsgCount(count);
        break;
    }
    case "Log/Error"_hash: {
        if (this->showWhenError)
            this->showWindow = true;
    };
    case "Log/Info"_hash:
    case "Log/Succ"_hash:
    case "Log/Warning"_hash: {
        if (this->autoScroll)
            this->needAutoScroll = true;
        auto fmtted = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        this->debugMsg.push_back(fmtted);
        break;
    }

    }
}
void MulNX::Debugger::Main() {
    this->Update();
}

void MulNX::Debugger::ResetMaxMsgCount(const int max) {
    std::unique_lock lock(this->smutex);
    if (max < 1) {
        this->LogError("最大信息条数不能小于一1!");
        return;
    }
    if (this->debugMsg.size() > max) {
        this->debugMsg.erase(this->debugMsg.begin(), this->debugMsg.end() - max);
    }
    this->maxMsgCount = max;
    lock.unlock();
    this->LogInfo(std::format("已重置最大信息条数为 {} 条", max));
    return;
}