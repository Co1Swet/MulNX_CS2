#include "CamPlayScheduler.hpp"
#include <CameraSystem/CampathManager/CampathManager.hpp>

bool CamPlayScheduler::Init() {
    this->pCampathManager = this->FindModule<CampathManager>("CampathManager");
    this->pCamDrawer = &this->FindModule<CameraSystem>("CameraSystem")->CamDrawer;

    (*this)
        .SubscribeAsync("CamPlay/Request")
        .SubscribeAsync("CamPlay/Batch")
        .SubscribeAsync<uint64_t, uint64_t>("CamPlay/CancelTracked")
        .SubscribeAsync<uint64_t>("CamPlay/StatusTracked")
        .SubscribeAsync<void>("CamPlay/Clear")
        .SubscribeAsync<void>("CamPlay/ClearForce")

        .SubscribeAsync<void>("CamPlay/Draw/Enable")
        .SubscribeAsync<void>("CamPlay/Draw/Disable")
        .SubscribeAsync<void>("CamPlay/Override/Enable")
        .SubscribeAsync<void>("CamPlay/Override/Disable")
        ;

    this->SubscribeSync("CamSync/Clear", [this](auto&&...) { this->Invalidate(); });
    this->SubscribeSync("CamSync/Invalidate", [this](auto&&...) { this->Invalidate(); });

    this->SubscribeSync("System/Init/End", [this](auto&&...) {
        this->UIRegisterBackground("DrawPlayCamera", [this](auto&&...) {
            this->DrawPlayCamera();
            });
        });

    return true;
}

void CamPlayScheduler::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "CamPlay/Batch"_hash: {
        const auto* batch = msg.asp.get<CamPlayBatch>();
        CamPlayEvent event{};
        event.type = "CamPlay/State";
        event.requestId = batch->requestId;
        event.generation = batch->generation;
        event.state = "failed";
        if (batch->generation != this->currentGeneration) event.reason = "stale_generation";
        else if (this->tracked) event.reason = "busy";
        else if (!this->camOverride.load(std::memory_order_acquire)) event.reason = "override_disabled";
        else event.reason = batch->error;
        std::vector<PlaySlot> slots;
        if (event.reason.empty()) {
            std::shared_lock lock(this->pCampathManager->smutex);
            const float time = this->pTimeline->GetTime();
            for (const auto& request : batch->paths) {
                auto path = this->pCampathManager->FindCampath(request.campathName);
                if (!path || path->CameraKeyframes.size() < 2) {
                    event.reason = "path_unavailable";
                    break;
                }
                PlaySlot slot{};
                slot.offsetTime = path->GetStartTime() - time + request.offsetTime;
                slot.pCampath = std::move(path);
                slot.tracked = true;
                slots.push_back(std::move(slot));
            }
        }
        if (!event.reason.empty()) {
            this->PostEvent(std::move(event));
            break;
        }
        this->requestId = batch->requestId;
        this->generation = batch->generation;
        this->started = false;
        this->tracked = true;
        this->playslots.insert(this->playslots.end(), slots.begin(), slots.end());
        break;
    }
    case "CamPlay/CancelTracked"_hash: {
        auto&& [id, gen] = msg.Access<uint64_t, uint64_t>();
        const bool matched = gen == this->currentGeneration;
        if (matched) {
            if (this->tracked) this->ClearTracked("cancelled", "cancelled");
            ++this->currentGeneration;
        }
        CamPlayEvent event{};
        event.type = "CamPlay/CancelTracked";
        event.requestId = id;
        event.generation = this->currentGeneration;
        event.matched = matched;
        this->PostEvent(std::move(event));
        break;
    }
    case "CamPlay/StatusTracked"_hash: {
        auto&& [id] = msg.Access<uint64_t>();
        CamPlayEvent event{};
        event.type = "CamPlay/StatusTracked";
        event.requestId = id;
        event.generation = this->currentGeneration;
        event.hasCurrent = this->tracked;
        if (this->tracked) {
            event.currentRequestId = this->requestId;
            event.currentGeneration = this->generation;
            event.currentState = this->started ? "started" : "pending";
        }
        this->PostEvent(std::move(event));
        break;
    }
    case "CamPlay/Request"_hash: {
        auto request = msg.asp.get<CamPlayRequest>();
        auto& name = request->campathName;
        std::shared_ptr<const FreeCameraPath> pCampath = this->pCampathManager->FindCampath(name);
        if (!pCampath) {
            this->LogError(std::format("请求播放运镜 \"{}\" 而未查找到！", name));
            break;
        }
        PlaySlot slot{};
        slot.pCampath = std::move(pCampath);
        slot.offsetTime = request->offsetTime;
        slot.isActiveMode = request->isActiveMode;
        this->playslots.push_back(std::move(slot));
        this->LogSucc(std::format("已添加运镜到播放阵列：{} ，时间偏移为：{}",
            name, request->offsetTime));
        break;
    }
    case "CamPlay/Clear"_hash: {
        ++this->currentGeneration;
        if (this->tracked) this->ClearTracked("cancelled", "clear");
        for (auto it = this->playslots.begin();it != this->playslots.end();) {
            if (it->isActiveMode) {
                ++it;
                continue;
            }
            this->LogInfo(std::format("因清理请求移除运镜：{}", it->pCampath->GetName()));
            it = this->playslots.erase(it);
        }
        break;
    }
    case "CamPlay/ClearForce"_hash: {
        ++this->currentGeneration;
        if (this->tracked) this->ClearTracked("cancelled", "clear_force");
        for (auto& slot : this->playslots) {
            this->LogInfo(std::format("因强制清理请求移除运镜：{}", slot.pCampath->GetName()));
        }
        this->playslots.clear();
        break;
    }
    case "CamPlay/Draw/Enable"_hash: {
        this->drawCam.store(true, std::memory_order_release);
        break;
    }
    case "CamPlay/Draw/Disable"_hash: {
        this->drawCam.store(false, std::memory_order_release);
        break;
    }
    case "CamPlay/Override/Enable"_hash: {
        this->camOverride.store(true, std::memory_order_release);
        break;
    }
    case "CamPlay/Override/Disable"_hash: {
        this->camOverride.store(false, std::memory_order_release);
        ++this->currentGeneration;
        if (this->tracked) this->ClearTracked("cancelled", "override_disabled");
        break;
    }
    }
}

