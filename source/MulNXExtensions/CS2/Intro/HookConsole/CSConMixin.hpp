#pragma once
#include <MulNX/Base/UI/UI.hpp>

template <size_t n>
struct CSConvarName {
    char data[n]{};
    consteval CSConvarName(const char(&str)[n]) {
        std::copy_n(str, n, data);
    }
    constexpr operator const char* () const { return data; }
};

template<typename T>
class CSConMixin {
    T* This() { return static_cast<T*>(this); }
    const T* This() const { return static_cast<const T*>(this); }
protected:
    template<CSConvarName name>
    auto ConvarSliderInt(const std::string& UIName, const int& min, const int& max) {
        static auto* pVar = This()->CS2Con->GetCvar(std::string(name))->template GetPtr<int>();
        return ImGui::SliderInt(UIName.c_str(), pVar, min, max);
    }

    template<CSConvarName name>
    auto ConvarSliderFloat(const std::string& UIName, const float& min, const float& max) {
        static auto* pVar = This()->CS2Con->GetCvar(std::string(name))->template GetPtr<float>();
        return ImGui::SliderFloat(UIName.c_str(), pVar, min, max);
    }

    template<CSConvarName name>
    auto ConvarCheckbox(const std::string& UIName) {
        static auto* pVar = This()->CS2Con->GetCvar(std::string(name))->template GetPtr<bool>();
        return ImGui::Checkbox(UIName.c_str(), pVar);
    }

    void AsyncCommand(std::string&& cmd)const {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Game/Command"_hash);
        rp->str1 = std::move(cmd);
        This()->PublishAsync(std::move(msg));
    }
    void AsyncCommandNoReport(std::string&& cmd)const {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Game/Command/NoReport"_hash);
        rp->str1 = std::move(cmd);
        This()->PublishAsync(std::move(msg));
    }
    void AsyncCommandHighPriority(std::string&& cmd)const {
        This()->LogInfo(std::format("发送高优先命令：{}", cmd));
        This()->CS2Con->bufferHighPriorityGameCmds.enqueue(std::move(cmd));
    }
};