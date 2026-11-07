#include "CamPack.hpp"
#include <MulNX/Base/yaml.hpp>
#include <fstream>

void CamPack::ResetName(const std::string& newName) {
    this->name = newName;
}

void CamPack::Refresh() {
    // 预留
}

std::expected<std::string, std::string> CamPack::Save(const fs::path& dir) {
    try {
        if (dir.empty())return std::unexpected("文件夹路径为空，无法保存运镜包！");

        auto pathSave = dir / (this->name + ".yaml");
        YAML::Node root;
        root["name"] = this->name;
        root["KCP"] = this->KCPack.load(std::memory_order_acquire);
        root["OnNewRound"] = this->OnNewRound;
        root["OnRoundStart"] = this->OnRoundStart;
        root["OnRoundEnd"] = this->OnRoundEnd;

        YAML::SaveFilePath(pathSave, root);

        return std::format("运镜包保存成功：{}", this->name);
    }
    catch (const std::exception& e) {
        return std::unexpected(std::format("运镜包保存时发生异常：{}", e.what()));
    }
}