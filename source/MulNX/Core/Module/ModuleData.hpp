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
        std::vector<std::move_only_function<bool()>> preInits{};
        std::vector<std::move_only_function<bool()>> postInits{};
        std::vector<std::move_only_function<bool()>> preDeinits{};
        std::vector<std::move_only_function<bool()>> postDeinits{};
    };
}