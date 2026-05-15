#include "ShortcutManager.hpp"
#include <MulNX/Systems/InputSystem/InputSystem.hpp>
#include <yaml-cpp/yaml.h>

bool MulNX::ShortcutManager::Init() {
    auto path = this->ISys().PathGet("Config");
    auto config = YAML::LoadFile((path / "shortcut.yaml").string());

    // 读取顶层 name
    std::string configName = config["name"].as<std::string>();
    this->ISys().LogInfo("配置文件名称: " + configName);

    // 遍历 buttons 列表
    for (const auto& cButton : config["buttons"]) {
        auto name = cButton["name"].as<std::string>();
        auto KCP = cButton["KCP"].as<MulNX::KeyCheckPack>();
        this->buttons[name] = KCP;
    }

    // 遍历 binds 列表
    for (const auto& cBind : config["binds"]) {
        Bind bind;
        bind.desc = cBind["desc"].as<std::string>();
        bind.msg = cBind["msg"].as<std::string>();
        bind.KCP = cBind["KCP"].as<MulNX::KeyCheckPack>();
        this->binds.push_back(std::move(bind));
    }

    this->SendTask("MulNXMain", [this]() {
        this->Check();
        return true;
        });

    return true;
}

void MulNX::ShortcutManager::Check() {
    for (const auto& bind : this->binds) {
        auto result = this->pInputSystem->CheckWithPack(bind.KCP);
        if (!result)continue;
        this->ISys().PublishAsync(MulNX::HashString(bind.msg));
    }
}

std::optional<MulNX::KeyCheckPack> MulNX::ShortcutManager::GetButton(const std::string& name) {
    auto it = this->buttons.find(name);
    if (it == this->buttons.end())return std::nullopt;
    return it->second;
}