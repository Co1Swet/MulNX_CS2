#include "CamPlayScheduler.hpp"
#include <CameraSystem/ElementManager/ElementManager.hpp>

bool CamPlayScheduler::Init() {
    this->pEManager = this->FindModule<ElementManager>("ElementManager");

    (*this)
        .SubscribeAsync("CamPlay/Request")
        ;

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
        slot.offestTime = request->offsetTime;
        this->playslots.push_back(std::move(slot));
        break;
    }
    }
    
}

bool CamPlayScheduler::HandleUpdate(CameraSystemIO* IO) {
    this->Update();
    if (this->playslots.empty())return false;
    bool ret = false;
    for (auto slot = this->playslots.begin();slot != this->playslots.end();) {
        IO->ElementTime = this->pTimeline->GetTime();
        IO->FrameGameTime = this->pTimeline->GetTime();

        IO->ElementTime += slot->pCampath->GetStartTime() - slot->offestTime;
        if (!slot->pCampath->CalculateFrame(IO)) {
            this->LogInfo(std::format("移除播放结束的运镜：{}", slot->pCampath->GetName()));
            slot = this->playslots.erase(slot);
            continue;
        }
        ret = true;
        ++slot;
    }
    return ret;
}