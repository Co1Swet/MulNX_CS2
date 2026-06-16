#pragma once
#include <MulNX/Core/Module/Module.hpp>

namespace YAML {
    class Node;
}

namespace MulNX {
    class I18nManager final :public MulNX::Module<I18nManager> {
    private:
        void LoadYaml(const YAML::Node& node, const std::string& key);
        std::unordered_map<std::string, std::string>strings{};
    public:
        inline static I18nManager* pThis = nullptr;
        I18nManager();
        bool Init()override;
        const std::string& Get(const std::string& key);
    };
    
}