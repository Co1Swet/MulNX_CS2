#include "MessageManager.hpp"
#include "MessageChannel/MessageChannel.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/Systems.hpp>

bool MulNX::MessageManager::Init() {

    return true;
}

bool MulNX::MessageManager::AddMsgMeta(const std::string& type, size_t hashed, const bool isAsync,
    std::function<void(MulNX::Message&, std::string_view)>&& makingHandler) {
    std::unique_lock lock(this->smutex);
    auto& Meta = this->msgInfo[hashed];
    
    if (Meta.RawString.empty()) {
        Meta.RawString = type;
        Meta.isAsync = isAsync;
        Meta.makingHandler = std::move(makingHandler);
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
    std::unique_ptr<MessageChannel> Channel = std::make_unique<MessageChannel>(this);
    MulNXHandle hChannel = MulNXHandle::CreateHandle();
    Channel->hChannel = hChannel;
    this->asyncChannels[hChannel] = std::move(Channel);
    return hChannel;
}
MulNX::MessageChannel* MulNX::MessageManager::GetMessageChannel(const MulNXHandle& hChannel)const {
    auto it = this->asyncChannels.find(hChannel);
    if (it == this->asyncChannels.end())return nullptr;
    return it->second.get();
}

bool MulNX::MessageManager::PublishAsync(Message&& msg)const {
    return this->asyncMsgBuffer.enqueue(std::move(msg));
}
bool MulNX::MessageManager::SubscribeAsync(MessageChannel* const pChannel, const std::string& type,
    std::function<void(MulNX::Message&, std::string_view)>&& makingHandler) {
    MulNX::MsgType hashed = MulNX::HashString(type);
    this->AddMsgMeta(type, hashed, true, std::move(makingHandler));
    this->asyncMap[hashed].push_back(pChannel);
    return true;
}

bool MulNX::MessageManager::DispatchAsyncMsg()const {
    MulNX::Message msg;
    if(!this->asyncMsgBuffer.wait_dequeue_timed(msg, 100000))return false;
    // 检查是否存在管道订阅者
    auto it = this->asyncMap.find(msg.type);
    if (it == this->asyncMap.end())return true;
    auto& vecSubscriber = it->second;
    size_t size = vecSubscriber.size();
    if (size == 0)return true;
    --size;
    // 按需复制
    for (size_t Index = 0; Index < size; ++Index) {
        // 其他订阅者使用克隆的消息
        vecSubscriber[Index]->PushMessage(Message(msg));
    }
    // 最后一个订阅者获得原始消息
    vecSubscriber[size]->PushMessage(std::move(msg));
    return true;
}

bool MulNX::MessageManager::HandleDispatch()const {
    if (!this->pGlobalVars->SystemReady.load()) {
        return true;
    }
    this->LogSucc("消息派发激活！");
    while (this->dispatchEnable.load(std::memory_order_acquire)) {
        this->DispatchAsyncMsg();
        continue;
    }
    return false;
}

bool MulNX::MessageManager::SubscribeSync(const std::string& type, SyncMsgCallback&& handle) {
    MulNX::MsgType hashed = MulNX::HashString(type);
    this->AddMsgMeta(type, hashed);
    this->syncMap[hashed].push_back(std::move(handle));
    return true;
}

bool MulNX::MessageManager::PublishSync(MulNX::Message& msg)const {
    auto it = this->syncMap.find(msg.type);
    if (it == this->syncMap.end())return false;
    auto& subscribers = it->second;
    try {
        for (auto& subscriber : subscribers) {
            subscriber(msg);
        }
    }
    catch (const std::exception& e) {
        MulNX::ErrorTerminate(e.what());
    }
    return true;
}