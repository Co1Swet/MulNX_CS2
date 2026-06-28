#pragma once
#include <stdexcept>
#include <source_location>

namespace MulNX {
    class Exception :public std::runtime_error {
    public:
        std::source_location where;
    };
}