#include "TaskSystem.hpp"

#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>
#include <MulNX/Systems/MessageManager/MessageManager.hpp>

void MulNX::Task::Worker::Start() {
    this->t = std::jthread([this](std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            std::function<bool()> task;
            while (queue.try_dequeue(task)) {
                tasks.push_back(std::move(task));
            }
            for (const auto& task : this->tasks) {
                task();
            }
        }
        return;
        });
    return;
}

bool MulNX::TaskSystem::Init() {
    this->ISys()
        .SubscribeAsync("Task/Create");

    auto [msg, rp] = MulNX::Message::Create<MulNX::Task::RegistrationPacket>("Task/Create"_hash);
    rp->targetWorker = "Messaging";
    rp->task = std::move([this]()->bool {
        this->Update();
        return true;
        });
    this->HandleAddTask(msg);

    auto [msg2, rp2] = MulNX::Message::Create<MulNX::Task::RegistrationPacket>("Task/Create"_hash);
    rp2->targetWorker = "Messaging";
    auto pMessageManager = this->Core->ModuleManager()->FindModule<MessageManager>("MessageManager");
    rp2->task = std::move([pMessageManager]()->bool {
        pMessageManager->HandleDispatch();
        return true;
        });
    this->HandleAddTask(msg2);

    return true;
}

void MulNX::TaskSystem::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Task/Create"_hash: {
        this->ISys().LogInfo("接收到一个任务！");
        this->HandleAddTask(msg);
        break;
    }
    default: {
        break;
    }
    }
}

void MulNX::TaskSystem::HandleAddTask(MulNX::Message& msg) {
    auto RegistrationPacket = std::move(*msg.asp.get<MulNX::Task::RegistrationPacket>());
    auto it = this->workers.find(RegistrationPacket.targetWorker);
    if (it == this->workers.end()) {
        this->workers[RegistrationPacket.targetWorker] = std::make_unique<MulNX::Task::Worker>();
        this->workers[RegistrationPacket.targetWorker]->Start();
        this->ISys().LogSucc(std::format("成功创建工作者：{}", RegistrationPacket.targetWorker));
    }
    this->workers[RegistrationPacket.targetWorker]->queue.enqueue(std::move(RegistrationPacket.task));
    this->ISys().LogSucc(std::format("成功将任务添加进入工作者：{}", RegistrationPacket.targetWorker));
}

void MulNX::TaskSystem::Deinit() {
    for (auto& [name, worker] : this->workers) {
        if (name == "Messaging")continue;
        worker->t.request_stop(); // 如果 t 是 jthread
    }
    for (auto& [name, worker] : this->workers) {
        if (name == "Messaging")continue;
        worker->t.join();
        worker.reset();
    }
    auto worker = std::move(this->workers["Messaging"]);
    worker->t.request_stop();
    worker->t.join();
}