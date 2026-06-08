#pragma once
#include <MulNXExtensions/CS2/PlayerHub/CSViewPlayerModuleBase.hpp>

class SpeakingController :public CSViewPlayerModuleBase {
    int* tv_listen_voice_indices = nullptr;
    void Main();
public:
    bool Init()override;
};