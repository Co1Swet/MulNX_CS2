#include "ObserverController.hpp"
#include <Intro/HookConsole/HookConsole.hpp>

bool ObserverController::Init() {
    (*this)
        .SubscribeAsync<Steam64UID>("Observe/SpecSteam64UID")
        ;

    this->SendTask("Update", "CSControl", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void ObserverController::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Observe/SpecSteam64UID"_hash: {
        auto&& [uid] = msg.Access<Steam64UID>();
        this->SpecSteam64UID(uid);
        break;
    }
    }
}

void ObserverController::SpecSteam64UID(Steam64UID uid) {
    int counter = 0;
    while (true) {
        try {
            int index = -1;
            auto* pController = this->CS2Entitys->FindControllerBySteam64UID(uid, &index);
            if (!pController) {
                this->LogError("因未查找到Controller导致观战设置失败！");
                break;
            }
            if (index == -1) {
                this->LogError("未查找到有效的控制器！");
                break;
            }
            this->AsyncCommand(std::format("spec_player {}", index));
            break;
        }
        catch (const std::exception& e) {
            this->LogError(std::format("在以Steam64UID设置观战目标时出错：{}", e.what()));
            ++counter;
            if (counter == 10) {
                return;
            }
            continue;
        }
    }
}