#pragma once
#include "Memory/Hook/Hook.hpp"

template<typename T>
class HookMixin {
protected:
    T* This() { return static_cast<T*>(this); }

    class WrapHook {
        T* pMod = nullptr;
        std::string name;
        std::unique_ptr<MulNX::Hook> hook;
        bool Added = false;
    public:
        WrapHook() = default;
        WrapHook(T* mod, std::string&& name, std::unique_ptr<MulNX::Hook> hk)
            : pMod(mod), name(std::move(name)), hook(std::move(hk)) {}
        void Attach() {
            auto res = this->hook->Attach();
            if (res == MulNX::Hook::Result::AttachError)
                throw std::runtime_error("Hook AttachError: " + this->name);
            this->pMod->LogSucc(I18n("hook.attached", this->name));
            if (this->Added)return;
            this->Added = true;
            this->pMod->beforeDeinits->push_back([this]() {
                if (!this->hook)return true;
                this->hook.reset();
                this->pMod->LogInfo(I18n("hook.destroyed", this->name));
                return true;
                });
        }
        auto Detach() { return hook->Detach(); }
    };

    template<typename... Args>
    auto CreateHook(std::string&& name, Args&&... args) -> std::expected<WrapHook, std::string> {
        auto expectedHook = MulNX::Hook::Create(std::forward<Args>(args)...);
        if (!expectedHook)return std::unexpected(expectedHook.error());
        return WrapHook(This(), std::move(name), std::move(*expectedHook));
    }
};