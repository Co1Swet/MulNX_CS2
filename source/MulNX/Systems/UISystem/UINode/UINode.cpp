#include "UINode.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/Systems.hpp>

MulNX::UINode MulNX::UINode::Create(MulNX::IModule* pModule) {
    MulNX::UINode node;
    node.hSelf = MulNXHandle::CreateHandle();
    node.HModule = pModule->GetHandle();
    return node;
}