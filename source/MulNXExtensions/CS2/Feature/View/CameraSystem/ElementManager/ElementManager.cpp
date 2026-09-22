#include "ElementManager.hpp"
#include <Intro/HookView/HookView.hpp>
#include <CameraSystem/CameraSystem.hpp>
#include <CameraSystem/CameraDrawer/CameraDrawer.hpp>
#include <CameraSystem/SolutionManager/SolutionManager.hpp>
#include <CameraSystem/CamPlayScheduler/CamPlayRequest.hpp>

//元素管理器基本函数
bool ElementManager::Init() {
    this->CamDrawer = &this->FindModule<CameraSystem>("CameraSystem")->CamDrawer;
    this->SManager = this->FindModule<SolutionManager>("SolutionManager");
    this->pIPCer = this->FindModule<MulNX::IPCer>("IPCer");

    this->SendUIRoot(this->GetName(), [this](auto&&...) {return this->UINodeFunc();});

    auto* PathManager = this->Path();
    PathManager->CreateKey("kCampaths", "Campaths", [this](MulNX::PathManager* PathManager)->bool {
        auto Path = PathManager->PathGetFromKey("kCampaths");
        this->LogSucc("成功设置元素路径为：" + Path.string());
        return true;
        });
    PathManager->KeyBindDynamic("kCampaths", "kCurrentPack");

    (*this)
        .SubscribeAsync("Element/Create")
        .SubscribeAsync("Element/Delete")
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
        this->Element_ClearAll();
        });

    this->SubscribeSync("CamSync/SaveAll", [this](auto&&...) {
        this->Element_SaveAll();
        });

    this->SubscribeSync("CamSync/Load", [this](auto&&...) {
        //获取元素文件夹路径
        std::filesystem::path ElementsPath = this->Path()->PathGetFromKey("kCampaths");
        std::vector<std::string>Elements = this->pIPCer->GetFileNamesByPath(ElementsPath);
        //遍历加载元素
        for (const std::string& Element : Elements) {
            this->Element_Load(ElementsPath / Element);
        }
        this->LogSucc(std::format("尝试加载元素总数：{}", Elements.size()));
        this->LogSucc(std::format("成功加载元素总数：", this->elements.size()));
        });

    return true;
}

void ElementManager::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Element/Create"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        if (!this->Element_Create(name)) {
            this->LogError(std::format("元素创建失败：{}", name));
        }
        break;
    }
    case "Element/Delete"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        if (!this->Element_Delete(name)) {
            this->LogError(std::format("元素删除失败：{}", name));
        }
        break;
    }
    case "Campath/OpenDebug"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        auto pCampath = this->FindCampath(name);
        if (!pCampath)break;
        this->CurrentElement.store(pCampath, std::memory_order_release);
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

bool ElementManager::HandleUpdate(CameraSystemIO* IO) {
    this->Update();
    return false;
}

std::shared_ptr<FreeCameraPath> ElementManager::FindCampath(const std::string& name) {
    auto it = this->elements.find(name);
    if (it == this->elements.end())return nullptr;
    return it->second;
}

//创建元素函数，支持传递任意参数给元素构造函数
FreeCameraPath* ElementManager::Element_Create(const std::string& name) {
    // 检查是否已存在同名元素
    if (this->elements.find(name) != this->elements.end()) {
        this->LogError("元素名已占用！ 元素名：" + name);
        return nullptr;
    }
    std::shared_ptr<FreeCameraPath> pElement = nullptr;
    pElement = std::make_shared<FreeCameraPath>(name);
    // 输出成功信息
    this->LogSucc("成功创建元素！  元素名：" + name);
    // 添加进Elements
    this->elements[name] = std::move(pElement);
    return this->elements[name].get();
}

bool ElementManager::Element_SaveAll() {
    //检查是否有元素
    if (this->elements.empty()) {
        this->LogWarning("当前没有任何元素，跳过保存操作！");
        return true;
    }
    std::filesystem::path ElementFolderPath = this->Path()->PathGetFromKey("kCampaths");
    //遍历所有元素并保存
    for (const auto& [name, elem] : this->elements) {
        if (!elem->Dirty) {
            //如果不脏则跳过保存
            continue;
        }
        auto [ok, msg] = elem->Save(ElementFolderPath);
        if (ok) {
            this->LogSucc(std::move(msg));
        }
        else {
            this->LogError(std::move(msg));
            return false;
        }
    }
    this->LogSucc("成功保存所有元素到磁盘！");
    return true;
}
bool ElementManager::Element_Load(const std::filesystem::path& FullPath) {
    this->LogInfo("尝试从磁盘文件加载元素，文件路径：" + FullPath.string());
    // 检查文件本身存在性
    if (!std::filesystem::exists(FullPath)) {
        this->LogError("磁盘文件不存在！文件路径：" + FullPath.string());
        return false;
    }

    try {
        YAML::Node root = YAML::LoadFile(FullPath.string());

        // 获取元素名称
        std::string NewElementName = root["name"].as<std::string>();
        // 检查元素名是否为空
        if (NewElementName.empty()) {
            this->LogError("尝试从磁盘文件加载元素失败，元素名称为空！");
            return false;
        }
        // 检查是否存在同名元素
        if (this->elements.find(NewElementName) != this->elements.end()) {
            this->LogError("元素名已占用，无法从磁盘文件加载元素！ 元素名：" + NewElementName);
            return false;
        }
        // 创建基类指针
        this->LogInfo("加载元素文件路径：" + FullPath.string());

        auto pElement = this->Element_Create(NewElementName);
        // 判空
        if (!pElement) {
            this->LogError("尝试从磁盘文件加载元素失败，无法创建指定类型的元素实例");
            return false;
        }
        // 统一加载信息
        auto [ok, msg] = pElement->Load(root);
        if (!ok) {
            this->LogError(std::move(msg));
            return false;
        }
        pElement->Refresh();
        pElement->Dirty = false;// 刚刚进入内存，非脏
        this->LogSucc(std::move(msg));
        return true;
    }
    catch (...) {
        MulNX::ErrorTerminate("元素加载异常");
    }
}
bool ElementManager::Element_Delete(const std::string Name) {
    // 安全检查
    if (Name.empty()) {
        this->LogError("尝试删除空名称的元素！");
        return false;
    }

    // 获取迭代器
    auto it = this->elements.find(Name);
    // 判空
    if (it == this->elements.end()) {
        this->LogError("未找到指定名称的元素：" + Name);
        return false;
    }

    // 检查是否当前正在操作此元素
    auto current = this->CurrentElement.load(std::memory_order_acquire);
    if (current && current->GetName() == Name) {
        this->CurrentElement = nullptr;
    }

    // 先标记为需要清理
    it->second->NeedBeDelete = true;
    // 通过迭代器删除元素
    this->elements.erase(it);
    // 添加刷新信息
    this->PublishAsync("CameraSystem/Element/Deleted"_hash);

    this->LogSucc("成功删除元素：" + Name);
    return true;
}
bool ElementManager::Element_ClearAll() {
    // 检查是否有元素
    if (this->elements.empty()) {
        this->LogWarning("当前没有任何元素，跳过清空操作！");
        return true;
    }
    // 清空当前操作元素
    this->CurrentElement = nullptr;
    // 把所有元素标记为需要清理并从Elements中释放
    for (auto& [name, elem] : this->elements) {
        elem->NeedBeDelete = true;
    }
    this->elements.clear();
    // 添加刷新信息
    this->PublishAsync("CameraSystem/Element/Deleted"_hash);
    this->LogSucc("成功清空所有元素！");
    return true;
}