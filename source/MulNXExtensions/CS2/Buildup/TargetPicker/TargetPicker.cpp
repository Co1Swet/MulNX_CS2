#include "TargetPicker.hpp"

bool TargetPicker::Init() {
    (*this)
        .SubscribeAsync("TargetPick/Check/SpecM4");

    this->SendTask("Main", "CSControl", [this]() {
        this->Main();
        return true;
        });

    return true;
}

void TargetPicker::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "TargetPick/Check/SpecM4"_hash: {
        try {
            auto oSteam64UID = this->CS2Entitys->TryGetObservingSteam64UID();
            if (!oSteam64UID.has_value())break;
            auto pPawn = this->CS2Entitys->TryGetObservingPawn();
            if (!pPawn)break;
            auto pos = pPawn->GetEyePos();
            auto angle = MulNX::MRead(pPawn->angEyeAngles());
            this->SetTarget(oSteam64UID.value());
            this->AsyncCommand(std::format("spec_goto {} {} {} {} {}",
                pos.x, pos.y, pos.z, angle.x, angle.y));
            this->lastUpdateTime.store(std::chrono::steady_clock::now(), std::memory_order_release);
        }
        catch (const MulNX::Exception& e) {
            this->LogError(e);
        }
        break;
    }
    }
}

void TargetPicker::Main() {
    this->Update();
}

bool TargetPicker::UpdateTarget() {
    try {
        auto oSteam64UID = this->CS2Entitys->TryGetObservingSteam64UID();
        if (!oSteam64UID.has_value())return false;
        auto pPawn = this->CS2Entitys->TryGetObservingPawn();
        if (!pPawn)return false;
        auto pos = pPawn->GetEyePos();
        auto angle = MulNX::MRead(pPawn->angEyeAngles());
        this->SetTarget(oSteam64UID.value());
        this->AsyncCommand(std::format("spec_goto {} {} {} {} {}",
            pos.x, pos.y, pos.z, angle.x, angle.y));
        this->lastUpdateTime.store(std::chrono::steady_clock::now(), std::memory_order_release);
        return true;
    }
    catch (const MulNX::Exception& e) {
        this->LogError(e);
    }
    return false;
}

Steam64UID TargetPicker::GetTarget()const {
    return this->target.load(std::memory_order_acquire);
}
void TargetPicker::SetTarget(Steam64UID uid) {
    return this->target.store(uid, std::memory_order_release);
}
std::chrono::steady_clock::time_point TargetPicker::GetLastUpdateTime() {
    return this->lastUpdateTime.load(std::memory_order_acquire);
}