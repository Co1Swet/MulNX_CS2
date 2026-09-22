#include "CampathManager.hpp"
#include <Intro/HookView/HookView.hpp>
#include <CameraSystem/CameraDrawer/CameraDrawer.hpp>
#include <CameraSystem/CamPlayScheduler/CamPlayRequest.hpp>

bool CampathManager::Init() {
    this->CamDrawer = &this->FindModule<CameraSystem>("CameraSystem")->CamDrawer;
    this->pIPCer = this->FindModule<MulNX::IPCer>("IPCer");

    this->SendUIRoot(this->GetName(), [this](auto&&...) {return this->UI();});

    auto* PathManager = this->Path();
    PathManager->CreateKey("kCampaths", "Campaths", [this](MulNX::PathManager* PathManager)->bool {
        auto Path = PathManager->PathGetFromKey("kCampaths");
        this->LogSucc("成功设置运镜轨道资源路径为：" + Path.string());
        return true;
        });
    PathManager->KeyBindDynamic("kCampaths", "kCurrentPack");

    (*this)
        .SubscribeAsync("Campath/Create")
        .SubscribeAsync("Campath/Delete")
        .SubscribeAsync("Campath/OpenDebug")
        .SubscribeAsync("Campath/AddKeyframe")
        .SubscribeAsync("Campath/DeleteKeyframe")
        .SubscribeAsync("Campath/CopyKeyframe")
        .SubscribeAsync("Campath/ClearOne")
        .SubscribeAsync("Campath/Preview")
        .SubscribeAsync("Campath/Active")
        .SubscribeAsync("Campath/DrawOne")
        .SubscribeAsync("Campath/UnDrawOne")
        ;

    this->SubscribeSync("CamSync/Clear", [this](auto&&...) {
        this->CampathClearAll();
        });

    this->SubscribeSync("CamSync/SaveAll", [this](auto&&...) {
        this->CampathSaveAll();
        });

    this->SubscribeSync("CamSync/Load", [this](auto&&...) {
        std::filesystem::path dirCampaths = this->Path()->PathGetFromKey("kCampaths");
        std::vector<std::string> campathNames = this->pIPCer->GetFileNamesByPath(dirCampaths);
        for (const std::string& campathName : campathNames) {
            if (!this->CampathLoad(dirCampaths / campathName)) {
                this->LogError(std::format("运镜轨道加载失败：{}", campathName));
            }
        }
        this->LogInfo(std::format("尝试加载运镜轨道总数： {}", campathNames.size()));
        this->LogSucc(std::format("成功加载运镜轨道总数： {}", this->campaths.size()));
        if (campathNames.size() != this->campaths.size()) {
            this->LogError("存在加载失败！");
        }
        });

    return true;
}

void CampathManager::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Campath/Create"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        if (!this->CampathCreate(name)) {
            this->LogError(std::format("运镜轨道创建失败：{}", name));
        }
        break;
    }
    case "Campath/Delete"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        if (!this->CampathDelete(name)) {
            this->LogError(std::format("运镜轨道删除失败：{}", name));
        }
        break;
    }
    case "Campath/OpenDebug"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        auto pCampath = this->FindCampath(name);
        if (!pCampath)break;
        this->pOperatingCampath.store(pCampath, std::memory_order_release);
        this->showWindow.store(true, std::memory_order_release);
        break;
    }
    case "Campath/AddKeyframe"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        auto pCampath = this->FindCampath(name);
        if (!pCampath)break;
        auto&& [t, x, y, z, fov, rx, ry, rz, d1, d2, d3, d4] =
            msg.Access<float, float, float, float, float, float, float, float, float, float, float, float>();
        MulNX::Math::View view{};
        view.position = { x,y,z };
        view.rotation = { rx,ry,rz };
        view.FOV = fov;
        view.dof.NearBlurry = d1;
        view.dof.NearCrisp = d2;
        view.dof.FarCrisp = d3;
        view.dof.FarBlurry = d4;
        MulNX::Math::CameraKeyframe keyframe{};
        keyframe.PositionAndFOV = view.ToPositionAndFOV();
        keyframe.RotationQuat = view.ToRotationQuat();
        keyframe.dof = view.ToDOFPack();
        keyframe.time = t;
        pCampath->AddKeyframe(std::move(keyframe));
        break;
    }
    case "Campath/DeleteKeyframe"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        auto pCampath = this->FindCampath(name);
        if (!pCampath)break;
        auto&& [index] = msg.Access<size_t>();
        pCampath->CameraKeyframes.erase(pCampath->CameraKeyframes.begin() + index);
        pCampath->Refresh();
        break;
    }
    case "Campath/CopyKeyframe"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        auto pCampath = this->FindCampath(name);
        if (!pCampath)break;
        auto&& [index] = msg.Access<size_t>();
        pCampath->AddKeyframe(pCampath->GetKeyFrame(index));
        break;
    }
    case "Campath/ClearOne"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        auto pCampath = this->FindCampath(name);
        if (!pCampath)break;
        pCampath->Clear();
        break;
    }
    case "Campath/Preview"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::shared_lock lock(this->smutex);
        auto pCampath = this->FindCampath(name);
        if (!pCampath)break;
        auto&& [previewOffset] = msg.Access<float>();

        auto [play, rp] = MulNX::Message::Create<CamPlayRequest>("CamPlay/Request"_hash);
        rp->campathName = name;
        rp->offsetTime = pCampath->GetStartTime() - this->pTimeline->GetTime();
        rp->offsetTime += previewOffset;
        rp->isActiveMode = false;
        this->PublishAsync(std::move(play));
        break;
    }
    case "Campath/Active"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::shared_lock lock(this->smutex);
        auto pCampath = this->FindCampath(name);
        if (!pCampath)break;
        auto [play, rp] = MulNX::Message::Create<CamPlayRequest>("CamPlay/Request"_hash);
        rp->campathName = name;
        rp->offsetTime = 0;
        rp->isActiveMode = true;
        this->PublishAsync(std::move(play));
        break;
    }
    case "Campath/DrawOne"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        auto pCampath = this->FindCampath(name);
        if (!pCampath)break;
        pCampath->draw = true;
        break;
    }
    case "Campath/UnDrawOne"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        auto pCampath = this->FindCampath(name);
        if (!pCampath)break;
        pCampath->draw = false;
        break;
    }
    }
}

