#include "CamPack.hpp"
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>

void CamPack::ResetName(const std::string& newName) {
    this->name = newName;
}

void CamPack::Refresh() {
    // 预留
}

std::pair<bool, std::string> CamPack::Save(const std::filesystem::path& dir) {
    try {
        if (dir.empty()) return { false, "文件夹路径为空，无法保存运镜包！" };

        auto pathSave = dir / (this->name + ".yaml");
        YAML::Node root;
        root["name"] = this->name;
        root["KCP"] = this->KCPack.load(std::memory_order_acquire);
        root["OnNewRound"] = this->OnNewRound;
        root["OnRoundStart"] = this->OnRoundStart;
        root["OnRoundEnd"] = this->OnRoundEnd;

        std::ofstream fout(pathSave);
        fout << root;
        fout.close();

        return { true, std::format("运镜包保存成功：{}", this->name) };
    }
    catch (const std::exception& e) {
        return { false, std::string(e.what()) };
    }
}