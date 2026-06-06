#include "../PathManager.hpp"

MulNX::FilePathNode* MulNX::PathManager::NodeGetFromKey(const std::string& Key) {
    auto it = this->Nodes.find(Key);
    if (it == this->Nodes.end()) {
        return nullptr;
    }
    return &it->second;
}
bool MulNX::PathManager::NodeHasRoot(FilePathNode* Node) {  
    if (!Node->Static.empty()) {
        return true;
    }
    else if (Node->KeyParent.empty()) {
        return false;
    }
    auto* nodeParent = this->NodeGetFromKey(Node->KeyParent);
    if (nodeParent == nullptr)return false;
    if (nodeParent->CurrentValue.empty())return false;
    return this->NodeHasRoot(nodeParent);
}

bool MulNX::PathManager::KeyBindParentKey(const std::string& Key, const std::string& Parent) {
    auto* Node = this->NodeGetFromKey(Key);
    if (this->NodeGetFromKey(Node->KeyParent)) {
        this->KeyUnbindParentKey(Key);
    }
    // 查找父节点
    auto* ParentNode = this->NodeGetFromKey(Parent);
    // 向父节点添加自己的信息
    ParentNode->KeySon.push_back(Key);
    // 写入新数据
    Node->KeyParent = Parent;
    Node->Static.clear();
    return true;
}
bool MulNX::PathManager::KeyUnbindParentKey(const std::string& Key) {
    auto* Node = this->NodeGetFromKey(Key);
    if (!Node->KeyParent.empty()) {
        // 查找父节点
        auto* ParentNode = this->NodeGetFromKey(Node->KeyParent);
        // 从父节点清除自己的信息
        std::erase(ParentNode->KeySon, Key);
        // 清除自身数据
        Node->KeyParent.clear();
    }
    return true;
}

bool MulNX::PathManager::CallNodeChange(FilePathNode* Node) {
    // 这里不需要加锁，内部函数只有加锁的内部函数进行访问
    if ((!this->NodeHasRoot(Node)) || Node->CurrentValue.empty()) {
        return true;
    }
    bool AllRight = true;
    if (Node->OnCurrentValueChange == nullptr) {
        AllRight = false;
    }
    else {
        AllRight = AllRight && Node->OnCurrentValueChange(this);
    }
    for (const auto& key : Node->KeySon) {
        auto it = this->Nodes.find(key);
        if (it != Nodes.end()) {
            AllRight = AllRight && this->CallNodeChange(&it->second);
        }
    }
    return AllRight;
}

bool MulNX::PathManager::CreateKey(const std::string& Key, std::string&& Value, std::function<bool(PathManager*)>&& OnChange) {
    std::unique_lock lock(this->MutexEx);
    auto it = this->Nodes.find(Key);
    if (it != this->Nodes.end()) {
        this->ISys().LogWarning("key尝试被二次创建:" + Key);
        return false;
    }
    this->Nodes[Key] = MulNX::FilePathNode{};
    this->Nodes[Key].OnCurrentValueChange = std::move(OnChange);
    this->Nodes[Key].CurrentValue = std::move(Value);
    return true;
}
bool MulNX::PathManager::KeyBindStatic(const std::string& Key, const std::filesystem::path& Position) {
    std::unique_lock lock(this->MutexEx);
    auto* Node = this->NodeGetFromKey(Key);
    this->KeyUnbindParentKey(Key);
    Node->Static = Position;
    if (Node->CurrentValue.empty()) {
        return true;
    }
    else {
        return this->CallNodeChange(Node);
    }
}
bool MulNX::PathManager::KeyBindDynamic(const std::string& Key, const std::string& Parent) {
    std::unique_lock lock(this->MutexEx);
    this->KeyBindParentKey(Key, Parent);
    // 通过Key查找当前节点
    auto* Node = this->NodeGetFromKey(Key);
    if (Node->CurrentValue.empty()) {
        return true;
    }
    else {
        return this->CallNodeChange(Node);
    }
}
bool MulNX::PathManager::KeySetCurrent(const std::string& Key, const std::string& Current) {
    std::unique_lock lock(this->MutexEx);
    auto* Node = this->NodeGetFromKey(Key);
    Node->CurrentValue = Current;
    bool NotCall = Node->KeyParent.empty() && Node->Static.empty();
    if (!NotCall) {
        return this->CallNodeChange(Node);
    }
    return true;
}
// ToDo 后续再处理栈溢出，反正都是我自己写的，我相信我自己
std::filesystem::path MulNX::PathManager::PathGetFromKey(const std::string& Key) {
    std::unique_lock lock(this->MutexEx);
    auto* Node = this->NodeGetFromKey(Key);
    if (!Node->Static.empty()) {
        return Node->Static / Node->CurrentValue;
    }
    else {
        if (Node->KeyParent.empty()) {
            this->ISys().LogError("一个节点在向上追溯时，发现既不存在静态绑定，父节点字符串也为空");
            return {};
        }
        if (Node->CurrentValue.empty()) {
            this->ISys().LogError(Key + " 对应的节点不存在当前值，无法拼接路径");
            return {};
        }
        return this->PathGetFromKey(Node->KeyParent) / Node->CurrentValue;// 一路构建完整路径
    }
}