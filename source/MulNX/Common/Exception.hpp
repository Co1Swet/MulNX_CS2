#pragma once
#include <stdexcept>
#include <source_location>

namespace MulNX {
    class Exception :public std::runtime_error {
    public:
        std::source_location where;
        Exception(const std::string& msg,
            std::source_location loc = std::source_location::current())
            : std::runtime_error(msg), where(loc) {}
    };
}