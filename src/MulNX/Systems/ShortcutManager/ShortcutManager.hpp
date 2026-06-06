#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <MulNX/Systems/InputSystem/Key/Key.hpp>

namespace MulNX {
    class ShortcutManager final :public MulNX::ModuleBase {
        class Bind {
        public:
            std::string desc;
            std::string msg;
            MulNX::KeyCheckPack KCP;
        };
        std::vector<Bind>binds;

        std::unordered_map<std::string, MulNX::KeyCheckPack>buttons;

        void Check();
    public:
        bool Init();
        std::optional<MulNX::KeyCheckPack> GetButton(const std::string& name);
    };
}