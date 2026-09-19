#include "Solution.hpp"
#include <CameraSystem/ElementManager/ElementManager.hpp>
#include <yaml-cpp/yaml.h>
#include <fstream>

bool Solution::AddElement(std::string&& name, const float Offset) {
    //检查重复
    auto it = std::find_if(this->elements.begin(), this->elements.end(),
        [&](const ElementWithOffset& ew) {
            if (ew.campathName.empty())return false;
            if (ew.campathName == name)return true;
            return false;
        });

    if (it != this->elements.end()) {
        return false;
    }

    //创建新元素
    ElementWithOffset newElement{ name, Offset };

    //找到正确的插入位置以保持排序
    auto insertPos = std::lower_bound(this->elements.begin(), this->elements.end(), newElement,
        [](const ElementWithOffset& a, const ElementWithOffset& b) {
            return a.Offset < b.Offset;
        });

    //插入元素
    this->elements.insert(insertPos, std::move(newElement));

    return true;
}
bool Solution::RemoveElementAt(const size_t Index) {
    if (Index < 0 || Index >= this->elements.size()) {
        return false;
    }
    this->elements.erase(this->elements.begin() + Index);
    return true;
}

void Solution::SetSolutionOffset(const float Offset) {
    this->solutionOffset = Offset;
}
std::string Solution::GetMsg() {
    std::ostringstream oss;
    oss << "解决方案名称：" << this->name
        << "   元素数量：" << this->elements.size()
        << "   总时长：" << this->totalDurationTime
        << "\n详细信息："
        << "\n";

    for (size_t i = 0; i < this->elements.size(); ++i) {
        auto& name = this->elements[i].campathName;
        auto& offset = this->elements[i].Offset;
        oss << i << ".  "
            "  |元素编号：" << i <<
            "  元素名称：" << name <<
            "  元素类型：" << "自由摄像机轨道" <<
            "  元素偏移时间：" << this->elements[i].Offset << "\n";
    }

    return oss.str();
}
void Solution::Clear() {
    this->elements.clear();
    this->solutionOffset = 0;
    return;
}
void Solution::ResetName(std::string_view NewName) {
    this->name = NewName;
    this->dirty = true;
    return;
}
std::string Solution::GetName()const {
    return this->name;
}
void Solution::SetKeyCheckPack(const MulNX::KeyCheckPack& KCPack) {
    this->KCPack = KCPack;
    this->KCPack.Refresh();
    this->dirty = true;
    return;
}

std::pair<bool, std::string> Solution::Save(const std::filesystem::path& folderPath) {
    // 检查文件路径和名称存在性
    if (folderPath.empty()) return { false, "文件夹路径为空，无法保存解决方案！" };

    // 拼接完整路径
    std::filesystem::path filePath = folderPath / (this->name + ".yaml");

    try {
        YAML::Node root;

        root["name"] = this->name;
        root["duration"] = this->totalDurationTime;
        root["KCP"] = this->KCPack;
        root["size"] = this->elements.size();

        YAML::Node elementsNode = root["elements"];
        for (size_t i = 0; i < this->elements.size(); ++i) {
            auto& name = this->elements[i].campathName;

            YAML::Node elemNode;
            elemNode["name"] = name;
            elemNode["offset"] = this->elements[i].Offset;
            elementsNode.push_back(elemNode);
        }

        std::ofstream fout(filePath);
        fout << root;
        fout.close();

        return { true, std::format("保存成功  解决方案名：{}  元素个数：{}",this->name ,this->elements.size()) };
    }
    catch (const std::exception& e) {
        return { false, "保存失败：" + std::string(e.what()) };
    }
}
std::pair<bool, std::string> Solution::Load(YAML::Node& root) {
    this->KCPack = root["KCP"].as<MulNX::KeyCheckPack>();
    // 获取解决方案名称并检查是否为空
    this->name = root["name"].as<std::string>();
    // 获取持续时长信息
    float TargetDurationTime = root["duration"].as<float>();
    // 元素总量
    size_t AllCount = root["size"].as<size_t>();
    if (root["elements"].size() != AllCount) {
        return { false,std::format("不安全的解决方案！实际元素数量与文件描述不符！ 解决方案名：{}",this->name) };
    }

    // 读取流程
    for (const auto& nodeElement : root["elements"]) {
        // 获取元素名称
        std::string campathName = nodeElement["name"].as<std::string>();
        // 获取元素偏移
        float ElementOffset = nodeElement["offset"].as<float>();
        // 尝试创建带有时间偏移的弱引用指针并添加进新解决方案并判断是否成功
        if (!this->AddElement(std::string(campathName), ElementOffset)) {
            return { false,std::format("无法添加元素到解决方案   解决方案名：{}  元素名：{}" ,this->name, campathName) };
        }
    }
    // 去除脏标记
    this->dirty = false;

    return { true,"解决方案加载成功" };
}