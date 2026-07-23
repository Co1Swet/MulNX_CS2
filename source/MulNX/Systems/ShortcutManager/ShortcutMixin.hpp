#pragma once
#include <MulNX/Core/Module/IModule.hpp>

namespace MulNX {
    class ShortcutManager;
    template<typename T>
    class ShortcutMixin {
        T* This() { return static_cast<T*>(this); }
        ShortcutManager* pShortcutManager = nullptr;
    public:
        ShortcutMixin() {
            This()->preInits.push_back([this]() {
                this->pShortcutManager = static_cast<ShortcutManager*>(This()->FindModule("ShortcutManager"));
                return true;
                });
        }
        ShortcutManager* Shortcut() {
            return this->pShortcutManager;
        }
    };
}