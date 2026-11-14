#pragma once
#include <MulNX/Base/UI/UI.hpp>

template<typename T>
class CSConMixin {
    T* This() { return static_cast<T*>(this); }
    const T* This() const { return static_cast<const T*>(this); }
protected:
    template<MulNX::FixedString name>
    auto ConvarSliderInt(const std::string& UIName, const int& min, const int& max) {
        static auto* pVar = This()->CS2Con->GetCVarByName(name)->template GetPtr<int>();
        return ImGui::SliderInt(UIName.c_str(), pVar, min, max);
    }

    template<MulNX::FixedString name>
    auto ConvarSliderFloat(const std::string& UIName, const float& min, const float& max) {
        static auto* pVar = This()->CS2Con->GetCVarByName(name)->template GetPtr<float>();
        return ImGui::SliderFloat(UIName.c_str(), pVar, min, max);
    }

    template<MulNX::FixedString name>
    auto ConvarCheckbox(const std::string& UIName) {
        static auto* pVar = This()->CS2Con->GetCVarByName(name)->template GetPtr<bool>();
        return ImGui::Checkbox(UIName.c_str(), pVar);
    }

    void AsyncCommand(std::string&& cmd)const {
        This()->LogInfo(std::format("推入MulNX缓冲队列：{}", cmd));
        This()->CS2Con->bufferGameCmds.enqueue(std::move(cmd));
    }
    void AsyncCommandNoReport(std::string&& cmd)const {
        This()->CS2Con->bufferGameCmds.enqueue(std::move(cmd));
    }
};