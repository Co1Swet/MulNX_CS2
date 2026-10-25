#pragma once
#include "ModuleBase.hpp"
#include <MulNX/Systems/Logger/LogMixin.hpp>
#include <MulNX/Systems/MessageManager/MsgMixin.hpp>
#include <MulNX/Systems/TaskSystem/TaskMixin.hpp>
#include <MulNX/Systems/UISystem/UIMixin.hpp>
#include <MulNX/Systems/PathManager/PathMixin.hpp>
#include <MulNX/Systems/InputSystem/InputMixin.hpp>
#include <MulNX/Systems/GlobalVars/GlobalVarMixin.hpp>
#include <MulNX/Systems/ShortcutManager/ShortcutMixin.hpp>
#include <MulNX/Systems/VarReflSystem/VarReflMixin.hpp>

namespace MulNX {
    namespace Core{
        class Driver;
    }
    template<typename T>
    class Module :public ModuleBase,
        public LogMixin<T>, public MsgMixin<T>, public TaskMixin<T>,
        public UIMixin<T>, public PathMixin<T>, public InputMixin<T>,
        public GlobalVarMixin<T>, public ShortcutMixin<T>, public VarReflMixin<T> {
        friend MulNX::Core::Driver;
    public:
        Module() {
            this->postInits.push_back([this]() {
                this->LogSucc("初始化成功");
                return true;
                });

            this->postDeinits.push_back([this]() {
                this->LogInfo("模块已关闭");
                return true;
                });
        }
    };

    template <typename T>
    concept IsModule = std::derived_from<T, MulNX::ModuleBase>;
}