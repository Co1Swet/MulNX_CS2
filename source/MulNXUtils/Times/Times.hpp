#pragma once
#include <chrono>
#include <format>
#include <string>

namespace MulNX::Time {
    inline std::string GetTimestamp(std::chrono::system_clock::time_point tp = std::chrono::system_clock::now()) {
        using namespace std::chrono;

        auto local = current_zone()->to_local(tp);
        auto secs = floor<seconds>(local);
        auto us = duration_cast<microseconds>(local - secs);

        return std::format("{:%y_%m_%d_%H_%M_%S}_{:06}", secs, us.count());
    }
}