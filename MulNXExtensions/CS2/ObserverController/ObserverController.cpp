#include "ObserverController.hpp"
#include <MulNXExtensions/CS2/CSController/CSController.hpp>
#include <atomic>
#include <chrono>
#include <thread>

bool ObserverController::Init() {
    this->ISys()
        .SubscribeAsync("CameraSystem/Play/Started")
        .SubscribeAsync("CameraSystem/Play/Ended")
        .SubscribeAsync("Observe/SpecSteam64UID")
        .SubscribeAsync("Observe/SpecHandle")
        .SubscribeAsync("spec_mode_changed_to");

    this->SendTask("CSControl", [this]() -> bool {
        this->Main();
        return true;
        });

    return true;
}

void ObserverController::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "CameraSystem/Play/Started"_hash: {
        this->ISys().LogInfo("收到运镜开始消息");
        this->HandlePlayStarted();
        break;
    }
    case "CameraSystem/Play/Ended"_hash: {
        this->ISys().LogInfo("收到运镜结束消息");
        this->HandlePlayEnded();
        break;
    }
    case "spec_mode_changed_to"_hash: {
        uint8_t newMode = msg.p1.low<uint8_t>();
        this->OnSpecModeChanged(newMode);
        break;
    }
    case "Observe/SpecSteam64UID"_hash: {
        while (true) {
            try {
                Steam64UID uid = msg.p1.as<Steam64UID>();
                auto* pController = this->FindControllerBySteam64UID(uid);
                auto handle = MulNX::MRead(pController->m_hPlayerPawn());
                MulNX::Message msg("Observe/SpecHandle"_hash);
                msg.p1.low<CS2::CHandleBase>() = handle;
                this->ISys().PublishAsync(std::move(msg));
                break;
            }
            catch (...) {
                continue;
            }
        }
        
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
    this->EntryProcessMsg();      // 处理消息队列（包括 spec_mode_changed_to）
}

void ObserverController::UpdateObserverState() {
    try {
        auto localPlayerPawn = this->CS2()->Modules.client.GetLocalPlayerPawn();
        if (!localPlayerPawn)return;
        auto pObserverServices = MulNX::MRead(localPlayerPawn->pObserverServices());
        if (!pObserverServices)return;
        uint8_t detectedMode = MulNX::MRead(pObserverServices->iObserverMode());
        static uint8_t lastObservedSpecMode = detectedMode;
        if (lastObservedSpecMode != detectedMode) {
            MulNX::Message msg("spec_mode_changed_to"_hash);
            msg.p1.low<uint8_t>() = detectedMode;
            this->ISys().PublishAsync(std::move(msg));
            lastObservedSpecMode = detectedMode;
        }
    }
    catch (const std::exception& e) {
        this->ISys().LogError(std::format("UpdateObserverState error: {}", e.what()));
    }
}

bool ObserverController::HandlePlayStarted() {
    if (this->CampathPlaying) {
        this->ISys().LogWarning("已在运镜播放中，重复的播放开始消息将被忽略。");
        return false;
    }
    this->startedAsSpecMode = this->currentMode;
    this->CampathPlaying = true;

    if (this->currentMode != 4) {   // 不是自由视角，需要切换
        this->ISys().LogInfo("运镜开始时当前模式非 spec_mode 4，切换");
        this->SetSpecMode(4);
    }
    else {
        this->ISys().LogInfo("运镜开始时已是 spec_mode 4，直接开始播放。");
    }
    return true;
}

bool ObserverController::HandlePlayEnded() {
    if (startedAsSpecMode == 2) {
        this->ISys().LogInfo("运镜结束后恢复 spec_mode 2。");
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
        this->ISys().LogWarning("检测到 spec_mode 2，已中断当前运镜。");
        this->ISys().PublishAsync("CameraSystem/Play/Shutdown"_hash);
        this->CampathPlaying = false;
    }
}

CS2::CCSPlayerController* ObserverController::FindControllerBySteam64UID(Steam64UID uid) {
    CS2::CCSPlayerController* pController = nullptr;
    try {
        for (int i = 0; i < this->CS2()->Modules.client.dwGameEntitySystem_highestEntityIndex(); ++i) {
            auto* controller = this->CS2()->Modules.client.GetBaseEntity(i)->As<CS2::CCSPlayerController>();
            if (!controller)continue;
            if (!controller->IsPlayerController())continue;
            auto steam64UID = MulNX::MRead(controller->m_steamID());
            if (steam64UID != uid)continue;
            pController = controller;
            break;
        }
    }
    catch (...) {
        this->ISys().LogError("test");
    }
    return pController;
}

void ObserverController::SetSpecMode(uint8_t mode) {
    try {
        auto* localPawn = this->CS2()->Modules.client.GetLocalPlayerPawn();
        if (!localPawn) {
            this->ISys().LogWarning("尝试在无本地实体情况下设置观战？");
            return;
        }
        auto pObserverServices = MulNX::MRead(localPawn->pObserverServices());
        auto* pTarget = pObserverServices->hObserverTarget();
        MulNX::MWrite(pObserverServices->iObserverMode(), mode);
        return;
    }
    catch (const std::runtime_error& e) {
        this->ISys().LogError(std::format("在设置观战模式时发生错误：{}", e.what()));
        return;
    }
    catch (...) {
        this->ISys().LogError("在设置观战模式时发生未知错误");
        return;
    }
}

bool ObserverController::SpecHandle(CS2::CHandleBase handle) {
    try {
        auto* localPawn = this->CS2()->Modules.client.GetLocalPlayerPawn();
        if (!localPawn) {
            this->ISys().LogWarning("尝试在无本地实体情况下设置观战？");
            return false;
        }
        auto pObserverServices = MulNX::MRead(localPawn->pObserverServices());
        auto* pTarget = pObserverServices->hObserverTarget();
        MulNX::MWrite(&pTarget->handle, handle.handle);
        MulNX::MWrite(pObserverServices->iObserverMode(), (uint8_t)2);
        return true;
    }
    catch (const std::runtime_error& e) {
        this->ISys().LogError(std::format("在设置观战目标时发生错误：{}", e.what()));
        return false;
    }
    catch (...) {
        this->ISys().LogError("在设置观战目标时发生未知错误");
        return false;
    }
}