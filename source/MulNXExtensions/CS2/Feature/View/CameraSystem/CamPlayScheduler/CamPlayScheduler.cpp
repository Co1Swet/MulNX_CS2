#include "CamPlayScheduler.hpp"
#include <CameraSystem/ElementManager/ElementManager.hpp>

void CamPlayScheduler::DrawPlayCamera() {
    if (!this->needDrawCamera.load(std::memory_order_acquire))return;

    auto frame = this->drawCamera.Read();
    this->pCamDrawer->DrawFrameCamera(*frame, "当前播放摄像机");
}

bool CamPlayScheduler::Init() {
    this->pEManager = this->FindModule<ElementManager>("ElementManager");
    this->pCamDrawer = &this->FindModule<CameraSystem>("CameraSystem")->CamDrawer;

    (*this)
        .SubscribeAsync("CamPlay/Request")
        .SubscribeAsync("CamPlay/Clear")
        .SubscribeAsync("CamPlay/ClearForce")
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
    case "CamPlay/Request"_hash: {
        auto request = msg.asp.get<CamPlayRequest>();
        auto& name = request->campathName;
        std::shared_ptr<const FreeCameraPath> pCampath = this->pEManager->FindCampath(name);
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
        for (auto& slot : this->playslots) {
            this->LogInfo(std::format("因强制清理请求移除运镜：{}", slot.pCampath->GetName()));
        }
        this->playslots.clear();
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
        if (calResult == FreeCameraPath::CalResult::In) {
            ret = true;
        }
        ++slot;
    }

    if (ret) {
        auto frame = this->drawCamera.Write();
        *frame = IO->Frame;
        this->needDrawCamera.store(true, std::memory_order_release);
    }
    else {
        this->needDrawCamera.store(false, std::memory_order_release);
    }
    return ret;
}