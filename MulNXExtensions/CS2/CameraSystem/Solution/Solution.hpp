#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/CS2/CameraSystem/Elements/Elements.hpp>

class ElementWithOffset {
public:
    //元素
    std::shared_ptr<ElementBase> Element;

    //这个Offset决定了元素的播放时间头
    float Offset = 0;
};

//解决方案，包含ElementWithOffset，用于调用call方法
class Solution final {
    //友元声明
    friend class SolutionManager;
private:
    //数据存储

    //解决方案名称
    std::string name{};

    //存储元素的指针，允许调用不同元素的同一个Call函数（多态，内部实现不同）
    std::vector<ElementWithOffset> elements;

    //开始时间
    float startTime{};
    //结束时间
    float endTime{};
    //总持续时间
    float totalDurationTime{};
    //解决方案时间偏移
    float solutionOffset = 0;
    //播放模式
    PlaybackMode playmode = PlaybackMode::Orchestration;

    //按键检测包
    MulNX::KeyCheckPack KCPack{};

    //脏标记，需要重新保存
    bool dirty = false;
public:
    Solution(const std::string& name) :
        name(name) {
        this->Refresh();
    }
    ~Solution() = default;

    //解决方案能否安全使用
    bool safeUse = false;
    

    //添加元素
    //常量指针常量说明没有修改权，时间偏移默认是0.0f
    bool AddElement(const std::shared_ptr<ElementBase> element, const float Offset = 0.0f);
    //移除指定位置的元素
    bool RemoveElementAt(const size_t Index);

    std::pair<bool, std::string> Save(const std::filesystem::path& folderPath);
    std::pair<bool, std::string> Load(YAML::Node& root, ElementManager* elementManager);

    //清空数据
    void Clear();
    //重设名字
    void ResetName(std::string_view NewName);
    //获取名字
    std::string GetName()const;

    //解决方案插值调用
    bool Call(CameraSystemIO* IO);
    //设置解决方案偏移
    void SetSolutionOffset(const float Offset);
    //按组合模式各个元素绝对启动时间生成复用模式的偏移
    bool TimeLineGenerate();
    //刷新（所有涉及调用、时间的修改操作应该调用本函数，会移除已经删除的元素）
    void Refresh();
    //展示信息
    std::string GetMsg();

    //设置按键检测包
    void SetKeyCheckPack(const MulNX::KeyCheckPack& KCPack);
};