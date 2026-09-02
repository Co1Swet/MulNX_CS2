#include "MediaRunningState.hpp"

bool MediaRunningState::Init() {
    (*this)
        .SubscribeAsync("Media/Record/Stop");

    this->SendTask("Update", "MediaState", [this](auto&&...) {
        this->Update();
        return true;
        });

    return true;
}
void MediaRunningState::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Media/Record/Stop"_hash:
        this->MediaSystemGlobalWorkFlag = false;
        this->PublishSync("Media/Record/Stop/FastNotify"_hash);
        break;
    }
}