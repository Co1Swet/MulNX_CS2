#pragma once
#include <MulNX/Core/Module/IModule.hpp>
#include <MulNX/Systems/UISystem/UINode/UINode.hpp>

namespace MulNX {

    template<typename T>
    class UIMixin {
    private:
        auto CreateUINode(std::string&& name, std::function<void(MulNX::UINode*)>&& func) {
            // 创建UI节点
            MulNX::UINode UINode = MulNX::UINode::Create(This());
            // 设置UI节点属性
            UINode.name = std::move(name);
            UINode.MyFunc = std::move(func);
            // 创建UI消息
            return MulNX::Message::Create<MulNX::UINode>("UISystem/ModulePush"_hash, std::move(UINode));
        }
    public:
        T* This() { return static_cast<T*>(this); }
        UIMixin() {

        }

        void SendUINode(std::string&& name, std::function<void(MulNX::UINode*)>&& func) {
            auto [msg, rp] = this->CreateUINode(std::move(name), std::move(func));
            This()->PublishAsync(std::move(msg));
            This()->LogInfo(I18n("module.send_ui"));
            return;
        }
        void SendUIRoot(std::string&& name, std::function<void(MulNX::UINode*)>&& func) {
            auto [msg, rp] = this->CreateUINode(std::move(name), std::move(func));
            rp->drawAsARoot = true;
            This()->PublishAsync(std::move(msg));
            This()->LogInfo(I18n("module.send_ui"));
        }
    };
}