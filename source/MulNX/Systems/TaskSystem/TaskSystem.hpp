#pragma once
#include <MulNX/Core/Module/Module.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>

namespace MulNX {
    class Worker {
        std::vector<Task>tasks{};
    public:
        std::jthread t{};
        moodycamel::ConcurrentQueue<Task>queue{};
        std::string name{};
        void Start();
    };

    class TaskSystem final :public Module<TaskSystem> {
        MessageManager* pMessageManager = nullptr;
        std::unordered_map<std::string, std::unique_ptr<Worker>>workers{};
        bool Init()override;
        bool Deinit()override;
        void ProcessMsg(MulNX::Message& msg)override;
    public:
        void HandleAddTask(MulNX::Task&& msg);
    };
}