bool CamPlayScheduler::HandleUpdate(CameraSystemIO* IO) {
    this->Update();

    if (this->playslots.empty()) {
        this->needDrawCamera = false;
        return false;
    }

    bool ret = false;
    bool trackedIn = false;
    const float time = this->pTimeline->GetTime();
    if (this->tracked) {
        CameraSystemIO check{};
        bool invalidFrame = false;
        for (const auto& slot : this->playslots) {
            if (!slot.tracked) continue;
            check.FrameGameTime = time;
            check.ElementTime = time + slot.offsetTime;
            if (slot.pCampath->CalculateFrame(&check) == FreeCameraPath::CalResult::NoFrame) {
                invalidFrame = true;
                break;
            }
        }
        if (invalidFrame) this->ClearTracked("failed", "no_frame");
    }
    for (auto slot = this->playslots.begin();slot != this->playslots.end();) {
        IO->ElementTime = time + slot->offsetTime;
        IO->FrameGameTime = time;
        auto calResult = slot->pCampath->CalculateFrame(IO);
        if (calResult == FreeCameraPath::CalResult::Back && slot->isActiveMode == false) {
            this->LogInfo(std::format("移除播放结束的运镜：{}", slot->pCampath->GetName()));
            slot = this->playslots.erase(slot);
            continue;
        }
        if (calResult == FreeCameraPath::CalResult::In) {
            trackedIn = slot->tracked;
            ret = true;
        }
        ++slot;
    }
    if (trackedIn && !this->started && this->camOverride.load(std::memory_order_acquire)) {
        this->started = true;
        this->TrackedState("started");
    }

    if (this->tracked && std::none_of(this->playslots.begin(), this->playslots.end(),
        [](const auto& slot) { return slot.tracked; })) {
        this->TrackedState(this->started ? "completed" : "failed", this->started ? "" : "no_frame");
        this->tracked = false;
    }

    if (ret) {
        auto frame = this->drawCamera.Write();
        *frame = IO->Frame;
        this->needDrawCamera.store(true, std::memory_order_release);
    }
    else {
        this->needDrawCamera.store(false, std::memory_order_release);
    }
    ret = ret && this->camOverride.load(std::memory_order_acquire);
    return ret;
}

void CamPlayScheduler::PostEvent(CamPlayEvent event) {
    auto [msg, out] = MulNX::Message::Create<CamPlayEvent>("CamPlay/Event"_hash);
    *out = std::move(event);
    this->PublishAsync(std::move(msg));
}

void CamPlayScheduler::TrackedState(const char* state, const std::string& reason) {
    CamPlayEvent event{};
    event.type = "CamPlay/State";
    event.requestId = this->requestId;
    event.generation = this->generation;
    event.state = state;
    event.reason = reason;
    this->PostEvent(std::move(event));
}

void CamPlayScheduler::Invalidate() {
    ++this->currentGeneration;
    if (this->tracked) this->ClearTracked("failed", "pack_changed");
}

void CamPlayScheduler::ClearTracked(const char* state, const std::string& reason) {
    std::erase_if(this->playslots, [](const auto& slot) { return slot.tracked; });
    this->needDrawCamera.store(false, std::memory_order_release);
    this->TrackedState(state, reason);
    this->tracked = false;
}
