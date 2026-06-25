#include "Solution.hpp"
#include <CameraSystem/ElementManager/ElementManager.hpp>
#include <yaml-cpp/yaml.h>
#include <fstream>

bool Solution::AddElement(const std::shared_ptr<ElementBase> element, const float Offset) {
    //检查重复
    auto it = std::find_if(this->elements.begin(), this->elements.end(),
        [&element](const ElementWithOffset& ew) {
            if (auto el = ew.Element) {
                return el == element;
            }
            return false;
        });

    if (it != this->elements.end()) {
        return false;
    }

    //创建新元素
    ElementWithOffset newElement{ element, Offset };

    //找到正确的插入位置以保持排序
    auto insertPos = std::lower_bound(this->elements.begin(), this->elements.end(), newElement,
        [](const ElementWithOffset& a, const ElementWithOffset& b) {
            return a.Offset < b.Offset;
        });

    //插入元素
    this->elements.insert(insertPos, std::move(newElement));

    //计算自身大小同时检验所有元素有效性
    this->Refresh();
    return true;
}
bool Solution::RemoveElementAt(const size_t Index) {
    if (Index < 0 || Index >= this->elements.size()) {
        return false;
    }
    this->elements.erase(this->elements.begin() + Index);
    this->Refresh();
    return true;
}

