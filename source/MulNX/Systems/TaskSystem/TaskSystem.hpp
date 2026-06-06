#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>

namespace MulNX {
    class Worker {
    public:
        std::jthread t;
        std::vector<Task>tasks;
        moodycamel::ConcurrentQueue<Task>queue;
        void Start();
    };

    class TaskSystem final :public ModuleBase {
        std::unordered_map<std::string, std::unique_ptr<Worker>>workers{};
        void HandleAddTask(MulNX::Message& msg);
    public:
        bool Init()override;
        void ProcessMsg(MulNX::Message& msg)override;
        void Deinit()override;
    };
}