#include "SpeakingController.hpp"
#include <MulNXExtensions/CS2/HookConsole/HookConsole.hpp>

bool SpeakingController::Init() {
    this->ISys().SubscribeSync("Hook/Source2Client002::Inited", [this](MulNX::Message& msg) {
        this->tv_listen_voice_indices = this->CS2Con->GetCvar("tv_listen_voice_indices")->GetPtr<int>();
        return;
        });

    this->ISys().SendTask("Main", "CSControl", [this]() {

        return true;
        });

    return true;
}

void SpeakingController::Main() {
    
}