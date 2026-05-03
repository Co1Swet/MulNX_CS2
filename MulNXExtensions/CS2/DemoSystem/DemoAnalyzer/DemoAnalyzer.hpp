#pragma once

#include <map>
#include <string>
#include <vector>
#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <MulNXExtensions/CS2/DeathMsgController/DeathMsgController.hpp>

class DemoAnalyzer final :public CSModuleBase {
    std::vector<KillEvent> bufferKillEvents;
    using KillEventTimeline = std::map<int, std::vector<KillEvent>>;
    using KillEventsByPlayer = std::map<Steam64UID, KillEventTimeline>;

    KillEventsByPlayer bufferPlayersKillInfo;
    std::string m_selectedAttackerIdRaw;

    void TransformKillEventsByAttacker();
    void PublishRecordWindows(uint64_t attackerId);
    bool Window(MulNX::UINode* node);
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
};