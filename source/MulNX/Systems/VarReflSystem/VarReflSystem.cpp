#include "VarReflSystem.hpp"
#include <Systems/PathManager/PathManager.hpp>
#include <Systems/MessageManager/MessageManager.hpp>
#include <Systems/Logger/Logger.hpp>
#include <MulNXThirdParty/yaml.hpp>

using namespace MulNX;

static YAML::Node config{};

bool VarReflSystem::Init() {
    this->pthCfg = this->PathGet("Vars") / "config.yaml";
    config = YAML::LoadFilePath(this->pthCfg);
    auto enable = config["enable"].as<bool>();
    if (!enable)return true;

    this->SubscribeSync("System/Init/End", [this](auto&&...) {
        this->LogInfo(std::format("共创建 {} 个可反射变量，并从配置文件中加载初始值成功 {} 个",
            this->vars.size(), this->successCount));
        });

    return true;
}

std::string VarReflSystem::MakeKey(const ModuleBase* pMod, std::string_view name) {
    return std::format("{}.{}", pMod->GetName(), name);
}

bool VarReflSystem::Register(std::unique_ptr<IReflVar> var) {
    auto name = this->MakeKey(var->GetModule(), var->GetName());
    YAML::Node node = config[name];
    if (node && node.IsDefined() && node.IsScalar()) {
        if (var->StringToValue(node.Scalar())) {
            ++this->successCount;
        }
    }
    this->vars[name] = std::move(var);
    return true;
}

bool VarReflSystem::Deinit() {

    return true;
}