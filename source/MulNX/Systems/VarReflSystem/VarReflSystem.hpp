#pragma once
#include <MulNX/Core/Module/Module.hpp>

namespace MulNX {

    //   - 主模板覆盖所有算术类型
    //   - 一个偏特化覆盖所有 std::atomic<U>
    //   - 其它类型只需补 ReflCodec<X> 特化，ReflVarT 不动
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

    class IReflVar {
    public:
        virtual ~IReflVar() = default;

        virtual void* GetAddr() = 0;
        virtual const void* GetAddr() const = 0;
        virtual const ModuleBase* GetModule() const = 0;
        virtual const char* GetName()   const = 0;
        virtual std::string       ValueToString() const = 0;
        virtual bool              StringToValue(std::string_view str) = 0;
    };

    template <typename T, typename Codec = ReflCodec<T>>
    class ReflVarT final : public IReflVar {
    public:
        ReflVarT(T* var, const ModuleBase* mod, std::string_view name)
            : pVar(var), pModule(mod), varName(name) {}

        void* GetAddr()       override { return pVar; }
        const void* GetAddr() const override { return pVar; }
        const ModuleBase* GetModule() const override { return pModule; }
        const char* GetName()   const override { return varName.c_str(); }

        std::string ValueToString() const override {
            return Codec::ToString(*pVar);
        }
        bool StringToValue(std::string_view str) override {
            return Codec::FromString(str, *pVar);
        }
    private:
        T* pVar = nullptr;
        const ModuleBase* pModule = nullptr;
        std::string       varName = {};
    };

    // 工厂：省掉模板参数推导
    template <typename T>
    std::unique_ptr<ReflVarT<T>>
        MakeReflVar(T& var, const ModuleBase* mod, std::string_view name) {
        return std::make_unique<ReflVarT<T>>(&var, mod, name);
    }

    class VarReflSystem final : public Module<VarReflSystem> {
        std::unordered_map<std::string, std::unique_ptr<IReflVar>> vars{};
        uint64_t successCount = 0;
        static std::string MakeKey(const ModuleBase* mod, std::string_view name);
    public:
        bool Init() override;

        bool Register(std::unique_ptr<IReflVar> var);

        template <typename T>
        bool Emplace(T& var, const ModuleBase* mod, std::string_view name) {
            auto up = MakeReflVar(var, mod, name);
            IReflVar* raw = up.get();
            return Register(std::move(up));
        }
    };

}