#pragma once
#include "Memory/Hook/Hook.hpp"
#include <MulNXUtils/WinExt/Memory/Hook/IATHook.hpp>

template<typename T>
class HookMixin {
    T* This() { return static_cast<T*>(this); }
    std::vector<std::pair<std::string, std::unique_ptr<MulNX::Hook>&>> hooks;
    std::vector<std::pair<std::string, MulNX::Memory::IatHook&>> iatHooks;
protected:
    HookMixin() {
        This()->postDeinits.push_back([this]() {
            for (auto& [name, hook] : this->hooks) {
                hook = nullptr;
                This()->LogInfo(std::format("内联钩子已销毁：{}", name));
            }
            for (auto& [name, hook] : this->iatHooks) {
                hook.Detach();
                This()->LogInfo(std::format("IAT钩子已销毁：{}", name));
            }
            return true;
            });
    }

    void RegisterAttachHook(std::unique_ptr<MulNX::Hook>& rhk, std::string&& name,
        std::source_location loc = std::source_location::current()) {
        auto res = rhk->Attach();
        if (res == MulNX::Hook::Result::AttachError)
            throw MulNX::Exception("Hook AttachError: " + name, loc);
        This()->LogSucc(std::format("内联钩子已附加：{}", name));
        this->hooks.push_back({ std::move(name),rhk });
    }
    void RegisterAttachIatHook(MulNX::Memory::IatHook& rhk, std::string&& name,
        std::source_location loc = std::source_location::current()) {
        if (!rhk.Attach())
            throw MulNX::Exception("IatHook AttachError: " + name, loc);
        This()->LogSucc(std::format("IAT钩子已附加：{}", name));
        this->iatHooks.push_back({ std::move(name), rhk });
    }
};