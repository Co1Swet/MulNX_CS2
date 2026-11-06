#pragma once
#include <string>

class CamPlayRequest final {
public:
    std::string campathName{};
    double offsetTime = 0.0;
    bool isActiveMode = false;
};
