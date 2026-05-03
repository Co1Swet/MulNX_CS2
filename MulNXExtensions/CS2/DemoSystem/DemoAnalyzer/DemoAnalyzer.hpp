#pragma once

#include <map>
#include <vector>
#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <MulNXExtensions/CS2/DeathMsgController/DeathMsgController.hpp>

class DemoAnalyzer final :public CSModuleBase {
    std::vector<KillEvent> m_killEvents;
    using KillEventTimeline = std::map<int, std::vector<KillEvent>>;
    using KillEventsByPlayer = std::map<Steam64UID, KillEventTimeline>;

    KillEventsByPlayer m_infos;

    void TransformKillEventsByAttacker();
    bool Window(MulNX::UINode* node);
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
    const KillEventsByPlayer& GetInfos() const { return m_infos; }
};