void CampathManager::HandleUpdate() {
    this->Update();
}

std::shared_ptr<FreeCameraPath> CampathManager::FindCampath(const std::string& name,
    std::source_location where) {
    auto it = this->campaths.find(name);
    if (it == this->campaths.end()) {
        this->LogError(std::format("搜索运镜轨道失败：{}", name), where);
        return nullptr;
    }
    return it->second;
}

FreeCameraPath* CampathManager::CampathCreate(const std::string& name) {
    if (this->campaths.find(name) != this->campaths.end()) {
        this->LogError(std::format("该运镜轨道名已经存在，无法创建：{}", name));
        return nullptr;
    }
    auto pCampath = std::make_shared<FreeCameraPath>(name);
    this->LogSucc(std::format("成功创建运镜轨道：{}", name));
    this->campaths[name] = std::move(pCampath);
    return this->campaths[name].get();
}

bool CampathManager::CampathSaveAll() {
    if (this->campaths.empty()) {
        this->LogInfo("当前没有任何运镜轨道，跳过保存操作");
        return true;
    }
    auto dirCampaths = this->Path()->PathGetFromKey("kCampaths");

    for (const auto& [name, pCampath] : this->campaths) {
        if (!pCampath->IsDirty()) {
            continue;
        }
        auto [ok, msg] = pCampath->Save(dirCampaths);
        if (ok) {
            this->LogSucc(std::move(msg));
        }
        else {
            this->LogError(std::move(msg));
            return false;
        }
    }
    this->LogSucc("成功保存所有运镜轨道到磁盘！");
    return true;
}
bool CampathManager::CampathLoad(const std::filesystem::path& pathCampath) {
    this->LogInfo(std::format("尝试从磁盘文件加载运镜轨道，文件路径：{}", pathCampath.string()));
    // 检查文件本身存在性
    if (!std::filesystem::exists(pathCampath)) {
        this->LogError("磁盘文件不存在！文件路径：" + pathCampath.string());
        return false;
    }

    try {
        YAML::Node root = YAML::LoadFile(pathCampath.string());

        std::string newCampathName = root["name"].as<std::string>();
        if (newCampathName.empty()) {
            this->LogError("尝试从磁盘文件加载运镜轨道失败，名称为空！");
            return false;
        }
        if (this->campaths.find(newCampathName) != this->campaths.end()) {
            this->LogError(std::format("运镜轨道名已占用，无法从磁盘文件加载：{}", newCampathName));
            return false;
        }

        auto pCampath = this->CampathCreate(newCampathName);
        if (!pCampath) {
            this->LogError(std::format("尝试从磁盘文件加载运镜轨道失败，无法实例化：{}", newCampathName));
            return false;
        }
        auto [ok, msg] = pCampath->Load(root);
        if (!ok) {
            this->LogError(std::move(msg));
            return false;
        }
        this->LogSucc(std::move(msg));
        return true;
    }
    catch (const std::exception& e) {
        MulNX::ErrorTerminate(std::format("运镜轨道加载异常：{}", e.what()));
    }
}
bool CampathManager::CampathDelete(const std::string& name) {
    if (name.empty()) {
        this->LogError("尝试删除空名称的运镜轨道！");
        return false;
    }
    auto it = this->campaths.find(name);
    if (it == this->campaths.end()) {
        this->LogError(std::format("未找到要删除的运镜轨道：{}", name));
        return false;
    }

    auto current = this->pOperatingCampath.load(std::memory_order_acquire);
    if (current && current->GetName() == name) {
        this->pOperatingCampath = nullptr;
    }
    this->campaths.erase(it);
    this->LogSucc(std::format("成功删除运镜轨道： {}", name));
    return true;
}
bool CampathManager::CampathClearAll() {
    if (this->campaths.empty()) {
        this->LogInfo("当前没有任何运镜轨道，跳过清空");
        return true;
    }
    this->pOperatingCampath = nullptr;
    this->campaths.clear();
    this->LogSucc("成功清空所有运镜轨道！");
    return true;
}