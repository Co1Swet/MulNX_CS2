#include "CamPackManager.hpp"

bool CamPackManager::Init() {
    this->pIPCer = this->Core->ModuleManager()->FindModule<MulNX::IPCer>("IPCer");

    this->SendUIRoot(this->GetName(), [this](auto&&...) {return this->UI();});

    this->Path()->CreateKey("kCurrentPack", {}, [this](MulNX::PathManager* PathManager)->bool {
        auto dirCamPack = PathManager->PathGetFromKey("kCurrentPack");
        if (std::filesystem::exists(dirCamPack)) {
            this->LogSucc(std::format("成功设置运镜包路径为：{}", dirCamPack.string()));
            return true;
        }
        this->LogInfo(std::format("指定的运镜包文件夹不存在，正在创建路径：{}", dirCamPack.string()));
        try {
            std::filesystem::create_directory(dirCamPack);
            std::filesystem::create_directory(dirCamPack / "Campaths");
            std::filesystem::create_directory(dirCamPack / "CamMacros");
        }
        catch (const std::exception& e) {
            this->LogError(std::format("创建运镜包文件夹失败，错误信息：{}", e.what()));
            return false;
        }
        this->LogSucc(std::format("成功创建运镜包文件夹：", dirCamPack.string()));
        return true;
        });
    this->Path()->KeyBindDynamic("kCurrentPack", "kCamPacks");

    (*this)
        .SubscribeAsync("CamPack/Create")
        .SubscribeAsync("CamPack/Apply")
        .SubscribeAsync("CamPack/Delete")
        .SubscribeAsync("CamPack/ClearAll")
        .SubscribeAsync("CamPack/Save")
        .SubscribeAsync("CamPack/OpenDebug")
        .SubscribeAsync("CamPack/CloseDebug")
        .SubscribeAsync("CamPack/ToggleShortcut")
        .SubscribeAsync("CamPack/OpenKCPWindow")
        .SubscribeAsync("Game/NewRound")
        ;

    this->SubscribeSync("System/Init/End", [this](auto&&...) {
        auto dirCamPacks = this->Path()->PathGetFromKey("kCamPacks");
        auto camPackNames = this->pIPCer->GetDirNamesByPath(dirCamPacks);
        if (camPackNames.empty())return;
        for (const auto& camPackName : camPackNames) {
            this->CamPackLoad(dirCamPacks / camPackName, camPackName);
        }

        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamPack/Apply"_hash);
        rp->str1 = "default";
        this->PublishAsync(std::move(msg));
        });

    this->SubscribeSync("CamSync/SaveAll", [this](auto&&...) {
        this->CamPackSave();
        });

    return true;
}

void CamPackManager::HandleUpdate() {
    this->Update();
    if (!this->shortcutEnable.load(std::memory_order_acquire)) return;

    std::shared_ptr<CamPack> toApply = nullptr;
    {
        std::shared_lock lock(this->smutex);
        for (const auto& [name, pCamPack] : this->camPacks) {
            if (this->pInputSystem->CheckWithPack(pCamPack->GetKeyCheckPack())) {
                toApply = pCamPack;
                break;
            }
        }
    }
    if (toApply) {
        std::unique_lock lock(this->smutex);
        this->CamPackApply(toApply);
    }
}

void CamPackManager::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Game/NewRound"_hash: {
        std::shared_lock lock(this->smutex);
        if (!this->pActiveCamPack) break;
        const auto& onNewRound = this->pActiveCamPack->OnNewRound;
        if (onNewRound.empty()) {
            this->LogWarning("无新回合运镜宏可尝试调用");
            break;
        }
        int idx = rand() % onNewRound.size();
        auto [out, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/Play"_hash);
        rp->str1 = onNewRound[idx];
        this->PublishAsync(std::move(out));
        break;
    }
    case "Game/RoundEnd"_hash: {
        break;
    }

    case "CamPack/Create"_hash: {
        std::unique_lock lock(this->smutex);
        const auto* ext = msg.asp.get<MulNX::NetExt>();
        if (ext->str1.empty()) break;
        if (this->camPacks.find(ext->str1) == this->camPacks.end()) {
            this->CamPackCreate(ext->str1);
        }
        auto pack = this->FindCamPack(ext->str1);
        if (pack) this->CamPackApply(pack);
        break;
    }
    case "CamPack/Apply"_hash: {
        std::unique_lock lock(this->smutex);
        const auto* ext = msg.asp.get<MulNX::NetExt>();
        auto pack = this->FindCamPack(ext->str1);
        if (!pack) break;
        this->CamPackApply(pack);
        break;
    }
    case "CamPack/Delete"_hash: {
        std::unique_lock lock(this->smutex);
        const auto* ext = msg.asp.get<MulNX::NetExt>();
        this->CamPackDelete(ext->str1);

        if (this->pOperatingCamPack && this->pOperatingCamPack->GetName() == ext->str1) {
            this->pOperatingCamPack = nullptr;
            this->showWindow.store(false, std::memory_order_release);
        }
        if (this->pActiveCamPack && this->pActiveCamPack->GetName() == ext->str1) {
            this->pActiveCamPack = nullptr;
        }
        break;
    }
    case "CamPack/ClearAll"_hash: {
        std::unique_lock lock(this->smutex);
        this->CamPackClearAll();
        this->pOperatingCamPack = nullptr;
        this->showWindow.store(false, std::memory_order_release);
        break;
    }
    case "CamPack/Save"_hash: {
        std::unique_lock lock(this->smutex);
        this->CamPackSave();
        break;
    }
    case "CamPack/OpenDebug"_hash: {
        std::unique_lock lock(this->smutex);
        const auto* ext = msg.asp.get<MulNX::NetExt>();
        auto pack = this->FindCamPack(ext->str1);
        if (!pack) break;
        this->pOperatingCamPack = pack;
        this->showWindow.store(true, std::memory_order_release);
        break;
    }
    case "CamPack/CloseDebug"_hash: {
        this->showWindow.store(false, std::memory_order_release);
        break;
    }
    case "CamPack/ToggleShortcut"_hash: {
        auto&& [v] = msg.Access<bool>();
        this->shortcutEnable.store(v, std::memory_order_release);
        break;
    }
    case "CamPack/OpenKCPWindow"_hash: {
        std::unique_lock lock(this->smutex);
        if (!this->pOperatingCamPack)break;
        this->bufKCPack = this->pOperatingCamPack->GetKeyCheckPack().load();
        this->bKCPWindow = true;
        break;
    }
    }
}
std::shared_ptr<CamPack> CamPackManager::FindCamPack(const std::string& name) {
    auto it = this->camPacks.find(name);
    if (it == this->camPacks.end())return nullptr;
    return it->second;
}

