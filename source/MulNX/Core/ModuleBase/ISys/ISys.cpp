#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Systems/UISystem/UISystem.hpp>
#include <MulNX/Systems/MessageManager/MessageManager.hpp>
#include <MulNX/Systems/PathManager/PathManager.hpp>
#include <MulNX/Systems/I18nManager/I18nManager.hpp>
#include <MulNX/Systems/ShortcutManager/ShortcutManager.hpp>
#include <MulNX/Systems/TaskSystem/TaskSystem.hpp>

MulNX::ISys MulNX::ModuleBase::ISys() {
    return MulNX::ISys(this);
}

void MulNX::ISys::LogInfo(const std::string& Msg) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Log/Info"_hash);
    rp->str1 = this->pModuleBase->GetName();
    rp->str2 = Msg;
    rp->timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
    this->PublishAsync(std::move(msg));
}
void MulNX::ISys::LogSucc(const std::string& Msg) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Log/Succ"_hash);
    rp->str1 = this->pModuleBase->GetName();
    rp->str2 = Msg;
    rp->timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
    this->PublishAsync(std::move(msg));
}
void MulNX::ISys::LogWarning(const std::string& Msg) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Log/Warning"_hash);
    rp->str1 = this->pModuleBase->GetName();
    rp->str2 = Msg;
    rp->timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
    this->PublishAsync(std::move(msg));
}
void MulNX::ISys::LogError(const std::string& Msg) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Log/Error"_hash);
    rp->str1 = this->pModuleBase->GetName();
    rp->str2 = Msg;
    rp->timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
    this->PublishAsync(std::move(msg));
}

void MulNX::ISys::LogLine() {
    this->LogInfo("+------------------------------------------------+");
}
// 异步消息

MulNX::ISys& MulNX::ISys::SubscribeAsync(const std::string& msgType) {
    this->pModuleBase->MainMsgChannel->SubscribeAsync(msgType);
    this->LogSucc(I18n("sys.msg.async.subed{}", msgType));
    return *this;
}
void MulNX::ISys::PublishAsync(MulNX::Message&& msg) {
    this->pModuleBase->pMsgManager->PublishAsync(std::move(msg));
}
void MulNX::ISys::PublishAsync(MulNX::MsgType msg) {
    this->pModuleBase->pMsgManager->PublishAsync(MulNX::Message(msg));
}
// 同步消息
MulNX::ISys& MulNX::ISys::SubscribeSync(const std::string& msgType, MulNX::SyncMsgCallback&& handle) {
    this->pModuleBase->pMsgManager->SubscribeSync(msgType, std::move(handle));
    this->LogSucc(I18n("sys.msg.sync.subed{}", msgType));
    return *this;
}
void MulNX::ISys::PublishSync(MulNX::Message& msg) {
    this->pModuleBase->pMsgManager->PublishSync(msg);
}
void MulNX::ISys::PublishSync(MulNX::MsgType msg) {
    auto m = MulNX::Message(msg);
    this->pModuleBase->pMsgManager->PublishSync(m);
}
bool MulNX::ISys::SendUINode(std::string&& name, std::function<void(MulNX::UINode*)>&& func) {
    // 创建UI节点
    MulNX::UINode UINode = MulNX::UINode::Create(this->pModuleBase);
    // 设置UI节点属性
    UINode.name = std::move(name);
    UINode.MyFunc = std::move(func);
    // 创建UI消息
    auto [msg, rp] = MulNX::Message::Create<MulNX::UINode>("UISystem/ModulePush"_hash, std::move(UINode));
    // 发送UI消息
    this->PublishAsync(std::move(msg));
    this->LogInfo(I18n("module.send_ui"));
    return true;
}
void MulNX::ISys::SendTask(std::string&& name, std::string&& targetWorker, std::function<bool()>&& Do) {
    auto fullName = std::format("{}::{}", this->pModuleBase->GetName(), std::move(name));
    this->LogInfo(I18n("module.send_task", fullName, targetWorker));
    auto [msg, rp] = MulNX::Message::Create<MulNX::Task>("Task/Create"_hash,
        std::move(fullName), std::move(targetWorker), std::move(Do));
    this->PublishAsync(std::move(msg));
}

void MulNX::ISys::AsyncCommand(std::string&& cmd) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Game/Command"_hash);
    rp->str1 = std::move(cmd);
    this->PublishAsync(std::move(msg));
}

std::filesystem::path MulNX::ISys::PathGet(const std::string& Target) {
    return this->pModuleBase->pPathManager->PathGetForModule(this->pModuleBase->GetName(), Target);
}

MulNX::PathManager* MulNX::ISys::Path() {
    return this->pModuleBase->pPathManager;
}

std::optional<MulNX::KeyCheckPack> MulNX::ISys::GetButton(const std::string& name) {
    return this->pModuleBase->pShortcutManager->GetButton(name);
}