#include "MessageChannel.hpp"
#include <MulNX/Systems/MessageManager/MessageManager.hpp>

MulNX::MessageChannel::MessageChannel(MessageManager* MsgManager) {
	this->MsgManager = MsgManager;
}
MulNX::MessageChannel& MulNX::MessageChannel::SubscribeAsync(const std::string& MsgType) {
    this->MsgManager->SubscribeAsync(this, MsgType);
	return *this;
}
bool MulNX::MessageChannel::PullMessage(Message& OutMsg) {
    return this->Messages.try_dequeue(OutMsg);
}
bool MulNX::MessageChannel::PushMessage(Message&& Msg) {
    return this->Messages.enqueue(std::move(Msg));
}
bool MulNX::MessageChannel::HasMessage()const {
    return this->Messages.size_approx();
}

MulNXHandle MulNX::MessageChannel::GetHandle()const {
	return this->hChannel;
}