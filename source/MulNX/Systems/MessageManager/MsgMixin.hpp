#pragma once
#include <MulNX/Core/Module/IModule.hpp>

namespace MulNX {
    class MessageManager;

    template<typename T>
    class MsgMixin {
    private:
        MessageManager* pMsgManager = nullptr;
    public:
        T* This() { return static_cast<T*>(this); }
        MsgMixin() {
            This()->delayInits->push_back([this]() {
                this->pMsgManager = static_cast<MessageManager*>(This()->FindModule("MessageManager"));
                This()->MainMsgChannel = this->pMsgManager->GetMessageChannel(this->pMsgManager->CreateMessageChannel());
                return true;
                });
        }

        // 异步消息

        auto& SubscribeAsync(const std::string& msgType) {
            This()->MainMsgChannel->SubscribeAsync(msgType);
            This()->LogSucc(I18n("sys.msg.async.subed{}", msgType));
            return *this;
        }
        template<typename... Args>
        auto& SubscribeAsync(const std::string& msgType) {
            auto h = createHandler<Args...>(MulNX::HashString(msgType));
            This()->MainMsgChannel->SubscribeAsync(msgType);
            This()->LogSucc(I18n("sys.msg.async.subed{}", msgType));
            return *this;
        }

        void PublishAsync(MulNX::Message&& msg) {
            this->pMsgManager->PublishAsync(std::move(msg));
        }
        void PublishAsync(MulNX::MsgType msg) {
            this->pMsgManager->PublishAsync(MulNX::Message(msg));
        }


        // 同步消息
        auto& SubscribeSync(const std::string& msgType, MulNX::SyncMsgCallback&& handle) {
            this->pMsgManager->SubscribeSync(msgType, std::move(handle));
            This()->LogSucc(I18n("sys.msg.sync.subed{}", msgType));
            return *this;
        }
        void PublishSync(MulNX::Message& msg) {
            this->pMsgManager->PublishSync(msg);
        }
        void PublishSync(MulNX::MsgType msg) {
            auto m = MulNX::Message(msg);
            this->pMsgManager->PublishSync(m);
        }

    };
}