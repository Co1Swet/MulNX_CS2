#include "UINode.hpp"

#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>
#include <MulNX/Systems/UISystem/UIContext/UIContext.hpp>
#include <MulNX/Systems/MessageManager/MessageManager.hpp>

void MulNX::UINode::Draw() {
	this->MyFunc(this);
}
bool MulNX::UINode::CallUINode(std::string&& Name) {
    return this->MainContext->CallUINode(Name);
}
bool MulNX::UINode::SetNextUINode(std::string&& Name) {
	this->MainContext->Next = std::move(Name);
	return true;
}

MulNX::UINode MulNX::UINode::Create(MulNX::ModuleBase* MB) {
    MulNX::UINode node;
    node.hSelf = MulNXHandle::CreateHandle();
    node.HModule = MB->HModule;
    return std::move(node);
}