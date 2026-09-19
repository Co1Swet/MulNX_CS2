#include "SolutionManager.hpp"
#include <CameraSystem/CameraSystem.hpp>

bool SolutionManager::Init() {
    this->pIPCer = this->FindModule<MulNX::IPCer>("IPCer");

    this->SendUIRoot(this->GetName(), [this](auto&&...) {return this->UINodeFunc();});

    auto* PathManager = this->Path();
    PathManager->CreateKey("Solutions", "Solutions", [this](MulNX::PathManager* PathManager)->bool {
        auto Path = PathManager->PathGetFromKey("Solutions");
        this->LogSucc("成功设置解决方案路径为：" + Path.string());
        return true;
        });
    PathManager->KeyBindDynamic("Solutions", "CurrentPack");

    (*this)
        .SubscribeAsync("CameraSystem/Element/Deleted")
        .SubscribeAsync("CameraSystem/Solution/Create")
        .SubscribeAsync("CameraSystem/Solution/Delete")
        .SubscribeAsync("CameraSystem/Solution/Play")
        .SubscribeAsync("CamMacro/OpenDebug")
        .SubscribeAsync("CamMacro/DebugKCP")
        .SubscribeAsync("CamMacro/SaveOne")
        .SubscribeAsync("CamMacro/AddCampath")
        ;

    this->SubscribeSync("CamSync/Clear", [this](auto&&...) {
        this->Solution_ClearAll();
        });

    this->SubscribeSync("CamSync/SaveAll", [this](auto&&...) {
        this->Solution_SaveAll();
        });

    this->SubscribeSync("CamSync/Load", [this](auto&&...) {
        //获取解决方案文件夹路径
        std::filesystem::path SolutionsPath = this->Path()->PathGetFromKey("Solutions");
        std::vector<std::string>Solutions = this->pIPCer->GetFileNamesByPath(SolutionsPath);
        //遍历加载解决方案
        for (const std::string& Solution : Solutions) {
            this->Solution_Load(SolutionsPath / Solution);
        }
        this->LogSucc("尝试加载解决方案总数：" + std::to_string(Solutions.size()));
        this->LogSucc("成功加载解决方案总数：" + std::to_string(this->solutions.size()));
        });

    this->SubscribeSync("CamSync/Play/Shutdown", [this](auto&&...) {
        });

    return true;
}

void SolutionManager::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "CameraSystem/Solution/Create"_hash: {
        auto name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->CamSys->smutex);
        if (!this->Solution_Create(name)) {
            this->LogError(std::format("创建解决方案失败：{}", name));
        }
        break;
    }
    case "CameraSystem/Solution/Delete"_hash: {
        auto name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->CamSys->smutex);
        if (!this->Solution_Delete(name)) {
            this->LogError(std::format("删除解决方案失败：{}", name));
        }
        break;
    }
    case "CameraSystem/Solution/Play"_hash: {
        auto name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->CamSys->smutex);
        this->Playing_Solution(name);
        break;
    }
    case "CameraSystem/Element/Deleted"_hash: {
        //全部刷新用于清理失效元素
        for (auto& [name, pSolution] : this->solutions) {
            //pSolution->Refresh();
        }
        break;
    }
    case "CamMacro/OpenDebug"_hash: {
        std::unique_lock lock(this->smutex);
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        auto pCamMacro = this->FindCamMacro(name);
        if (!pCamMacro)break;
        this->CurrentSolution = pCamMacro;
        this->showWindow.store(true, std::memory_order_release);
        break;
    }
    case "CamMacro/DebugKCP"_hash: {
        std::unique_lock lock(this->smutex);
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        const Solution* pCamMacro = this->FindCamMacro(name);
        if (!pCamMacro)break;
        this->Buffer_KCPack = pCamMacro->KCPack;//缓存
        this->OpenSolutionKCPackDebugWindow = true;//打开窗口
        break;
    }
    case "CamMacro/SaveOne"_hash: {
        std::shared_lock lock(this->smutex);
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        const Solution* pCamMacro = this->FindCamMacro(name);
        if (!pCamMacro)break;
        auto path = this->Path()->PathGetFromKey("Solutions");
        auto [ok, msg] = pCamMacro->Save(path);
        if (ok) {
            this->LogSucc(std::move(msg));
        }
        else {
            this->LogError(std::move(msg));
        }
        break;
    }
    case "CamMacro/AddCampath"_hash: {
        std::unique_lock lock(this->smutex);
        auto pNetExt = msg.asp.get<MulNX::NetExt>();
        Solution* pCamMacro = this->FindCamMacro(pNetExt->str1);
        if (!pCamMacro)break;
        if (pCamMacro->AddElement(pNetExt->str2, 0)) {
            this->LogError(std::format("无法添加运镜到宏，可能是运镜已存在于解决方案中。宏：{}，运镜：{}",
                pNetExt->str1, pNetExt->str2));
        }
        else {
            this->LogSucc(std::format("成功添加运镜到宏 。宏：{}，运镜：{}",
                pNetExt->str1, pNetExt->str2));
        }
        break;
    }
    }
}

bool SolutionManager::HandleUpdate(CameraSystemIO* IO) {
    this->Update();
    return false;
    if (!this->Config.SolutionShortcutEnable)return false;
    //遍历
    for (const auto& [name, pSolution] : this->solutions) {
        //快捷键播放处理
        if (this->pInputSystem->CheckWithPack(pSolution->KCPack)) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Play"_hash);
            rp->str1 = pSolution->name;
            this->PublishAsync(std::move(msg));
        }
    }
    return false;
}
//创建，得到，删除

