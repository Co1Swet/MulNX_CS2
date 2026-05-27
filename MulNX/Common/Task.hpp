#pragma once
#include <string>
#include <functional>

namespace MulNX {
    class Task {
    public:
        Task() = default;
        Task(std::string&& name, std::string&& targetWorker, std::function<bool()>&& Do) :
            name(name), targetWorker(targetWorker), Do(Do) {};

        std::string targetWorker;
        std::string name;
        std::function<bool()>Do;
    };
}