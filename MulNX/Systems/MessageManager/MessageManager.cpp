#include "MessageManager.hpp"
#include "MessageChannel/MessageChannel.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/GlobalVars/GlobalVars.hpp>

bool MulNX::MessageManager::Init() {

    return true;
}

bool MulNX::MessageManager::AddMsgMeta(const std::string& type, size_t hashed) {
    auto& Meta = this->msgInfo[hashed];
    if (Meta.RawString.empty()) {
        Meta.RawString = type;
    }
    else if (Meta.RawString == type) {

    }
    else {
        MulNX::ErrorTerminate(
            std::string("哈希碰撞!"
                "\n想要声明的: " + type +
                "\n已有的: " + Meta.RawString));
    }
    return true;
}

// 创建私有消息队列（但是生命周期仍然委托给消息管理器）
MulNXHandle MulNX::MessageManager::CreateMessageChannel() {
    std::unique_lock lock(this->smutex);
    std::unique_ptr<MessageChannel> Channel = std::make_unique<MessageChannel>(this);
    MulNXHandle hChannel = MulNXHandle::CreateHandle();
    Channel->hChannel = hChannel;
    this->asyncChannels[hChannel] = std::move(Channel);
    return hChannel;
}
MulNX::MessageChannel* MulNX::MessageManager::GetMessageChannel(const MulNXHandle& hChannel) {
    std::unique_lock lock(this->smutex);
    auto it = this->asyncChannels.find(hChannel);
    if (it == this->asyncChannels.end())return nullptr;
    return it->second.get();
}

bool MulNX::MessageManager::PublishAsync(Message&& Msg) {
    return this->asyncMsgBuffer.enqueue(std::move(Msg));
}
bool MulNX::MessageManager::SubscribeAsync(MessageChannel* const pChannel, const std::string& type) {
    std::unique_lock lock(this->smutex);
    MulNX::MsgType hashed = MulNX::HashString(type);
    this->AddMsgMeta(type, hashed);
    this->asyncMap[hashed].push_back(pChannel);
    return true;
}

bool MulNX::MessageManager::DispathAsyncMsg() {
    std::shared_lock lock(this->smutex);
    MulNX::Message Msg;
    if (this->asyncMsgBuffer.try_dequeue(Msg)) {
        // 检查是否存在管道订阅者
        auto& SubscriberVector = this->asyncMap[Msg.type];// 获取订阅者容器，这里不可能是空指针
        size_t size = SubscriberVector.size();
        if (size == 0)return false;
        --size;
        // 按需复制
        for (size_t Index = 0; Index < size; ++Index) {
            // 其他订阅者使用克隆的消息
            SubscriberVector[Index]->PushMessage(Message(Msg));
        }
        // 最后一个订阅者获得原始消息
        SubscriberVector[size]->PushMessage(std::move(Msg));
        return true;
    }
    return false;
}

void MulNX::MessageManager::HandleDispatch() {
    while (this->DispathAsyncMsg()) {
        continue;
    }
}

bool MulNX::MessageManager::SubscribeSync(const std::string& type, SyncMsgCallback&& handle) {
    std::unique_lock lock(this->smutex);
    MulNX::MsgType hashed = MulNX::HashString(type);
    this->AddMsgMeta(type, hashed);
    this->syncMap[hashed].push_back(std::move(handle));
    return true;
}

bool MulNX::MessageManager::PublishSync(MulNX::Message& msg) {
    std::shared_lock lock(this->smutex);
    auto it = this->syncMap.find(msg.type);
    if (it == this->syncMap.end())return false;
    auto& subscribers = it->second;
    for (auto& subscriber : subscribers) {
        subscriber(msg);
    }
    return true;
}