Solution* SolutionManager::FindCamMacro(const std::string& name) {
    auto it = this->solutions.find(name);
    if (it == this->solutions.end())return nullptr;
    return it->second.get();
}

bool SolutionManager::Solution_Create(const std::string& name) {
    // 检查是否已存在同名解决方案
    if (this->solutions.find(name) != this->solutions.end()) {
        this->LogError("解决方案名已占用！ 解决方案名：" + name);
        return false;
    }
    //输出成功信息
    this->LogSucc("成功创建解决方案！  解决方案名：" + name);
    //创建新解决方案
    std::unique_ptr<Solution> newSolution = std::make_unique<Solution>(name);
    this->solutions[name] = std::move(newSolution);

    return true;
}
bool SolutionManager::Solution_SaveAll() {
    if (this->solutions.empty()) {
        this->LogWarning("尝试在没有任何解决方案的情况下保存");
        return true;
    }
    std::filesystem::path SolutionFolderPath = this->Path()->PathGetFromKey("Solutions");
    //遍历所有解决方案保存
    for (const auto& [name, solution] : this->solutions) {
        if (!solution->dirty) {
            continue;//不脏不需保存
        }
        auto [ok, msg] = solution->Save(SolutionFolderPath);
        if (!ok) {
            this->LogError(std::move(msg));
            return false;
        }
        this->LogSucc(std::move(msg));
    }
    this->LogSucc("成功保存所有解决方案到文件！");
    return true;
}
bool SolutionManager::Solution_Load(const std::filesystem::path& FullPath) {
    // 输出调试信息
    this->LogInfo("尝试从yaml文件加载解决方案，文件路径：" + FullPath.string());
    // 检查文件本身存在性
    if (!std::filesystem::exists(FullPath)) {
        this->LogError("yaml文件不存在！文件路径：" + FullPath.string());
        return false;
    }
    try {
        YAML::Node root = YAML::LoadFile(FullPath.string());
        // 获取解决方案名称并检查是否为空
        std::string NewSolutionName = root["name"].as<std::string>();

        if (NewSolutionName.empty()) {
            this->LogError("尝试从yaml文件加载解决方案失败，解决方案名称为空！");
            return false;
        }

        // 检查是否存在同名解决方案
        if (this->solutions.find(NewSolutionName) != this->solutions.end()) {
            this->LogError("解决方案名已占用，无法从yaml文件加载解决方案！ 解决方案名：" + std::move(NewSolutionName));
            return false;
        }
        // 获取持续时长信息
        float TargetDurationTime = root["duration"].as<float>();
        // 制作解决方案
        auto newSolution = std::make_unique<Solution>(NewSolutionName);
        auto [ok, msg] = newSolution->Load(root);

        if (!ok) {
            this->LogError(std::move(msg));
            return false;
        }

        // 检验时间关系
        if (newSolution->totalDurationTime != TargetDurationTime) {
            this->LogWarning("该解决方案实际持续时长与预估持续时长不同，可能出现问题");
        }

        // 添加进解决方案组
        this->solutions[NewSolutionName] = std::move(newSolution);
        this->LogSucc(std::move(msg));
        this->LogLine();
        return true;
    }
    catch (const YAML::Exception& e) {
        this->LogError("在加载yaml时遇到异常：" + std::string(e.what()));
        return false;
    }
}

bool SolutionManager::Solution_Delete(const std::string& name) {
    //安全检查
    if (name.empty()) {
        this->LogError("尝试删除空名称的解决方案！");
        return false;
    }
    auto it = this->solutions.find(name);
    if (it == this->solutions.end()) {
        this->LogError("未找到指定名称的解决方案：" + name);
        return false;
    }
    //检查是否当前正在操作此解决方案
    if (this->CurrentSolution) {
        if (this->CurrentSolution == it->second.get())
            this->CurrentSolution = nullptr;
    }
    this->solutions.erase(it);
    this->LogSucc("成功删除解决方案：" + name);
    return true;
}
bool SolutionManager::Solution_ClearAll() {
    //清空当前操作解决方案
    this->CurrentSolution = nullptr;

    if (this->solutions.empty()) {
        this->LogWarning("当前没有任何解决方案，跳过清空操作！");
        return true;
    }
    //清空所有解决方案
    this->solutions.clear();
    this->LogSucc("成功删除所有解决方案！");
    return true;
}
void SolutionManager::Playing_Solution(const std::string& name) {
    auto it = this->solutions.find(name);
    if (it == this->solutions.end()) {
        this->LogError(std::format("目标解决方案不存在：{}", name));
        return;
    }

    switch (it->second->playmode) {
    case PlaybackMode::Orchestration:
        it->second->SetSolutionOffset(this->pTimeline->GetTime());//偏移时间轴播放
        this->LogInfo(std::format("偏移时间轴播放，偏移时间设置为：{}", this->pTimeline->GetTime()));
        break;
    case PlaybackMode::Activation:
        it->second->SetSolutionOffset(0);
        break;
    }
    this->PublishAsync("CameraSystem/Play/Started"_hash);

    for (const auto& item : it->second->elements) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Campath/Preview"_hash);
        auto&& [previewOffset] = msg.Access<float>();
        previewOffset = 0.0f - item.Offset;
        rp->str1 = item.campathName;
        this->PublishAsync(std::move(msg));
    }

    this->LogInfo(std::format("播放解决方案：{}", name));
    return;
}