#pragma once
// MessageManager.hpp
// 跨线程安全的消息管理器
// 管理整个系统消息的发送和接受
// 消息由发布者创建，经过发布后，生命周期的管理即委托给消息管理器

#include <MulNX/Core/Module/Module.hpp>
#include <MulNX/Base/Cmd/Cmd.hpp>
#include "MessageChannel/MessageChannel.hpp"
#include <unordered_map>
#include <MulNXThirdParty/queue/blockingconcurrentqueue.h>

namespace MulNX {
    class MsgMeta {
    public:
        std::string RawString;
        std::function<ParsedArgs(std::string)> handle;
    };
    class MessageManager final :public MulNX::Module<MessageManager> {
        friend MessageChannel;
    private:
        // 元数据
        std::unordered_map<MulNX::MsgType, MsgMeta>msgInfo{};
        // 异步
        std::shared_mutex asyncMutex;
        std::unordered_map<MulNX::MsgType, std::vector<MessageChannel*>>asyncMap{};
        std::unordered_map<MulNXHandle, std::unique_ptr<MessageChannel>>asyncChannels;
        moodycamel::BlockingConcurrentQueue<MulNX::Message> asyncMsgBuffer;
        // 同步
        std::shared_mutex syncMutex;
        std::unordered_map<MulNX::MsgType, std::vector<SyncMsgCallback>>syncMap{};

        bool AddMsgMeta(const std::string& type, size_t hashed);
    public:
        bool Init()override;

        // 创建私有消息队列（但是生命周期仍然委托给消息管理器）
        MulNXHandle CreateMessageChannel();
        MessageChannel* GetMessageChannel(const MulNXHandle& hChannel);
        bool SubscribeAsync(MessageChannel* const pChannel, const std::string& type);
        bool PublishAsync(Message&& msg);
        bool DispathAsyncMsg();// 单次派发，返回true意味着还有消息
        bool HandleDispatch();// 派发所有剩余消息

        bool SubscribeSync(const std::string& type, SyncMsgCallback&& handle);
        bool PublishSync(MulNX::Message& msg);
    };
}