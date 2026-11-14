#pragma once
#include <Feature/DemoSystem/DemoSystem.hpp>

template<typename T>
class DemModuleMixin {
    T* This() { return static_cast<T*>(this); }
protected:
    DemoSystem* pDemSys = nullptr;
    DemModuleMixin() {
        This()->preInits.push_back([this]() {
            This()->pDemSys = This()->FindModule<DemoSystem>();
            return true;
            });
    }
};

template <typename T>
class DemModule :public CSModule<T>, public DemModuleMixin<T> {
protected:
    // 立即从有效消息转换出信息，并摆脱消息本身生命周期
    inline std::expected <
        std::tuple<std::string, std::string, fs::path>,
        std::tuple<std::string, std::string, std::string>> ParseDemoPath(MulNX::Message& msg) {
        auto* pExt = msg.asp.get<MulNX::NetExt>();
        if (!pExt) {
            return std::unexpected(std::make_tuple(std::string(), std::string(), "消息中缺少 NetExt"));
        }
        const auto& key = pExt->str1;
        const auto& demoName = pExt->str2;
        auto demoPath = this->Path()->PathGetFromKey(key) / (demoName + ".dem");
        // 确保可拼接为绝对路径
        if (!demoPath.is_absolute()) {
            return std::unexpected(std::make_tuple(key, demoName, "无法拼接为绝对路径"));
        }
        return std::make_tuple(key, demoName, demoPath);
    }
    inline MulNX::Message MakeDemoInfoErrorNotify(const std::string& key, const std::string& demoName) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Info/Notify"_hash);
        auto&& [succ] = msg.Access<int>();
        succ = 1;
        rp->str1 = key;
        rp->str2 = demoName;
        return msg;
    }
};