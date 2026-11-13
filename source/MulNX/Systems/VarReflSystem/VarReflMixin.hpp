#pragma once

namespace MulNX {
    class IReflVar {
    public:
        virtual ~IReflVar() = default;

        virtual void* GetAddr() = 0;
        virtual const void* GetAddr() const = 0;
        virtual const ModuleBase* GetModule() const = 0;
        virtual const char* GetName() const = 0;
        virtual void UpdateKey() = 0;
        virtual std::string_view GetFullKey() const = 0;
        virtual std::string ValueToString() const = 0;
        virtual bool StringToValue(std::string_view str) = 0;
    };

    template <typename T>
    struct ReflCodec {
        static_assert(std::is_arithmetic_v<T>,
            "ReflCodec<T>: specialize ReflCodec for non-arithmetic T");

        static std::string ToString(T value) {
            if constexpr (std::is_same_v<T, bool>) {
                return value ? "true" : "false";
            }
            else {
                char buf[64];
                auto [p, ec] = std::to_chars(buf, buf + sizeof(buf), value);
                if (ec != std::errc{}) return {};
                return std::string(buf, p);
            }
        }

        static bool FromString(std::string_view str, T& out) {
            if constexpr (std::is_same_v<T, bool>) {
                if (str == "true" || str == "1") { out = true;  return true; }
                if (str == "false" || str == "0") { out = false; return true; }
                return false;
            }
            else {
                T tmp{};
                auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), tmp);
                if (ec != std::errc{} || p != str.data() + str.size()) return false;
                out = tmp;
                return true;
            }
        }
    };

    template <typename U>
    struct ReflCodec<std::atomic<U>> {
        static std::string ToString(const std::atomic<U>& value) {
            return ReflCodec<U>::ToString(value.load(std::memory_order_acquire));
        }
        static bool FromString(std::string_view str, std::atomic<U>& value) {
            U tmp{};
            if (!ReflCodec<U>::FromString(str, tmp)) return false;
            value.store(tmp, std::memory_order_release);
            return true;
        }
    };

    template <>
    struct ReflCodec<std::string> {
        static std::string ToString(const std::string& value) {
            return value;
        }
        static bool FromString(std::string_view str, std::string& out) {
            out.assign(str.data(), str.size());
            return true;
        }
    };

    template <typename T>
    struct ReflCodec<std::shared_ptr<T>> {
        static std::string ToString(const std::shared_ptr<T>& p) {
            return p ? ReflCodec<T>::ToString(*p) : std::string{};
        }
        static bool FromString(std::string_view str, std::shared_ptr<T>& p) {
            T tmp{};
            if (!ReflCodec<T>::FromString(str, tmp)) return false;
            p = std::make_shared<T>(std::move(tmp));
            return true;
        }
    };

    template <typename T, typename Codec = ReflCodec<T>>
    class ReflVarT final : public IReflVar {
        T* pVar;
        const ModuleBase* pModule = nullptr;
        std::string varName;
        std::string fullKey;
    public:
        ReflVarT(T* var, const ModuleBase* mod, std::string_view name)
            : pVar(var), pModule(mod), varName(name) {}

        void* GetAddr() override { return pVar; }
        const void* GetAddr() const override { return pVar; }
        const ModuleBase* GetModule() const override { return pModule; }
        const char* GetName() const override { return varName.c_str(); }
        void UpdateKey() override { this->fullKey = std::format("{}.{}", this->pModule->GetName(), this->varName); }
        std::string_view GetFullKey() const override { return fullKey; }

        std::string ValueToString() const override {
            return Codec::ToString(*pVar);
        }
        bool StringToValue(std::string_view str) override {
            return Codec::FromString(str, *pVar);
        }        
    };

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
        template <typename Sys>
        static bool EmplaceRefl(Sys* pSys, std::unique_ptr<IReflVar> iVar) {
            return pSys->Register(std::move(iVar));
        }
    public:
        template<typename U>
        inline RegisterVar(VarReflHost* pHost, std::atomic<U>* pVar, const char* name) {
            auto up = std::make_unique<ReflVarT<std::atomic<U>>>(pVar, pHost->pModuleBase, name);
            pHost->pModuleBase->preInits.push_back([pHost, iVar = std::move(up)]()mutable->bool {
                return EmplaceRefl(pHost->pVarReflSystem, std::move(iVar));
                });
        }
    };
}

#define MulReflInt(name, defaultValue) \
    std::atomic<int> name{defaultValue}; \
    MulNX::RegisterVar _mulrefl_##name##_register{this, &this->name, #name};

#define MulReflBool(name, defaultValue) \
    std::atomic<bool> name{defaultValue}; \
    MulNX::RegisterVar _mulrefl_##name##_register{this, &this->name, #name};

#define MulReflFloat(name, defaultValue) \
    std::atomic<float> name{defaultValue}; \
    MulNX::RegisterVar _mulrefl_##name##_register{this, &this->name, #name};

#define MulReflStr(name, defaultValue) \
    std::atomic<std::shared_ptr<std::string>> name{std::make_shared<std::string>(defaultValue)}; \
    MulNX::RegisterVar _mulrefl_##name##_register{this, &this->name, #name};