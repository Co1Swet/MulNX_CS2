#pragma once
#include <Feature/DemoSystem/DemoSystem.hpp>

template<typename T>
class DemModuleMixin {
    T* This() { return static_cast<T*>(this); }
protected:
    DemoSystem* pDemSys = nullptr;
    DemModuleMixin() {
        This()->preInits.push_back([this]() {
            This()->pDemSys = This()->FindModule<DemoSystem>("DemoSystem");
            return true;
            });
    }
};

class DemModuleBase :public CSModuleBase, public DemModuleMixin<DemModuleBase> {
protected:
    // 立即从有效消息转换出信息，并摆脱消息本身生命周期
    inline std::expected<std::tuple<std::string, std::string, fs::path>, std::string> ParseDemoPath(MulNX::Message& msg) {
        const auto& key = msg.asp.get<MulNX::NetExt>()->str1;
        const auto& demoName = msg.asp.get<MulNX::NetExt>()->str2;
        auto demoPath = this->Path()->PathGetFromKey(key) / (demoName + ".dem");
        // 确保可拼接为绝对路径
        if (!demoPath.is_absolute()) {
            return std::unexpected(std::format("无法拼接为绝对路径: {}", demoPath.string()));
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