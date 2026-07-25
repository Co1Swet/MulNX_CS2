#pragma once
#include <MulNX/Core/Module/Module.hpp>
#include "UIPack/UIPack.hpp"
#include <map>

namespace MulNX {
    class UICoordinator final :public MulNX::Module<UICoordinator> {
        
        UIPack midPack{};
        UIPack backgoundPack{};

        std::map<uint64_t, std::vector<MulNX::UINode>>UICallbacks{};

        struct Padding {
            float top = 0;
            float bottom = 0;
            float left = 0;
            float right = 0;
        };
        Padding padding;

        bool Init()override;
        void ProcessMsg(MulNX::Message& msg)override;
        void Window();
    public:
        void HandleUpdate();
        void Render(bool mid);
        void CallbackCall(uint64_t hash, Message* msg);
    };
}