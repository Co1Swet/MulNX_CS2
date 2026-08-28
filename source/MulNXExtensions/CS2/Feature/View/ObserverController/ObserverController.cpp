#include "ObserverController.hpp"
#include <Intro/HookConsole/HookConsole.hpp>

bool ObserverController::Init() {
    (*this)
        .SubscribeAsync("CameraSystem/Play/Started")
        .SubscribeAsync("CameraSystem/Play/Ended")
        .SubscribeAsync("Observe/SpecSteam64UID")
        .SubscribeAsync("Observe/SpecHandle")
        .SubscribeAsync("spec_mode_changed_to")
        ;

    this->SendTask("Main", "CSControl", [this]() -> bool {
        this->Main();
        return true;
        });

    return true;
}

void ObserverController::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "CameraSystem/Play/Started"_hash: {
        this->LogInfo("收到运镜开始消息");
        break;
    }
    case "CameraSystem/Play/Ended"_hash: {
        this->LogInfo("收到运镜结束消息");
        break;
    }
    case "spec_mode_changed_to"_hash: {
        auto&& [newMode] = msg.Access<uint8_t>();
        this->OnSpecModeChanged(newMode);
        break;
    }
    case "Observe/SpecSteam64UID"_hash: {
        auto&& [uid] = msg.Access<Steam64UID>();
        this->SpecSteam64UID(uid);
        break;
    }
    case "Observe/SpecHandle"_hash: {
        auto&& [handle] = msg.Access<CS2::CHandleBase>();
        this->SpecHandle(handle);
        break;
    }
    }
}

void ObserverController::Main() {
    this->UpdateObserverState();  // 轮询检测模式变化，并发布事件
    this->Update();      // 处理消息队列（包括 spec_mode_changed_to）
}

void ObserverController::UpdateObserverState() {
    try {
        auto localPlayerPawn = this->CS2Entitys->GetLocalPlayerPawnEx();
        if (!localPlayerPawn)return;
        auto pObserverServices = MulNX::MRead(localPlayerPawn->pObserverServices());
        if (!pObserverServices)return;
        uint8_t detectedMode = MulNX::MRead(pObserverServices->iObserverMode());
        static uint8_t lastObservedSpecMode = detectedMode;
        if (lastObservedSpecMode != detectedMode) {
            MulNX::Message msg("spec_mode_changed_to"_hash);
            auto&& [newMode] = msg.Access<uint8_t>();
            newMode = detectedMode;
            this->PublishAsync(std::move(msg));
            lastObservedSpecMode = detectedMode;
        }
    }
    catch (const std::exception& e) {
        this->LogError(std::format("UpdateObserverState error: {}", e.what()));
    }
}

void ObserverController::OnSpecModeChanged(uint8_t newMode) {
    if (newMode == 2) {
        this->LogWarning("检测到 spec_mode 切换至 2");
        this->PublishAsync("CameraSystem/Play/Shutdown"_hash);
    }
}

void ObserverController::SetSpecMode(uint8_t mode) {
    this->AsyncCommand(std::format("spec_mode {}", mode));
}

bool ObserverController::SpecHandle(CS2::CHandleBase handle) {
    try {
        auto* localPawn = this->CS2Entitys->GetLocalPlayerPawnEx();
        if (!localPawn) {
            this->LogWarning("尝试在无本地实体情况下设置观战？");
            return false;
        }
        auto pObserverServices = MulNX::MRead(localPawn->pObserverServices());
        auto* pTarget = pObserverServices->hObserverTarget();
        MulNX::MWrite(pObserverServices->iObserverMode(), (uint8_t)2);
        MulNX::MWrite(&(pTarget->value), handle.value);
        return true;
    }
    catch (const std::runtime_error& e) {
        this->LogError(std::format("在设置观战目标时发生错误：{}", e.what()));
        return false;
    }
    catch (...) {
        this->LogError("在设置观战目标时发生未知错误");
        return false;
    }
}

void ObserverController::SpecSteam64UID(Steam64UID uid) {
    int counter = 0;
    while (true) {
        try {
            auto* pController = this->CS2Entitys->FindControllerBySteam64UID(uid);
            if (!pController) {
                this->LogError("因未查找到Controller导致观战设置失败！");
                break;
            }
            auto handle = MulNX::MRead(pController->m_hPlayerPawn());
            MulNX::Message msg("Observe/SpecHandle"_hash);
            auto&& [handleRef] = msg.Access<CS2::CHandleBase>();
            handleRef = handle;
            this->PublishAsync(std::move(msg));
            break;
        }
        catch (const std::exception& e) {
            this->LogError(I18n("ob.Spec64.error", e.what()));
            ++counter;
            if (counter == 10) {
                return;
            }
            continue;
        }
    }
}
bool ObserverController::SpecPlayer(int IndexInMap) {
    this->AsyncCommand("spec_mode 2;spec_player " + std::to_string(this->pBackgroundEntityScan->CS2EBGameData.Players[IndexInMap].IndexInMap));
    return true;
}