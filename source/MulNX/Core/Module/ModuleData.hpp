#pragma once
#include "IModule.hpp"
#include <MulNX/Systems/I18nManager/I18n.hpp>
#include <MulNX/Common/Exception.hpp>

namespace MulNX {
    namespace Core {
        class Core;
    }
    class UINode;
    class ModuleData :public IModule {
    public:
        MulNX::Core::Core* Core = nullptr;
        std::vector<std::function<bool()>>preInits{};
        std::vector<std::function<bool()>>postInits{};
        std::vector<std::function<bool()>>preDeinits{};
        std::vector<std::function<bool()>>postDeinits{};
    };
}