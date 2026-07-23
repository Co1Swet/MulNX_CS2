#pragma once
#include "Memory/Hook/Hook.hpp"

template<typename T>
class HookMixin {
    T* This() { return static_cast<T*>(this); }
    std::vector<std::pair<std::string, std::unique_ptr<MulNX::Hook>&>>hooks;
protected:
    HookMixin() {
        This()->postDeinits.push_back([this]() {
            for (auto& [name, hook] : this->hooks) {
                hook = nullptr;
                This()->LogInfo(I18n("hook.destroyed", name));
            }
            return true;
            });
    }

    void RegisterAttachHook(std::unique_ptr<MulNX::Hook>& rhk, std::string&& name,
        std::source_location loc = std::source_location::current()) {
        auto res = rhk->Attach();
        if (res == MulNX::Hook::Result::AttachError)
            throw MulNX::Exception("Hook AttachError: " + name, loc);
        This()->LogSucc(I18n("hook.attached", name));
        this->hooks.push_back({ std::move(name),rhk });
    }
};