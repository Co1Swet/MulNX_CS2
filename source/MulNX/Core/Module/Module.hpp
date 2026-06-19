#pragma once
#include "ModuleBase.hpp"
#include <MulNX/Common/Task.hpp>

#include <MulNX/Systems/MessageManager/MsgMixin.hpp>
#include <MulNX/Systems/TaskSystem/TaskMixin.hpp>
#include <MulNX/Systems/UISystem/UIMixin.hpp>
#include <MulNX/Systems/PathManager/PathMixin.hpp>
#include <MulNX/Systems/InputSystem/InputMixin.hpp>
#include <MulNX/Systems/GlobalVars/GlobalVarMixin.hpp>
#include <MulNX/Systems/ShortcutManager/ShortcutMixin.hpp>

#include <thread>
#include <functional>
#include <concepts>
#include <filesystem>

namespace MulNX {
    namespace Core{
        class Driver;
    }
    template<typename T>
    class Module :public ModuleBase,
        public LogMixin<T>, public MsgMixin<T>, public TaskMixin<T>,
        public UIMixin<T>, public PathMixin<T>, public InputMixin<T>,
        public GlobalVarMixin<T>, public ShortcutMixin<T> {
        friend MulNX::Core::Driver;
    public:
        Module() {
            this->backInits->push_back([this]() {
                this->LogSucc(I18n("module.inited"));
                return true;
                });
        }
    };

    template <typename T>
    concept IsModule = std::derived_from<T, MulNX::ModuleBase>;
}