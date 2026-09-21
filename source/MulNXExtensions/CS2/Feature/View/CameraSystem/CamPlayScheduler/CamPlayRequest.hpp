#pragma once
#include <string>

class CamPlayRequest final {
public:
    std::string campathName{};
    float offsetTime = 0.0f;
    bool isActiveMode = false;
};