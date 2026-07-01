#pragma once

#include <MulNX/Base/MulNXHandle/MulNXHandle.hpp>
#include <MulNX/Common/Message.hpp>
#include <MulNX/Config/Config.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>

namespace MulNX {
	class MessageChannel final {
		friend class IMessageManager;
		friend class MessageManager;
		MessageManager* MsgManager = nullptr;
		//自身句柄
		MulNXHandle hChannel{};
        moodycamel::ConcurrentQueue<MulNX::Message>Messages;
    public:
		MessageChannel(MessageManager* MsgManager);
        MessageChannel& SubscribeAsync(const std::string& MsgType, std::function<void(MulNX::Message&, std::string_view)>&& makingHandler = nullptr);
		bool PushMessage(Message&& Msg);
		bool PullMessage(Message& OutMsg);
		bool HasMessage()const;
		MulNXHandle GetHandle()const;
	};
}