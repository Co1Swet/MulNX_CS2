#pragma once
#include <MulNX/Core/Module/IModule.hpp>
#include <MulNX/Systems/UISystem/UIContext/UINode/UINode.hpp>

namespace MulNX {

    template<typename Derived>
    class UIMixin {
    public:
        Derived* This() { return static_cast<Derived*>(this); }
        UIMixin() {

        }

        bool SendUINode(std::string&& name, std::function<void(MulNX::UINode*)>&& func) {
            // 创建UI节点
            MulNX::UINode UINode = MulNX::UINode::Create(This());
            // 设置UI节点属性
            UINode.name = std::move(name);
            UINode.MyFunc = std::move(func);
            // 创建UI消息
            auto [msg, rp] = MulNX::Message::Create<MulNX::UINode>("UISystem/ModulePush"_hash, std::move(UINode));
            // 发送UI消息
            This()->PublishAsync(std::move(msg));
            This()->LogInfo(I18n("module.send_ui"));
            return true;
        }

    };
}