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
        static consteval std::string_view GetLeafName() {
            std::string_view sig = __FUNCSIG__;
            constexpr std::string_view marker = "Module<";
            auto pos = sig.find(marker);
            if (pos == std::string_view::npos) return {};
            pos += marker.size();
            int depth = 1;
            std::size_t end = pos;
            for (; end < sig.size(); ++end) {
                if (sig[end] == '<') ++depth;
                else if (sig[end] == '>') { if (--depth == 0) break; }
            }
            if (depth != 0) return {};
            std::string_view inner = sig.substr(pos, end - pos);
            for (auto kw : { "class ", "struct ", "enum ", "union " }) {
                std::string_view k{ kw };
                if (inner.starts_with(k)) { inner.remove_prefix(k.size()); break; }
            }
            if (auto lt = inner.find('<'); lt != std::string_view::npos)
                inner = inner.substr(0, lt);
            if (auto p = inner.rfind("::"); p != std::string_view::npos)
                inner.remove_prefix(p + 2);
            return inner;
        }
        
        inline std::string_view GetName()const override { return GetLeafName(); }
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