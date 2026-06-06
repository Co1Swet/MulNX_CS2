#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>

namespace YAML {
    class Node;
}

namespace MulNX {
    class I18nManager final :public MulNX::ModuleBase {
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

const std::string& I18n(const std::string& key);

template<typename... Args>
std::string I18n(const std::string& key, Args&&... args) {
    return std::vformat(I18n(key), std::make_format_args((args)...));
}