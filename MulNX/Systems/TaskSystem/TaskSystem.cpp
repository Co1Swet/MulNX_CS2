#include "TaskSystem.hpp"

#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>
#include <MulNX/Systems/MessageManager/MessageManager.hpp>
#include <MulNX/Systems/GlobalVars/GlobalVars.hpp>

void MulNX::Worker::Start() {
    this->t = std::jthread([this](std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            MulNX::Task task;
            while (queue.try_dequeue(task)) {
                this->tasks.push_back(std::move(task));
            }
            for (auto it = this->tasks.begin();it != this->tasks.end();) {
                auto keep = it->Do();
                if (keep)++it;
                else it = this->tasks.erase(it);
            }
        }
        return;
        });
    return;
}

bool MulNX::TaskSystem::Init() {
    this->ISys()
        .SubscribeAsync("Task/Create");

    std::thread t([this]()->void {
        while (!this->GlobalVars->SystemReady.load(std::memory_order_acquire)){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        auto [msg2, rp2] = MulNX::Message::Create<MulNX::Task>("Task/Create"_hash);
        rp2->name = "MessageManager::HandleDispatch";
        rp2->targetWorker = "Messaging";
        auto pMessageManager = this->Core->ModuleManager()->FindModule<MessageManager>("MessageManager");
        pMessageManager->ISys().LogSucc("消息派发激活！");
        rp2->Do = std::move([this, pMessageManager]()->bool {
            pMessageManager->HandleDispatch();
            return true;
            });
        this->HandleAddTask(msg2);

        auto [msg, rp] = MulNX::Message::Create<MulNX::Task>("Task/Create"_hash);
        rp->name = "TaskSystem::Update";
        rp->targetWorker = "Messaging";
        rp->Do = std::move([this]()->bool {
            this->Update();
            return true;
            });
        this->HandleAddTask(msg);

        });
    t.detach();

    return true;
}

void MulNX::TaskSystem::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Task/Create"_hash: {
        this->HandleAddTask(msg);
        break;
    }
    default: {
        break;
    }
    }
}

void MulNX::TaskSystem::HandleAddTask(MulNX::Message& msg) {
    auto task = std::move(*msg.asp.get<MulNX::Task>());
    const auto& targetWorker = task.targetWorker;
    const auto& taskName = task.name;
    auto it = this->workers.find(targetWorker);
    if (it == this->workers.end()) {
        this->workers[targetWorker] = std::make_unique<MulNX::Worker>();
        this->workers[targetWorker]->Start();
        this->ISys().LogSucc(std::format("成功创建工作者：{}", targetWorker));
    }
    this->ISys().LogSucc(std::format("成功将任务：{} 添加进入工作者：{}", taskName, targetWorker));
    this->workers[targetWorker]->queue.enqueue(std::move(task));
}

void MulNX::TaskSystem::Deinit() {
    for (auto& [name, worker] : this->workers) {
        if (name == "Messaging")continue;
        worker->t.request_stop();
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