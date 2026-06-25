#include "Project.hpp"

#include <strstream>
#include <filesystem>
#include <fstream>

void Project::ResetName(const std::string& NewName) {
    this->Name = NewName;
    return;
}

void Project::Refresh() {
    return;
}

std::pair<bool, std::string> Project::Save(const std::filesystem::path& FolderPath) {
    try {
        YAML::Node root;
        //检查文件路径和名称存在性
        if (FolderPath.empty())return { false,"文件夹路径为空，无法保存解决方案！" };
        //拼接完整路径
        std::filesystem::path FullPath = FolderPath / (this->Name + ".yaml");

        //保存Project名称
        root["name"] = this->Name;
        root["KCP"] = this->KCPack;
        root["OnNewRound"] = this->OnNewRound;

        std::ofstream fout(FullPath);
        fout << root;
        fout.close();

        return { true,"保存成功  项目名：" + this->Name };
    }
    catch (const YAML::Exception& e) {
        return { false,std::string(e.what()) };
    }
}

std::string Project::GetMsg()const {
    std::ostringstream oss;
    oss << "项目名称：" << this->Name << "   项目描述：" << this->GetDescription();

    return oss.str();
}