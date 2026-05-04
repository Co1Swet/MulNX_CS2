#include"Workspace.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNXThirdParty/All_pugixml.hpp>
#include <fstream>

std::pair<bool, std::string> Workspace::Save(const std::filesystem::path& FolderPath) {
    // 检查文件路径和名称存在性
    if (FolderPath.empty())return { false,"文件夹路径为空，无法保存工作区配置文件！" };
    
    // 拼接完整路径
    std::filesystem::path FullPath = FolderPath / ("WorkspaceConfig.yaml");
    try {
        YAML::Node root;

        auto config = root["config"];
        auto elements = config["elements"];
        elements["PreviewDraw"] = this->Config.ElementCfg.PreviewDraw;
        elements["PreviewOverride"] = this->Config.ElementCfg.PreviewOverride;

        auto solutions = config["solutions"];
        solutions["shortcutEnable"] = this->Config.SolutionCfg.SolutionShortcutEnable;
        solutions["PlayingDraw"] = this->Config.SolutionCfg.PlayingDraw;
        solutions["PlayingOverride"] = this->Config.SolutionCfg.PlayingOverride;

        auto projects = config["projects"];
        projects["ProjectShortcutEnable"] = this->Config.ProjectCfg.ProjectShortcutEnable;

        std::ofstream fout(FullPath);
        fout << root;
        fout.close();

        return { true,"成功保存工作区配置文件到文件！ 文件路径：" + FullPath.string() };
    }
    catch (const YAML::Exception& e) {
        return { false,"在尝试保存工作区时发生异常：" + std::string(e.what()) };
    }
}