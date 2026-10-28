#include "VarReflSystem.hpp"
#include <Core/Core.hpp>
#include <Systems/PathManager/PathManager.hpp>
#include <Systems/MessageManager/MessageManager.hpp>
#include <Systems/Logger/Logger.hpp>
#include <MulNXThirdParty/yaml.hpp>

using namespace MulNX;

static YAML::Node config{};

bool VarReflSystem::Init() {
    this->pthCfg = this->Path()->PathGetForShared("Config") /
        std::format("{}_Refl.yaml", this->Core->GetName());

    try {
        config = YAML::LoadFilePath(this->pthCfg);
    }
    catch (const std::exception& e) {
        this->LogError(std::format("文件加载失败，无法执行反序列化：{}", e.what()));
    }

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
    this->LogInfo("正在执行变量序列化");
    YAML::Node root;
    std::ofstream fout(this->pthCfg.wstring());
    uint64_t succ = 0;
    for (auto& [name, iVar] : this->vars) {
        try {
            auto valueString = iVar->ValueToString();
            root[name] = valueString;
            ++succ;
        }
        catch (const std::exception& e) {
            this->LogError(e.what());
        }
    }
    fout << root;
    if (succ == this->vars.size()) {
        this->LogSucc(std::format("成功序列化反射变量 {} 个到 {}", succ, this->pthCfg.string()));
    }
    else {
        this->LogWarning(std::format("成功序列化反射变量 {} 个到 {}，失败 {} 个",
            succ, this->pthCfg.string(), this->vars.size()));
    }
    return true;
}