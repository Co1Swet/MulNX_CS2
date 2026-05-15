#include <MulNX/Systems/MessageManager/MessageManager.hpp>
#include <MulNX/Systems/PathManager/PathManager.hpp>
#include <MulNX/Systems/I18nManager/I18nManager.hpp>
#include <MulNX/Systems/ShortcutManager/ShortcutManager.hpp>

MulNX::C_ISys MulNX::ModuleBase::ISys() {
    return C_ISys(this);
}

void MulNX::C_ISys::LogInfo(const std::string& Msg) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Log/Info"_hash);
    rp->str1 = this->pModuleBase->GetName();
    rp->str2 = Msg;
    rp->timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
    this->PublishAsync(std::move(msg));
}
void MulNX::C_ISys::LogSucc(const std::string& Msg) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Log/Succ"_hash);
    rp->str1 = this->pModuleBase->GetName();
    rp->str2 = Msg;
    rp->timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
    this->PublishAsync(std::move(msg));
}
void MulNX::C_ISys::LogWarning(const std::string& Msg) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Log/Warning"_hash);
    rp->str1 = this->pModuleBase->GetName();
    rp->str2 = Msg;
    rp->timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
    this->PublishAsync(std::move(msg));
}
void MulNX::C_ISys::LogError(const std::string& Msg) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Log/Error"_hash);
    rp->str1 = this->pModuleBase->GetName();
    rp->str2 = Msg;
    rp->timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
    this->PublishAsync(std::move(msg));
}

void MulNX::C_ISys::LogLine() {
    this->LogInfo("+------------------------------------------------+");
}
// 异步消息

MulNX::C_ISys& MulNX::C_ISys::SubscribeAsync(const std::string& msgType) {
    this->pModuleBase->MainMsgChannel->SubscribeAsync(msgType);
    this->LogSucc(I18n("sys.msg.async.subed{}", msgType));
    return *this;
}
void MulNX::C_ISys::PublishAsync(MulNX::Message&& msg) {
    this->pModuleBase->pMsgManager->PublishAsync(std::move(msg));
}
void MulNX::C_ISys::PublishAsync(MulNX::MsgType msg) {
    this->pModuleBase->pMsgManager->PublishAsync(MulNX::Message(msg));
}
// 同步消息
MulNX::C_ISys& MulNX::C_ISys::SubscribeSync(const std::string& msgType, MulNX::SyncMsgCallback&& handle) {
    this->pModuleBase->pMsgManager->SubscribeSync(msgType, std::move(handle));
    this->LogSucc(I18n("sys.msg.sync.subed{}", msgType));
    return *this;
}
void MulNX::C_ISys::PublishSync(MulNX::Message& msg) {
    this->pModuleBase->pMsgManager->PublishSync(msg);
}
void MulNX::C_ISys::PublishSync(MulNX::MsgType msg) {
    auto m = MulNX::Message(msg);
    this->pModuleBase->pMsgManager->PublishSync(m);
}

void MulNX::C_ISys::AsyncCommand(std::string&& cmd) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Game/Command"_hash);
    rp->str1 = std::move(cmd);
    this->PublishAsync(std::move(msg));
}

std::filesystem::path MulNX::C_ISys::PathGet(const std::string& Target) {
    return this->pModuleBase->pPathManager->PathGetForModule(this->pModuleBase->GetName(), Target);
}

MulNX::PathManager* MulNX::C_ISys::PathManager() {
    return this->pModuleBase->pPathManager;
}

std::optional<MulNX::KeyCheckPack> MulNX::C_ISys::GetButton(const std::string& name) {
    return this->pModuleBase->pShortcutManager->GetButton(name);
}