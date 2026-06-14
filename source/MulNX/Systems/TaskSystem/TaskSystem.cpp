#include "TaskSystem.hpp"

#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>
#include <MulNX/Systems/MessageManager/MessageManager.hpp>
#include <MulNX/Systems/GlobalVars/GlobalVars.hpp>

void MulNX::Worker::Start() {
    this->t = std::jthread([this](std::stop_token stoken) {
        // 用于可中断等待的对象
        std::mutex dummyMutex;
        std::condition_variable_any cv;
        constexpr auto cycleDuration = std::chrono::milliseconds(10);

        while (!stoken.stop_requested()) {
            auto cycleStart = std::chrono::steady_clock::now();

            // 1. 尽量清空并发队列，收集任务
            MulNX::Task task;
            while (queue.try_dequeue(task)) {
                this->tasks.push_back(std::move(task));
            }

            // 2. 执行所有待处理任务
            for (auto it = this->tasks.begin(); it != this->tasks.end();) {
                if (it->Do()) {
                    ++it;
                }
                else {
                    it = this->tasks.erase(it);
                }
            }

            // 3. 限速：若整个周期不足 10ms，则休眠剩余时间
            auto elapsed = std::chrono::steady_clock::now() - cycleStart;
            if (elapsed < cycleDuration) {
                auto remaining = cycleDuration - elapsed;
                std::unique_lock lock(dummyMutex);
                // 等待剩余时间，但能被 stop_token 提前唤醒
                cv.wait_for(lock, stoken, remaining, [] { return false; });
            }
            // 如果 stop_token 被触发，wait_for 会提前返回，外层 while 检查后退出
        }
        });
}

bool MulNX::TaskSystem::Init() {
    this->ISys()
        .SubscribeAsync("Task/Create");

    std::thread t([this]()->void {
        while (!this->GlobalVars->SystemReady.load(std::memory_order_acquire)){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        auto [msg, rp] = MulNX::Message::Create<MulNX::Task>("Task/Create"_hash);
        rp->name = "TaskSystem::Update";
        rp->targetWorker = "MulNXMain";
        rp->Do = std::move([this]()->bool {
            this->Update();
            return true;
            });
        this->HandleAddTask(msg);

        auto [msg2, rp2] = MulNX::Message::Create<MulNX::Task>("Task/Create"_hash);
        rp2->name = "MessageManager::HandleDispatch";
        rp2->targetWorker = "Messaging";
        auto pMessageManager = this->Core->ModuleManager()->FindModule<MessageManager>("MessageManager");
        pMessageManager->ISys().LogSucc("消息派发激活！");
        rp2->Do = std::move([pMessageManager]()->bool {
            pMessageManager->HandleDispatch();
            return true;
            });
        this->HandleAddTask(msg2);

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