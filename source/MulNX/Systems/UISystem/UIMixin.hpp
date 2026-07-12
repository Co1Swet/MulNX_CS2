#pragma once
#include <MulNX/Core/Module/IModule.hpp>
#include <MulNX/Systems/UISystem/UINode/UINode.hpp>

namespace MulNX {

    template<typename T>
    class UIMixin {
        T* This() { return static_cast<T*>(this); }
        auto CreateUINode(std::string&& name, std::function<void(UICoordinator*, Message*)>&& func) {
            MulNX::UINode UINode = MulNX::UINode::Create(This());
            UINode.name = std::move(name);
            UINode.Render = std::move(func);
            return MulNX::Message::Create<MulNX::UINode>("UISystem/ModulePush"_hash, std::move(UINode));
        }
    public:
        void SendUINode(std::string&& name, std::function<void(UICoordinator*, Message*)>&& func) {
            auto [msg, rp] = this->CreateUINode(std::move(name), std::move(func));
            This()->PublishAsync(std::move(msg));
            This()->LogInfo(I18n("module.send_ui"));
            return;
        }
        void SendUIRoot(std::string&& name, std::function<void(UICoordinator*, Message*)>&& func) {
            auto [msg, rp] = this->CreateUINode(std::move(name), std::move(func));
            rp->drawAsARoot = true;
            This()->PublishAsync(std::move(msg));
            This()->LogInfo(I18n("module.send_ui"));
        }
        void UIRegisterCallback(const char* name, std::function<void(UICoordinator*, Message*)>&& func) {
            MulNX::UINode UINode = MulNX::UINode::Create(This());
            UINode.name = std::string(name);
            UINode.Render = std::move(func);
            auto [msg, pNode] = MulNX::Message::Create<MulNX::UINode>("UISystem/UICallback"_hash);
            *pNode = std::move(UINode);
            auto&& [target, str] = msg.Access<uint64_t, const char*>();
            target = MulNX::HashString(name);
            str = name;
            This()->PublishAsync(std::move(msg));
        }
    };
}