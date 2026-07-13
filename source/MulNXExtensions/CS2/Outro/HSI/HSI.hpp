#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/WebSocketManager/WebSocketMixin.hpp>

namespace CS2 {
    class CGameEvent;
}
class HSI final :public CSModuleBase, public WebSocketMixin<HSI> {
    struct KillEvent {
        Steam64UID attacker;
        Steam64UID victim;
        Steam64UID assister;

        int assistedflash;
        int headshot;
        int penetrated;
        int noscope;
        int thrusmoke;
        int attackerblind;
        int attackerinair;
        std::string weapon;  // weapon 是字符串，单独处理
    };
    moodycamel::ConcurrentQueue<KillEvent>buffer{};
    bool Init()override;
    void OnDeathEvent(CS2::CGameEvent*);
    void Main();
};