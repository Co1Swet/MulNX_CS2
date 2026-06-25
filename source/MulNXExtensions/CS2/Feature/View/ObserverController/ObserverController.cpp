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
        this->HandlePlayStarted();
        break;
    }
    case "CameraSystem/Play/Ended"_hash: {
        this->LogInfo("收到运镜结束消息");
        this->HandlePlayEnded();
        break;
    }
    case "spec_mode_changed_to"_hash: {
        uint8_t newMode = msg.p1.low<uint8_t>();
        this->OnSpecModeChanged(newMode);
        break;
    }
    case "Observe/SpecSteam64UID"_hash: {
        Steam64UID uid = msg.p1.as<Steam64UID>();
        this->SpecSteam64UID(uid);
        break;
    }
    case "Observe/SpecHandle"_hash: {
        auto handle = msg.p1.low<CS2::CHandleBase>();
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
        auto localPlayerPawn = this->CS2->client.GetLocalPlayerPawn();
        if (!localPlayerPawn)return;
        auto pObserverServices = MulNX::MRead(localPlayerPawn->pObserverServices());
        if (!pObserverServices)return;
        uint8_t detectedMode = MulNX::MRead(pObserverServices->iObserverMode());
        static uint8_t lastObservedSpecMode = detectedMode;
        if (lastObservedSpecMode != detectedMode) {
            MulNX::Message msg("spec_mode_changed_to"_hash);
            msg.p1.low<uint8_t>() = detectedMode;
            this->PublishAsync(std::move(msg));
            lastObservedSpecMode = detectedMode;
        }
    }
    catch (const std::exception& e) {
        this->LogError(std::format("UpdateObserverState error: {}", e.what()));
    }
}

bool ObserverController::HandlePlayStarted() {
    if (this->CampathPlaying) {
        this->LogWarning("已在运镜播放中，重复的播放开始消息将被忽略。");
        return false;
    }
    this->startedAsSpecMode = this->currentMode;
    this->CampathPlaying = true;

    if (this->currentMode != 4) {   // 不是自由视角，需要切换
        this->LogInfo("运镜开始时当前模式非 spec_mode 4，切换");
        this->SetSpecMode(4);
    }
    else {
        this->LogInfo("运镜开始时已是 spec_mode 4，直接开始播放。");
    }
    return true;
}

bool ObserverController::HandlePlayEnded() {
    if (startedAsSpecMode == 2) {
        this->LogInfo("运镜结束后恢复 spec_mode 2。");
        this->SetSpecMode(2);
    }
    this->CampathPlaying = false;
    this->startedAsSpecMode = 0;
    return true;
}

void ObserverController::OnSpecModeChanged(uint8_t newMode) {
    this->currentMode = newMode;
    // 运镜播放中，检测用户是否切回 spec_mode 2
    if (this->CampathPlaying && newMode == 2) {
        this->LogWarning("检测到 spec_mode 2，已中断当前运镜。");
        this->PublishAsync("CameraSystem/Play/Shutdown"_hash);
        this->CampathPlaying = false;
    }
}

CS2::CCSPlayerController* ObserverController::FindControllerBySteam64UID(Steam64UID uid) {
    CS2::CCSPlayerController* pController = nullptr;
    try {
        for (int i = 0; i < 32; ++i) {
            auto* controller = this->CS2->client.GetBaseEntity(i)->As<CS2::CCSPlayerController>();
            if (!controller)continue;
            if (!controller->IsPlayerController())continue;
            auto steam64UID = MulNX::MRead(controller->m_steamID());
            if (steam64UID != uid)continue;
            pController = controller;
            break;
        }
    }
    catch (...) {
        this->LogError("FindControllerBySteam64UID 失败");
    }
    return pController;
}

void ObserverController::SetSpecMode(uint8_t mode) {
    this->AsyncCommand(std::format("spec_mode {}", mode));
}

bool ObserverController::SpecHandle(CS2::CHandleBase handle) {
    try {
        auto* localPawn = this->CS2->client.GetLocalPlayerPawn();
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
            auto* pController = this->FindControllerBySteam64UID(uid);
            auto handle = MulNX::MRead(pController->m_hPlayerPawn());
            MulNX::Message msg("Observe/SpecHandle"_hash);
            msg.p1.low<CS2::CHandleBase>() = handle;
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
    this->AsyncCommand("spec_mode 2;spec_player " + std::to_string(this->CS2->CS2EBGameData.Players[IndexInMap].IndexInMap));
    return true;
}