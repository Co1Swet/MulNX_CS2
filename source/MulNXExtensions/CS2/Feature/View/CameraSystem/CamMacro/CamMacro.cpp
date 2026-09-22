#include "CamMacro.hpp"
#include <yaml-cpp/yaml.h>
#include <fstream>

bool CamMacro::AddCampath(const std::string& name, const float offset) {
    auto it = std::find_if(this->wrapCampaths.begin(), this->wrapCampaths.end(),
        [&](const auto& ew) {
            if (ew.campathName.empty())return false;
            if (ew.campathName == name)return true;
            return false;
        });

    if (it != this->wrapCampaths.end()) {
        return false;
    }

    WrapCampath newWrap{ name, offset };
    auto insertPos = std::lower_bound(this->wrapCampaths.begin(), this->wrapCampaths.end(),
        newWrap, [](const auto& a, const auto& b) {
            return a.offset < b.offset;
        });
    this->wrapCampaths.insert(insertPos, std::move(newWrap));
    return true;
}
bool CamMacro::RemoveCampath(const std::string& campathName) {
    for (auto it = this->wrapCampaths.begin();it != this->wrapCampaths.end();++it) {
        if (it->campathName == campathName) {
            this->wrapCampaths.erase(it);
            return true;
        }
    }
    return false;
}
std::string CamMacro::GetMsg() {
    std::ostringstream oss;
    oss << "运镜宏名称：" << this->name
        << "   运镜数量：" << this->wrapCampaths.size()
        << "\n详细信息："
        << "\n";

    for (size_t i = 0; i < this->wrapCampaths.size(); ++i) {
        oss << i << ".  "
            "  --编号：" << i <<
            "  名称：" << this->wrapCampaths[i].campathName <<
            "  偏移时间：" << this->wrapCampaths[i].offset << "\n";
    }

    return oss.str();
}
void CamMacro::Clear() {
    this->wrapCampaths.clear();
    this->dirty = true;
    return;
}
void CamMacro::ResetName(std::string_view NewName) {
    this->name = NewName;
    this->dirty = true;
    return;
}
const std::string& CamMacro::GetName()const {
    return this->name;
}
void CamMacro::SetKeyCheckPack(const MulNX::KeyCheckPack& KCPack) {
    this->KCPack = KCPack;
    this->KCPack.load().Refresh();
    this->dirty = true;
    return;
}

std::pair<bool, std::string> CamMacro::Save(const std::filesystem::path& folderPath)const {
    if (!this->dirty)return { true,std::format("宏未修改，无需保存：{}",this->name) };
    if (folderPath.empty()) return { false, "文件夹路径为空，无法保存运镜宏！" };
    std::filesystem::path filePath = folderPath / (this->name + ".yaml");
    try {
        YAML::Node root;

        root["name"] = this->name;
        root["KCP"] = this->KCPack.load();
        root["size"] = this->wrapCampaths.size();

        YAML::Node elementsNode = root["elements"];
        for (size_t i = 0; i < this->wrapCampaths.size(); ++i) {
            auto& name = this->wrapCampaths[i].campathName;

            YAML::Node elemNode;
            elemNode["name"] = name;
            elemNode["offset"] = this->wrapCampaths[i].offset;
            elementsNode.push_back(elemNode);
        }

        std::ofstream fout(filePath);
        fout << root;
        fout.close();

        return { true, std::format("保存成功：运镜宏名：{}  运镜数：{}",this->name ,this->wrapCampaths.size()) };
    }
    catch (const std::exception& e) {
        return { false, std::format("保存失败：" ,e.what()) };
    }
}
std::pair<bool, std::string> CamMacro::Load(YAML::Node& root) {
    this->KCPack = root["KCP"].as<MulNX::KeyCheckPack>();
    this->name = root["name"].as<std::string>();
    size_t allCount = root["size"].as<size_t>();
    if (root["elements"].size() != allCount) {
        return { false,std::format("不安全的运镜宏！实际运镜数量与文件描述不符！ 运镜宏名：{}",this->name) };
    }
    for (const auto& nodeElement : root["elements"]) {
        std::string campathName = nodeElement["name"].as<std::string>();
        float offset = nodeElement["offset"].as<float>();
        if (!this->AddCampath(campathName, offset)) {
            return { false,std::format("无法添加运镜到运镜宏，运镜宏名：{}，运镜名：{}" ,this->name, campathName) };
        }
    }
    this->dirty = false;
    return { true,"运镜宏加载成功" };
}