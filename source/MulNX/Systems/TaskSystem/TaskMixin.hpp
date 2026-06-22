#pragma once
#include <MulNX/Core/Module/IModule.hpp>
#include <MulNX/Common/Task.hpp>

namespace MulNX {
    class TaskSystem;

    template<typename T>
    class TaskMixin {
    private:
        TaskSystem* pTaskSys = nullptr;
        T* This() { return static_cast<T*>(this); }
    public:
        TaskMixin() {
            This()->delayInits->push_back([this]() {
                this->pTaskSys = static_cast<TaskSystem*>(This()->FindModule("TaskSystem"));
                return true;
                });
        }

        void SendTask(std::string&& name, std::string&& targetWorker, std::function<bool()>&& Do) {
            auto fullName = std::format("{}::{}", This()->GetName(), std::move(name));
            This()->LogInfo(I18n("module.send_task", fullName, targetWorker));
            auto [msg, rp] = MulNX::Message::Create<MulNX::Task>("Task/Create"_hash,
                std::move(fullName), std::move(targetWorker), std::move(Do));
            This()->PublishAsync(std::move(msg));
        }
        void ImCreateTask(std::string&& name, std::string&& targetWorker, std::function<bool()>&& Do) {
            auto fullName = std::format("{}::{}", This()->GetName(), std::move(name));
            MulNX::Task task(std::move(fullName), std::move(targetWorker), std::move(Do));
            this->pTaskSys->HandleAddTask(std::move(task));
        }
    };
}