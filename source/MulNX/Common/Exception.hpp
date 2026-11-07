#pragma once
#include <stdexcept>
#include <source_location>
#include <format>

namespace MulNX {
    class Exception :public std::runtime_error {
        std::source_location where{};
        std::string WhatInfo{};
        std::string whatInfo{};
    public:

        explicit Exception(const std::string& msg,
            std::source_location loc = std::source_location::current())
            : std::runtime_error(msg) {
            this->WhatInfo = msg;
            this->SetWhere(loc);
        }

        void SetWhere(std::source_location loc) {
            this->where = loc;
            this->whatInfo = std::format("{} (at {}:{})",
                this->WhatInfo, loc.file_name(), loc.line());
        }

        const char* what()const noexcept override {
            return this->whatInfo.c_str();
        }
        std::string What()const noexcept {
            return this->WhatInfo;
        }
        std::source_location Where()const noexcept {
            return this->where;
        }
    };
}