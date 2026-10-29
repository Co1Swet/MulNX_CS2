#include "CamPlayScheduler.hpp"
#include <CameraSystem/CampathManager/CampathManager.hpp>
#include <CameraSystem/CamMacroManager/CamMacroManager.hpp>
#include <CameraSystem/CamPackManager/CamPackManager.hpp>
#include <nlohmann/json.hpp>

bool CamPlayScheduler::Init() {
    this->pCampathManager = this->FindModule<CampathManager>("CampathManager");
    this->pCamMacroManager = this->FindModule<CamMacroManager>("CamMacroManager");
    this->pCamPackManager = this->FindModule<CamPackManager>("CamPackManager");
    this->pCamDrawer = &this->FindModule<CameraSystem>("CameraSystem")->CamDrawer;

    (*this)
        .SubscribeAsync("CamPlay/Request")
        .SubscribeAsyncEx<uint64_t, uint64_t>("CamPlay/Tracked")
        .SubscribeAsync<uint64_t, uint64_t>("CamPlay/CancelTracked")
        .SubscribeAsync<uint64_t>("CamPlay/StatusTracked")
        .SubscribeAsyncEx<uint64_t>("CamPack/CheckMacro")
        .SubscribeAsync<void>("CamPlay/Clear")
        .SubscribeAsync<void>("CamPlay/ClearForce")

        .SubscribeAsync<void>("CamPlay/Draw/Enable")
        .SubscribeAsync<void>("CamPlay/Draw/Disable")
        .SubscribeAsync<void>("CamPlay/Override/Enable")
        .SubscribeAsync<void>("CamPlay/Override/Disable")
        ;

    this->SubscribeSync("System/Init/End", [this](auto&&...) {
        this->UIRegisterBackground("DrawPlayCamera", [this](auto&&...) {
            this->DrawPlayCamera();
            });
        });

    return true;
}

void CamPlayScheduler::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "CamPack/CheckMacro"_hash: {
        const auto* ext = msg.asp.get<MulNX::NetExt>();
        auto&& [id] = msg.Access<uint64_t>();
        nlohmann::json out{{"type", "CamPack/CheckMacro"}, {"requestId", id},
            {"macro", ext ? ext->str1 : ""}, {"expectedPack", ext ? ext->str2 : ""}};
        std::string reason;
        std::vector<PlaySlot> slots;
        {
            std::shared_lock lock(this->pCamPackManager->smutex);
            auto pack = this->pCamPackManager->pActiveCamPack;
            if (!ext || !pack || pack->GetName() != ext->str2) reason = "pack_mismatch";
            else this->ResolveMacro(ext->str1, slots, reason);
        }
        out["available"] = reason.empty();
        out["reason"] = reason;
        this->WebPost(out.dump());
        break;
    }
    case "CamPlay/Tracked"_hash: {
        const auto* ext = msg.asp.get<MulNX::NetExt>();
        auto&& [id, gen] = msg.Access<uint64_t, uint64_t>();
        if (this->tracked) {
            nlohmann::json out{{"type", "CamPlay/State"}, {"requestId", id},
                {"generation", gen}, {"state", "failed"}, {"reason", "busy"}};
            this->WebPost(out.dump());
            break;
        }
        this->requestId = id;
        this->generation = gen;
        this->started = false;
        std::vector<PlaySlot> slots;
        std::string reason;
        {
            std::shared_lock lock(this->pCamPackManager->smutex);
            auto pack = this->pCamPackManager->pActiveCamPack;
            if (this->hasCancellation && gen <= this->cancelledGeneration) reason = "cancelled_generation";
            else if (!ext || !pack || pack->GetName() != ext->str2) reason = "pack_mismatch";
            else {
                this->expectedPack = ext->str2;
                this->macroName = ext->str1;
                this->packRevision = this->pCamPackManager->revision;
                this->ResolveMacro(ext->str1, slots, reason);
            }
        }
        if (!reason.empty()) {
            this->TrackedState("failed", reason);
            break;
        }
        this->tracked = true;
        this->playslots.insert(this->playslots.end(), slots.begin(), slots.end());
        break;
    }
    case "CamPlay/CancelTracked"_hash: {
        auto&& [id, gen] = msg.Access<uint64_t, uint64_t>();
        if (!this->hasCancellation || gen > this->cancelledGeneration) this->cancelledGeneration = gen;
        this->hasCancellation = true;
        if (this->tracked && this->generation <= gen) this->ClearTracked("cancelled", "cancelled");
        nlohmann::json out{{"type", "CamPlay/CancelTracked"}, {"requestId", id},
            {"generation", gen}, {"state", "cancelled"}};
        this->WebPost(out.dump());
        break;
    }
    case "CamPlay/StatusTracked"_hash: {
        auto&& [id] = msg.Access<uint64_t>();
        nlohmann::json out{{"type", "CamPlay/StatusTracked"}, {"requestId", id}, {"current", nullptr}};
        if (this->tracked) out["current"] = {{"requestId", this->requestId},
            {"generation", this->generation}, {"state", this->started ? "started" : "pending"},
            {"expectedPack", this->expectedPack}, {"macro", this->macroName}};
        this->WebPost(out.dump());
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
        if (this->tracked) {
            if (!this->hasCancellation || this->generation > this->cancelledGeneration)
                this->cancelledGeneration = this->generation;
            this->hasCancellation = true;
            this->ClearTracked("cancelled", "clear_force");
        }
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
        break;
    }
    }
}