bool CamPackManager::CamPackDelete(const std::string& name) {
    auto it = this->camPacks.find(name);
    if (it == this->camPacks.end()) {
        return true;
    }
    this->camPacks.erase(it);
    return true;
}
bool CamPackManager::CamPackClearAll() {
    this->camPacks.clear();
    this->pActiveCamPack = nullptr;
    return true;
}

bool CamPackManager::CamPackCreate(const std::string& name) {
    if (this->camPacks.find(name) != this->camPacks.end()) {
        this->LogError(std::format("运镜包已占用：{}", name));
        return false;
    }
    auto pCamPack = std::make_shared<CamPack>(name);
    pCamPack->Refresh();
    this->camPacks[name] = std::move(pCamPack);
    this->LogSucc(std::format("成功创建运镜包：{}", name));
    return true;
}
bool CamPackManager::CamPackRefresh() {
    if (!this->pActiveCamPack) {
        return false;
    }
    this->pActiveCamPack->Refresh();
    return true;
}
bool CamPackManager::CamPackSave() {
    if (!this->CamPackRefresh()) {
        return false;
    }
    auto dir = this->Path()->PathGetFromKey("kCamPacks") / this->pActiveCamPack->GetName();
    auto [ok, msg] = this->pActiveCamPack->Save(dir);
    if (ok) {
        this->LogSucc(std::move(msg));
        return true;
    }
    else {
        this->LogError(std::move(msg));
        return false;
    }
}

bool CamPackManager::CamPackApply(const std::shared_ptr<CamPack> pCamPack) {
    this->CamPackSave();
    this->pActiveCamPack = pCamPack;
    this->PublishSync("CamSync/Clear"_hash);
    if (!this->Path()->KeySetCurrent("kCurrentPack", pCamPack->GetName())) {
        this->LogError("尝试切换运镜包时出现问题，设置运镜包路径失败！");
        return false;
    }
    this->PublishSync("CamSync/Load"_hash);
    this->pActiveCamPack = pCamPack;
    this->LogSucc(std::format("已切换至运镜包：{}", pCamPack->GetName()));
    return true;
}
bool CamPackManager::CamPackLoad(const std::filesystem::path& dir, const std::string& yamlName) {
    if (dir.empty() || yamlName.empty()) {
        this->LogError("文件夹路径或文件名为空，无法加载运镜包！");
        return false;
    }
    std::filesystem::path pathCamPack = dir / (yamlName + ".yaml");
    this->LogInfo(std::format("尝试加载运镜包：{}", pathCamPack.string()));
    if (!std::filesystem::exists(pathCamPack)) {
        this->LogError(std::format("运镜包不存在：{}", pathCamPack.string()));
        return false;
    }
    try {
        YAML::Node root = YAML::LoadFile(pathCamPack.string());
        std::string loadName = root["name"].as<std::string>();
        if (this->camPacks.find(loadName) != this->camPacks.end()) {
            this->LogError(std::format("运镜包名已占用，无法从文件加载：", loadName));
            return false;
        }
        auto loadCamPack = std::make_shared<CamPack>(loadName);
        loadCamPack->SetKeyCheckPack(root["KCP"].as<MulNX::KeyCheckPack>());
        loadCamPack->OnNewRound = root["OnNewRound"].as<std::vector<std::string>>();

        loadCamPack->Refresh();
        this->LogSucc(std::format("成功从文件加载运镜包：{}", loadName));
        this->camPacks[loadName] = std::move(loadCamPack);
        return true;
    }
    catch (const std::exception& e) {
        this->LogError(std::format("在加载运镜包时出现问题：{}", e.what()));
        return false;
    }
}