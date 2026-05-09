#include "I18nManager.hpp"

#include <yaml-cpp/yaml.h>
#include <stack>

MulNX::I18nManager::I18nManager() {
    this->pThis = this;
}

bool MulNX::I18nManager::Init() {
    auto path = this->ISys().PathGet("Language");
    auto filePath = path / "lan.yaml";

    this->strings.clear();
    YAML::Node root = YAML::LoadFile(filePath.string());
    this->LoadYaml(root, {});
    
    this->ISys().LogSucc(I18n("sys.i18n.load_succ", filePath.string()));
    this->ISys().LogSucc(I18n("sys.i18n.load_nums", this->strings.size()));
    return true;
}

const std::string& I18n(const std::string& key) {
    return MulNX::I18nManager::pThis->Get(key);
}

void MulNX::I18nManager::LoadYaml(const YAML::Node& node, const std::string& key) {
    if (node.IsScalar()) {
        this->strings[key] = node.as<std::string>();
        return;
    }
    for (auto it = node.begin();it != node.end();++it) {
        auto full = key.empty()
            ? it->first.as<std::string>()
            : key + "." + it->first.as<std::string>();
        this->LoadYaml(it->second, full);
    }
}

const std::string& MulNX::I18nManager::Get(const std::string& key) {
    auto it = this->strings.find(key);
    if (it == this->strings.end())return key;
    return it->second;
}