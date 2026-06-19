#include "UINode.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/Systems.hpp>

void MulNX::UINode::Draw() {
	this->MyFunc(this);
}
bool MulNX::UINode::CallUINode(std::string&& Name) {
    this->pCoordinator->CallUINode(std::move(Name));
    return true;
}
MulNX::UINode MulNX::UINode::Create(MulNX::ModuleBase* MB) {
    MulNX::UINode node;
    node.hSelf = MulNXHandle::CreateHandle();
    node.HModule = MB->HModule;
    return std::move(node);
}