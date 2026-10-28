#pragma once
#include <MulNX/Core/Module/Module.hpp>

namespace MulNX {
    class VarReflSystem final : public Module<VarReflSystem> {
        std::unordered_map<std::string, std::unique_ptr<IReflVar>> vars{};
        uint64_t successCount = 0;
        ghc::filesystem::path pthCfg{};
        static std::string MakeKey(const ModuleBase* mod, std::string_view name);

        bool Init()override;
        bool Deinit()override;
    public:
        bool Register(std::unique_ptr<IReflVar> var);
    };
}