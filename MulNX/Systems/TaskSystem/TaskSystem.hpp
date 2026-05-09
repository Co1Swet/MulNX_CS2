#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>

namespace MulNX {
    namespace Task {
        class Worker {
        public:
            std::jthread t;
            std::vector<std::function<bool()>>tasks;
            moodycamel::ConcurrentQueue<std::function<bool()>>queue;
            void Start();
        };

        class RegistrationPacket {
        public:
            std::string targetWorker;
            std::function<bool()>task;
        };
    }
    class TaskSystem final :public ModuleBase {
        std::unordered_map<std::string, std::unique_ptr<Task::Worker>>workers{};
        void HandleAddTask(MulNX::Message& msg);
    public:
        bool Init()override;
        void ProcessMsg(MulNX::Message& msg)override;
        void Deinit()override;
    };
}