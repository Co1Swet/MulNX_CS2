#include "HSI.hpp"
#include <Intro/HookGameEvents/HookGameEvents.hpp>

bool HSI::Init() {
    this->SubscribeSync("Hook/FireEventClientSide/player_death", [this](MulNX::Message& msg) {
        auto&& [pEvent] = msg.Access<CS2::CGameEvent*>();
        std::string test("游戏事件");
        this->WebPost(test);
        });

    return true;
}