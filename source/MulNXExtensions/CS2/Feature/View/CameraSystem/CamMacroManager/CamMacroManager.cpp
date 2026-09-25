#include "CamMacroManager.hpp"
#include <CameraSystem/CameraSystem.hpp>

bool CamMacroManager::Init() {
    this->pIPCer = this->FindModule<MulNX::IPCer>("IPCer");

    this->SendUIRoot(this->GetName(), [this](auto&&...) {return this->UI();});

    this->Path()->CreateKey("kCamMacros", "CamMacros", [this](MulNX::PathManager* PathManager)->bool {
        auto Path = PathManager->PathGetFromKey("kCamMacros");
        this->LogSucc("成功设置运镜宏路径为：" + Path.string());
        return true;
        });
    this->Path()->KeyBindDynamic("kCamMacros", "kCurrentPack");

    (*this)
        .SubscribeAsync("CamMacro/Create")
        .SubscribeAsync("CamMacro/Delete")
        .SubscribeAsync("CamMacro/Play")
        .SubscribeAsync("CamMacro/OpenDebug")
        .SubscribeAsync("CamMacro/DebugKCP")
        .SubscribeAsync("CamMacro/ClearOne")
        .SubscribeAsync("CamMacro/SaveOne")
        .SubscribeAsync("CamMacro/AddCampath")
        .SubscribeAsync("CamMacro/RemoveCampath")
        ;

    this->SubscribeSync("CamSync/Clear", [this](auto&&...) {
        this->CamMacroClearAll();
        });

    this->SubscribeSync("CamSync/SaveAll", [this](auto&&...) {
        this->CamMacroSaveAll();
        });

    this->SubscribeSync("CamSync/Load", [this](auto&&...) {
        auto dirCamMacros = this->Path()->PathGetFromKey("kCamMacros");
        auto camMacroNames = this->pIPCer->GetFileNamesByPath(dirCamMacros);
        for (const auto& camMacroName : camMacroNames) {
            this->CamMacroLoad(dirCamMacros / camMacroName);
        }
        this->LogInfo(std::format("尝试加载运镜宏总数：{}", camMacroNames.size()));
        this->LogSucc(std::format("成功加载运镜宏总数：{}" ,this->camMacros.size()));
        });

    return true;
}

void CamMacroManager::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "CamMacro/Create"_hash: {
        auto name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        if (!this->CamMacroCreate(name)) {
            this->LogError(std::format("创建运镜宏失败：{}", name));
        }
        break;
    }
    case "CamMacro/Delete"_hash: {
        auto name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        if (!this->CamMacroDelete(name)) {
            this->LogError(std::format("删除运镜宏失败：{}", name));
        }
        break;
    }
    case "CamMacro/Play"_hash: {
        auto name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        this->PlayCamMacro(name);
        break;
    }
    case "CamMacro/OpenDebug"_hash: {
        std::unique_lock lock(this->smutex);
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        auto pCamMacro = this->FindCamMacro(name);
        if (!pCamMacro)break;
        this->pOperatingMacro = pCamMacro;
        this->showWindow.store(true, std::memory_order_release);
        break;
    }
    case "CamMacro/DebugKCP"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        const auto* pCamMacro = this->FindCamMacro(name);
        if (!pCamMacro)break;
        this->bufKCPack = pCamMacro->GetKeyCheckPack().load();
        this->bKCPWindow = true;
        break;
    }
    case "CamMacro/ClearOne"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        auto* pCamMacro = this->FindCamMacro(name);
        if (!pCamMacro)break;
        pCamMacro->Clear();
        this->LogSucc(std::format("成功清空宏 {} 的所有运镜", name));
        break;
    }
    case "CamMacro/SaveOne"_hash: {
        std::shared_lock lock(this->smutex);
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        const auto* pCamMacro = this->FindCamMacro(name);
        if (!pCamMacro)break;
        auto path = this->Path()->PathGetFromKey("kCamMacros");
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
        auto pNetExt = msg.asp.get<MulNX::NetExt>();
        auto&& [offset] = msg.Access<float>();
        std::unique_lock lock(this->smutex);
        auto* pCamMacro = this->FindCamMacro(pNetExt->str1);
        if (!pCamMacro)break;
        if (pCamMacro->AddCampath(pNetExt->str2, offset)) {
            this->LogSucc(std::format("成功添加运镜到宏 。宏：{}，运镜：{}",
                pNetExt->str1, pNetExt->str2));
        }
        else {
            this->LogError(std::format("无法添加运镜到宏，可能是运镜已存在于宏中。宏：{}，运镜：{}",
                pNetExt->str1, pNetExt->str2));
        }
        break;
    }
    case "CamMacro/RemoveCampath"_hash: {
        std::unique_lock lock(this->smutex);
        auto pNetExt = msg.asp.get<MulNX::NetExt>();
        auto* pCamMacro = this->FindCamMacro(pNetExt->str1);
        if (!pCamMacro)break;
        pCamMacro->RemoveCampath(pNetExt->str2);
        break;
    }
    }
}

