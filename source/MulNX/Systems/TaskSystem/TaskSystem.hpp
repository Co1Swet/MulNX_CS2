#pragma once
#include <MulNX/Core/Module/Module.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>

namespace MulNX {
    class Worker {
    public:
        std::jthread t;
        std::vector<Task>tasks;
        moodycamel::ConcurrentQueue<Task>queue;
        void Start();
    };

    class TaskSystem final :public Module<TaskSystem> {
        MessageManager* pMessageManager = nullptr;
        std::unordered_map<std::string, std::unique_ptr<Worker>>workers{};
        bool Init()override;
        void ProcessMsg(MulNX::Message& msg)override;
    public:
        void Deinit()override;
        void HandleAddTask(MulNX::Task&& msg);
    };
}