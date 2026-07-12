#include "UINode.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/Systems.hpp>

MulNX::UINode MulNX::UINode::Create(MulNX::ModuleBase* MB) {
    MulNX::UINode node;
    node.hSelf = MulNXHandle::CreateHandle();
    node.HModule = MB->HModule;
    return std::move(node);
}