#pragma once

namespace MulNX {
    class VarReflSystem;

    class VarReflHost {
    public:
        ModuleBase* pModuleBase = nullptr;
        VarReflSystem* pVarReflSystem = nullptr;
    };

    template<typename T>
    class VarReflMixin :public VarReflHost {
        T* This() { return static_cast<T*>(this); }
    public:
        VarReflMixin() {
            this->pModuleBase = static_cast<ModuleBase*>(This());
            This()->preInits.push_back([this]() {
                this->pVarReflSystem = This()->FindModule<VarReflSystem>("VarReflSystem");
                return true;
                });
        }
    };

    class RegisterVar {
    public:
        template <typename Sys, typename U>
        static bool EmplaceRefl(Sys* pSys, ModuleBase* pMod, std::atomic<U>& v, const char* name) {
            return pSys->Emplace(v, pMod, name);   // 依赖表达式，延迟到实例化点查找
        }
        
        template<typename U>
        inline RegisterVar(VarReflHost* pHost, std::atomic<U>& v, const char* name) {
            pHost->pModuleBase->preInits.push_back([pHost,&v,name]() {
                return EmplaceRefl(pHost->pVarReflSystem, pHost->pModuleBase, v, name);
                });
            
            // auto pMixin = static_cast<VarReflMixin<T>*>(pModule);
            // pMixin->
        }
    };
}

#define MulReflInt(name, defaultValue) \
    std::atomic<int> name{defaultValue}; \
    MulNX::RegisterVar name##_MulRefl_re{this, this->##name, #name};

#define MulReflBool(name, defaultValue) \
    std::atomic<bool> name{defaultValue}; \
    MulNX::RegisterVar name##_MulRefl_re{this, this->##name, #name};