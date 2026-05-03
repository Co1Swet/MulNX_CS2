#include "DemoAnalyzer.hpp"
#include <MulNX/Base/UI/UI.hpp>

void DemoAnalyzer::TransformKillEventsByAttacker() {
    this->m_infos.clear();
    for (const auto& ev : this->m_killEvents) {
        this->m_infos[ev.attackerSteamId][ev.DemoTick].push_back(ev);
    }
}

bool DemoAnalyzer::Window(MulNX::UINode* node) {
    auto c = MulNX::UI::RAIIChild("Demo分析");
    if (!c)return true;
    if (ImGui::Button(I18n("demo.analyze.dump").c_str())) {
        this->ISys().PublishAsync("Demo/Analyze/Dump"_hash);
    }

    return true;
}

bool DemoAnalyzer::Init() {

    this->ISys()
        .SubscribeAsync("Demo/Analyze/Restart")
        .SubscribeAsync("Demo/Analyze/Dump")
        .SubscribeAsync("Game/KillEvent");

    this->SendTask("MulNXMain", [this]()->bool {
        this->EntryProcessMsg();
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
        this->m_killEvents.clear();
        this->m_infos.clear();
        break;
    }
    case "Demo/Analyze/Dump"_hash: {
        this->TransformKillEventsByAttacker();
        break;
    }
    case "Game/KillEvent"_hash: {
        auto pKillEvent = msg.asp.get<KillEvent>();
        this->m_killEvents.push_back(*pKillEvent);
        break;
    }
    }
}
