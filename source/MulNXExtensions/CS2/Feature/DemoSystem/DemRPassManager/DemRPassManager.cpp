#include "DemRPassManager.hpp"

bool DemRPassManager::Init() {

    (*this)
        .SubscribeAsync("Demo/Record/Enqueue")
        ;

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void DemRPassManager::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Record/Enqueue"_hash: {
        std::unique_lock lock(this->smutex);
        auto task = std::move(*msg.asp.get<Demo::RecordTask>());
        auto name = task.demoName;
        if (name.empty()) {
            this->LogError("不能入队空Demo名的录制任务！");
            break;
        }
        this->bufferTasks.push_back(std::move(task));
        break;
    }
    }
}