bool CamPlayScheduler::HandleUpdate(CameraSystemIO* IO) {
    this->Update();

    if (this->tracked) {
        std::shared_lock lock(this->pCamPackManager->smutex);
        if (this->pCamPackManager->revision != this->packRevision)
            this->ClearTracked("failed", "pack_changed");
    }
    if (this->playslots.empty()) {
        this->needDrawCamera = false;
        return false;
    }

    bool ret = false;
    bool invalidFrame = false;
    for (auto slot = this->playslots.begin();slot != this->playslots.end();) {
        IO->ElementTime = this->pTimeline->GetTime();
        IO->FrameGameTime = this->pTimeline->GetTime();

        IO->ElementTime += slot->offsetTime;
        auto calResult = slot->pCampath->CalculateFrame(IO);
        if (calResult == FreeCameraPath::CalResult::Back && slot->isActiveMode == false) {
            this->LogInfo(std::format("移除播放结束的运镜：{}", slot->pCampath->GetName()));
            slot = this->playslots.erase(slot);
            continue;
        }
        if (slot->tracked && calResult == FreeCameraPath::CalResult::NoFrame) {
            invalidFrame = true;
            break;
        }
        if (calResult == FreeCameraPath::CalResult::In) {
            if (slot->tracked && !this->started) {
                this->started = true;
                this->TrackedState("started");
            }
            ret = true;
        }
        ++slot;
    }

    if (invalidFrame) {
        this->ClearTracked("failed", "no_frame");
        ret = false;
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

void CamPlayScheduler::TrackedState(const char* state, const std::string& reason) {
    nlohmann::json out{{"type", "CamPlay/State"}, {"requestId", this->requestId},
        {"generation", this->generation}, {"state", state}, {"reason", reason}};
    this->WebPost(out.dump());
}

void CamPlayScheduler::ClearTracked(const char* state, const std::string& reason) {
    std::erase_if(this->playslots, [](const auto& slot) { return slot.tracked; });
    this->needDrawCamera.store(false, std::memory_order_release);
    this->TrackedState(state, reason);
    this->tracked = false;
}

bool CamPlayScheduler::ResolveMacro(const std::string& name, std::vector<PlaySlot>& slots, std::string& reason) {
    std::vector<WrapCampath> paths;
    if (!this->pCamMacroManager->GetMacroPaths(name, paths)) {
        reason = "macro_not_found";
        return false;
    }
    if (paths.empty()) {
        reason = "macro_empty";
        return false;
    }
    std::shared_lock lock(this->pCampathManager->smutex);
    for (const auto& path : paths) {
        auto campath = this->pCampathManager->FindCampath(path.campathName);
        if (!campath || campath->CameraKeyframes.size() < 2) {
            reason = "path_unavailable";
            return false;
        }
        PlaySlot slot{};
        slot.offsetTime = campath->GetStartTime() - this->pTimeline->GetTime() - path.offset;
        slot.pCampath = std::move(campath);
        slot.tracked = true;
        slots.push_back(std::move(slot));
    }
    return true;
}
