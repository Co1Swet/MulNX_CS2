#include "AVCodecManager.hpp"

bool AVCodecManager::Init() {

    return true;
}

void AVCodecManager::AddEncoder(MulNX::IVEncoder* pEncoder) {
    this->encoders.push_back(pEncoder);
}

MulNX::IVEncoder* AVCodecManager::GetCurrentVEncoder() {

    return this->encoders.front();
}