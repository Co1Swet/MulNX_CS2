#pragma once
#include <string>
#include <vector>
#include <cstdint>

class CamPlayRequest final {
public:
    std::string campathName{};
    float offsetTime = 0.0f;
    bool isActiveMode = false;
};

class CamPlayBatch final {
public:
    uint64_t requestId = 0;
    uint64_t generation = 0;
    std::vector<CamPlayRequest> paths;
    std::string error;
};

class CamPlayEvent final {
public:
    std::string type;
    uint64_t requestId = 0;
    uint64_t generation = 0;
    std::string state;
    std::string reason;
    uint64_t currentRequestId = 0;
};
