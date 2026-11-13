#pragma once
#include <MulNX/Core/Module/Module.hpp>

namespace MulNX {
    class VarReflSystem final : public Module<VarReflSystem> {
        std::unordered_map<std::string_view, size_t> nameToIndex{};
        std::vector<std::unique_ptr<IReflVar>> reflVars{};
        uint64_t successCount = 0;
        ghc::filesystem::path pthCfg{};

        bool Init()override;
        bool Deinit()override;

        void SaveVars();
    public:
        bool Register(std::unique_ptr<IReflVar> var);
    };
}