void CamMacroManager::HandleUpdate() {
    this->Update();
    if (!this->shortcutEnable)return;
    for (const auto& [name, pCamMacro] : this->camMacros) {
        if (this->pInputSystem->CheckWithPack(pCamMacro->GetKeyCheckPack())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/Play"_hash);
            rp->str1 = pCamMacro->GetName();
            this->PublishAsync(std::move(msg));
        }
    }
}

CamMacro* CamMacroManager::FindCamMacro(const std::string& name) {
    auto it = this->camMacros.find(name);
    if (it == this->camMacros.end())return nullptr;
    return it->second.get();
}

bool CamMacroManager::CamMacroCreate(const std::string& name) {
    if (this->camMacros.find(name) != this->camMacros.end()) {
        this->LogError(std::format("运镜宏已占用：{}" , name));
        return false;
    }
    this->LogSucc(std::format("成功创建运镜宏：{}" , name));
    auto newCamMacro = std::make_unique<CamMacro>(name);
    this->camMacros[name] = std::move(newCamMacro);
    return true;
}
bool CamMacroManager::CamMacroSaveAll() {
    if (this->camMacros.empty()) {
        this->LogInfo("当前无运镜宏，跳过保存");
        return true;
    }
    auto dirMacros = this->Path()->PathGetFromKey("kCamMacros");
    for (const auto& [name, pCamMacor] : this->camMacros) {
        auto [ok, msg] = pCamMacor->Save(dirMacros);
        if (!ok) {
            this->LogError(std::move(msg));
            return false;
        }
        this->LogSucc(std::move(msg));
    }
    this->LogSucc("成功保存所有运镜宏到文件！");
    return true;
}
bool CamMacroManager::CamMacroLoad(const std::filesystem::path& pathMacro) {
    this->LogInfo(std::format("尝试从yaml文件加载运镜宏：{}", pathMacro.string()));
    if (!std::filesystem::exists(pathMacro)) {
        this->LogError(std::format("yaml文件不存在：", pathMacro.string()));
        return false;
    }
    try {
        YAML::Node root = YAML::LoadFile(pathMacro.string());
        std::string newCamMacroName = root["name"].as<std::string>();
        if (newCamMacroName.empty()) {
            this->LogError("尝试从yaml文件加载运镜宏失败，名称为空！");
            return false;
        }
        if (this->camMacros.find(newCamMacroName) != this->camMacros.end()) {
            this->LogError(std::format("运镜宏已占用，无法磁盘加载：{}", newCamMacroName));
            return false;
        }
        auto newCamMacro = std::make_unique<CamMacro>(newCamMacroName);
        auto [ok, msg] = newCamMacro->Load(root);
        if (!ok) {
            this->LogError(std::move(msg));
            return false;
        }
        this->camMacros[newCamMacroName] = std::move(newCamMacro);
        this->LogSucc(std::move(msg));
        this->LogLine();
        return true;
    }
    catch (const std::exception& e) {
        this->LogError(std::format("在加载yaml时遇到异常：{}" ,e.what()));
        return false;
    }
}

bool CamMacroManager::CamMacroDelete(const std::string& name) {
    if (name.empty()) {
        this->LogError("尝试删除空名称的运镜宏！");
        return false;
    }
    auto it = this->camMacros.find(name);
    if (it == this->camMacros.end()) {
        this->LogError(std::format("未找到指定名称的运镜：{}" , name));
        return false;
    }
    if (this->pOperatingMacro) {
        if (this->pOperatingMacro == it->second.get())
            this->pOperatingMacro = nullptr;
    }
    this->camMacros.erase(it);
    this->LogSucc(std::format("成功删除运镜宏：", name));
    return true;
}
bool CamMacroManager::CamMacroClearAll() {
    this->pOperatingMacro = nullptr;
    if (this->camMacros.empty()) {
        this->LogWarning("当前没有任何运镜宏，跳过清空操作！");
        return true;
    }
    this->camMacros.clear();
    this->LogSucc("成功删除所有运镜宏！");
    return true;
}
void CamMacroManager::PlayCamMacro(const std::string& name) {
    auto it = this->camMacros.find(name);
    if (it == this->camMacros.end()) {
        this->LogError(std::format("目标运镜宏不存在：{}", name));
        return;
    }
    this->PublishAsync("CameraSystem/Play/Started"_hash);
    for (const auto& item : it->second->GetVec()) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Campath/Preview"_hash);
        auto&& [previewOffset] = msg.Access<float>();
        previewOffset = 0.0f - item.offset;
        rp->str1 = item.campathName;
        this->PublishAsync(std::move(msg));
    }
    this->LogInfo(std::format("播放运镜宏：{}", name));
}