void Solution::Refresh() {
    //标记为脏
    this->dirty = true;
    //判空处理
    if (this->elements.empty()) {
        this->safeUse = false;
        this->startTime = 0;
        this->endTime = 0;
        this->totalDurationTime = 0;
        return;
    }

    //清理过期元素的同时查找整个解决方案的结束时间点
    for (auto It = this->elements.begin(); It != this->elements.end();) {
        if (It->Element->NeedBeDelete) {
            It = this->elements.erase(It);//erase返回下一个有效迭代器
        }
        else {
            float ElementEndTime = It->Offset + It->Element->DurationTime;
            if (this->endTime < ElementEndTime) {
                this->endTime = ElementEndTime;
            }

            ++It;//未删除时递增
        }
    }

    //更新后判空
    if (this->elements.empty()) {
        this->safeUse = false;
        this->startTime = 0;
        this->endTime = 0;
        this->totalDurationTime = 0;
        return;
    }

    //获取第一个元素的开始时间
    this->startTime = this->elements.front().Offset;

    //计算总持续时间（从第一个元素开始到最后一个元素结束）
    this->totalDurationTime = this->endTime - this->startTime;
    this->safeUse = true;

    return;
}
void Solution::SetSolutionOffset(const float Offset) {
    this->solutionOffset = Offset;
}
bool Solution::TimeLineGenerate() {
    this->Refresh();

    if (this->elements.empty()) {
        return false;
    }
    float TimeReference = this->elements[0].Offset;
    for (int i = 0; i < this->elements.size(); ++i) {
        //先获取最早的元素的开始时间作为参考时间
        if (TimeReference > this->elements[i].Element->GetStartTime()) {
            TimeReference = this->elements[i].Element->GetStartTime();
        }
    }
    //然后设置元素偏移
    for (int i = 0; i < this->elements.size(); ++i) {
        this->elements[i].Offset = this->elements[i].Element->GetStartTime() - TimeReference;
    }

    this->Refresh();

    return true;
}
std::string Solution::GetMsg() {
    this->Refresh();

    if (!this->safeUse)return "不安全的解决方案";

    std::ostringstream oss;
    oss << "解决方案名称：" << this->name
        << "   元素数量：" << this->elements.size()
        << "   总时长：" << this->totalDurationTime
        << "\n详细信息："
        << "\n";

    for (size_t i = 0; i < this->elements.size(); ++i) {
        std::shared_ptr<ElementBase> element = this->elements.at(i).Element;
        if (element) {
            oss << i << ".  "
                "  |元素编号：" << i <<
                "  元素名称：" << element->Name <<
                "  元素类型：" << element->TypeGet_String() <<
                "  元素持续时间：" << element->DurationTime <<
                "  元素偏移时间：" << this->elements[i].Offset << "\n";
        }
    }

    return oss.str();
}
void Solution::Clear() {
    this->elements.clear();
    this->Refresh();
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
bool Solution::Call(CameraSystemIO* IO) {
    if (!this->safeUse) {
        IO->isPlaying = false;
        return false;
    }
    //对时间依次进行两次偏移：解决方案偏移，具体元素具体偏移

    //判断是否有插值结果。
    //有结果，Manager才可能覆盖
    bool bResult = false;

    //尝试依次调用所有的元素插值
    //编号靠后的元素有更高的决定权
    this->Refresh();//调用前立刻更新，智能共享指针锁定状态

    //先分开播放模式逻辑
    switch (this->playmode) {
    case PlaybackMode::Orchestration: {
        //偏移时间轴播放

        //判断偏移后的时间是否位于解决方案持续范围之中（先统一计算偏移后时间，无论究竟有没有偏移）
        float SolutionOffsetedTime = IO->SolutionTime - this->solutionOffset;

        if (this->endTime < SolutionOffsetedTime) {
            //播放结束
            this->solutionOffset = 0;//归位时间
            IO->isPlaying = false;//播放结束
            return false;//无插值结果
        }
        for (size_t i = 0; i < this->elements.size(); ++i) {
            //这里用减法得到相对于元素的时间
            //尝试该元素插值，如果有结果则代表可以应用
            //这里传入的时间已经是相对时间
            //模式1自动减去头时间
            auto& element = this->elements[i].Element;
            IO->ElementTime = SolutionOffsetedTime - this->elements[i].Offset + element->GetStartTime();
            bResult = bResult || element->CalculateFrame(IO);
        }
        break;
    }
    case PlaybackMode::Activation: {
        //默认游戏时间轴播放
        //if (Time < this->StartTime || this->EndTime < Time) {
        //    return false;//无插值结果
        //    //不修改isPlaying状态，使用者任意跳转时间，如果在范围内，仍能给出插值结果
        //}
        
        IO->ElementTime = IO->SolutionTime;
        for (size_t i = 0; i < this->elements.size(); ++i) {
            auto& element = this->elements[i].Element;
            bResult = bResult || element->CalculateFrame(IO);
        }
        break;
    }
    }

    return bResult;
}
void Solution::SetKeyCheckPack(const MulNX::KeyCheckPack& KCPack) {
    this->KCPack = KCPack;
    this->KCPack.Refresh();
    this->dirty = true;
    return;
}

std::pair<bool, std::string> Solution::Save(const std::filesystem::path& folderPath) {
    // 先刷新确保安全有效
    this->Refresh();
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
            std::shared_ptr<ElementBase> element = this->elements[i].Element;
            if (!element) return { false, "疑似有元素在保存过程中被删除，保存终止！" };

            YAML::Node elemNode;
            elemNode["name"] = element->Name;
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
std::pair<bool, std::string> Solution::Load(YAML::Node& root, ElementManager* elementManager) {
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
        std::string NewElementName = nodeElement["name"].as<std::string>();
        // 得到元素指针
        auto it = elementManager->elements.find(NewElementName);
        // 检验是否找到元素
        if (it==elementManager->elements.end()) {
            return { false,"找不到目标元素   元素名：" + NewElementName };
        }
        // 获取元素偏移
        float ElementOffset = nodeElement["offset"].as<float>();
        // 尝试创建带有时间偏移的弱引用指针并添加进新解决方案并判断是否成功
        if (!this->AddElement(it->second, ElementOffset)) {
            return { false,std::format("无法添加元素到解决方案   解决方案名：{}  元素名：{}" ,this->name, NewElementName) };
        }
    }

    // 刷新
    this->Refresh();
    // 去除脏标记
    this->dirty = false;

    return { true,"解决方案加载成功" };
}