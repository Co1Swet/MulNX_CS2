#pragma once
#include <MulNX/Common/Cmd.hpp>
#include <source_location>

namespace MulNX {
    class MessageManager;

    namespace Detail {
        constexpr std::string_view ExtractFuncName(std::string_view fn) noexcept {
            if (auto p = fn.find('('); p != std::string_view::npos) fn = fn.substr(0, p); // 去参数
            if (auto p = fn.find('<'); p != std::string_view::npos) fn = fn.substr(0, p); // 去模板
            if (auto p = fn.rfind("::"); p != std::string_view::npos) fn = fn.substr(p + 2); // 去 ns:: / Class::
            if (auto p = fn.rfind(' '); p != std::string_view::npos) fn = fn.substr(p + 1); // 去返回类型
            return fn;
        }

        struct InitOnly {
            consteval InitOnly(std::source_location loc = std::source_location::current()) {
                if (ExtractFuncName(loc.function_name()) != "Init") {
                    throw "Subscribe* must be called from a function named 'Init'";
                }
            }
        };
    }

    template<typename T>
    class MsgMixin {
        T* This() { return static_cast<T*>(this); }
        MessageManager* pMsgManager = nullptr;
    public:
        MsgMixin() {
            This()->preInits.push_back([this]() {
                this->pMsgManager = This()->FindModule<MessageManager>("MessageManager");
                This()->MainMsgChannel = this->pMsgManager->GetMessageChannel(this->pMsgManager->CreateMessageChannel());
                return true;
                });
        }

        // 异步消息
        auto& SubscribeAsync(const std::string& msgType, Detail::InitOnly = {}) {
            This()->MainMsgChannel->SubscribeAsync(msgType);
            auto full = std::format("{}  Args: <void>", msgType);
            This()->LogSucc(I18n("sys.msg.async.subed{}", full));
            return *this;
        }

        template<typename... Args>
        auto& SubscribeAsync(const std::string& msgType, Detail::InitOnly = {}) {
            if constexpr (sizeof...(Args) == 1 && std::is_same_v<void, std::tuple_element_t<0, std::tuple<Args...>>>) {
                auto h = [](MulNX::Message& msg, std::string_view raw) {return; };
                This()->MainMsgChannel->SubscribeAsync(msgType, std::move(h));
                auto full = std::format("{} Also Externed With Args: <void>", msgType);
                This()->LogSucc(I18n("sys.msg.async.subed{}", full));
            }
            else {
                auto h = MulNX::CreateFiller<Args...>();
                auto typeDesc = MulNX::GetTypeString<Args...>();
                This()->MainMsgChannel->SubscribeAsync(msgType, std::move(h));
                auto full = std::format("{} Also Externed With Args: {}", msgType, typeDesc);
                This()->LogSucc(I18n("sys.msg.async.subed{}", full));
            }
            return *this;
        }

        void PublishAsync(MulNX::Message&& msg)const {
            this->pMsgManager->PublishAsync(std::move(msg));
        }
        void PublishAsync(MulNX::MsgType msg)const {
            this->pMsgManager->PublishAsync(MulNX::Message(msg));
        }

        // 同步消息
        auto& SubscribeSync(const std::string& msgType, MulNX::SyncMsgCallback&& handle, Detail::InitOnly = {}) {
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