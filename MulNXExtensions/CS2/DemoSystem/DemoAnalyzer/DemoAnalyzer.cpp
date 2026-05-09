#include "DemoAnalyzer.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <format>

bool DemoAnalyzer::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("Demo分析");
    if (ImGui::Button(I18n("demo.analyze.dump").c_str())) {
        this->ISys().PublishAsync("Demo/Analyze/Dump"_hash);
    }

    ImGui::InputText("Attacker ID", &this->m_selectedAttackerIdRaw);
    ImGui::SameLine();
    if (ImGui::Button("Record by Attacker ID")) {
        try {
            uint64_t attackerId = std::stoull(this->m_selectedAttackerIdRaw, nullptr, 0);
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Analyze/Record"_hash);
            msg.p1.as<uint64_t>() = attackerId;
            this->ISys().PublishAsync(std::move(msg));
        }
        catch (const std::exception&) {
            this->ISys().LogWarning("无效的攻击者 ID，请输入十进制或 0x 十六进制数值。");
        }
    }

    if (!this->bufferPlayersKillInfo.empty()) {
        ImGui::Text("记录的攻击者数量：%zu", this->bufferPlayersKillInfo.size());
    }

    return true;
}

bool DemoAnalyzer::Init() {
    this->ISys()
        .SubscribeAsync("Demo/Analyze/Restart")
        .SubscribeAsync("Demo/Analyze/Dump")
        .SubscribeAsync("Demo/Analyze/Record")
        .SubscribeAsync("Game/KillEvent");

    this->SendTask("MulNXMain", [this]()->bool {
        this->Update();
        return true;
        });

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {
        this->Window(node);
        });

    return true;
}

void DemoAnalyzer::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Analyze/Restart"_hash: {
        this->bufferKillEvents.clear();
        this->bufferPlayersKillInfo.clear();
        this->ISys().PublishAsync("Demo/Record/Reset"_hash);
        break;
    }
    case "Demo/Analyze/Dump"_hash: {
        this->TransformKillEventsByAttacker();
        break;
    }
    case "Demo/Analyze/Record"_hash: {
        uint64_t attackerId = msg.p1.as<uint64_t>();
        this->PublishRecordWindows(attackerId);
        break;
    }
    case "Game/KillEvent"_hash: {
        auto pKillEvent = msg.asp.get<KillEvent>();
        this->bufferKillEvents.push_back(*pKillEvent);
        break;
    }
    }
}

void DemoAnalyzer::TransformKillEventsByAttacker() {
    this->bufferPlayersKillInfo.clear();
    for (const auto& ev : this->bufferKillEvents) {
        this->bufferPlayersKillInfo[ev.attackerSteamId][ev.DemoTick].push_back(ev);
    }
}

void DemoAnalyzer::PublishRecordWindows(uint64_t attackerId) {
    auto it = this->bufferPlayersKillInfo.find(attackerId);
    if (it == this->bufferPlayersKillInfo.end()) {
        this->ISys().LogError(std::format("攻击者 {} 未找到，请先执行 Dump。", attackerId));
        return;
    }

    for (const auto& [tick, events] : it->second) {

        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Record/Enqueue"_hash);
        msg.p1.as<Steam64UID>() = it->first;
        msg.p2.low<int>() = tick;
        this->ISys().PublishAsync(std::move(msg));
    }
}