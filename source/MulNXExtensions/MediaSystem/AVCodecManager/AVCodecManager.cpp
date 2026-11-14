#include "AVCodecManager.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>

void AVCodecManager::Menu() {
    if (!ImGui::CollapsingHeader("视频编码器设置")) {
        return;
    }

    auto* current = this->GetCurrentVEncoder();
    auto currentNameValue = this->currentEncoderName.load(std::memory_order_acquire);
    const std::string_view currentName = currentNameValue ?
        std::string_view(*currentNameValue) : std::string_view{};

    const std::string_view preview = current ? current->GetImplModuleName() : "无可用编码器";
    if (ImGui::BeginCombo("当前编码器", preview.data())) {
        for (auto* encoder : this->encoders) {
            const std::string_view name = encoder->GetImplModuleName();
            const bool selected = name == currentName;
            if (!encoder->IsAvailable()) {
                ImGui::BeginDisabled();
                ImGui::Selectable(name.data(), selected);
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextDisabled("不可用：%s", encoder->GetNotAvailableReason());
                continue;
            }
            if (ImGui::Selectable(name.data(), selected)) {
                this->currentEncoderName.store(std::make_shared<std::string>(name), std::memory_order_release);
                this->selectionStatus.clear();
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    current = this->GetCurrentVEncoder();
    if (!this->selectionStatus.empty()) {
        ImGui::TextWrapped("%s", this->selectionStatus.c_str());
    }
    bool hasUnavailableEncoder = std::any_of(this->encoders.begin(), this->encoders.end(), [](auto* encoder) {
        return !encoder->IsAvailable();
        });
    if (hasUnavailableEncoder && ImGui::CollapsingHeader("不可用编码器", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (auto* encoder : this->encoders) {
            if (!encoder->IsAvailable()) {
                const std::string_view name = encoder->GetImplModuleName();
                ImGui::TextWrapped("%s：%s", name.data(), encoder->GetNotAvailableReason());
            }
        }
    }
    ImGui::SeparatorText("当前编码器设置");
    if (!current) {
        ImGui::TextUnformatted("没有可用的视频编码器。请检查探测原因和回退链配置。");
        return;
    }
    current->SettingsMenu();
}

bool AVCodecManager::Init() {
    this->ResolveStartupSelection();

    this->UIRegisterCallback("UI.MediaSys", [this](auto&&...) {this->Menu();});

    return true;
}

void AVCodecManager::AddEncoder(MulNX::IVEncoder* pEncoder) {
    if (!pEncoder) {
        this->LogError("尝试注册空的视频编码器");
        return;
    }
    const std::string_view name = pEncoder->GetImplModuleName();
    if (name.empty()) {
        this->LogError("尝试注册没有模块名的视频编码器");
        return;
    }
    if (this->FindEncoder(name)) {
        this->LogError(std::format("重复的视频编码器模块名：{}", name));
        return;
    }
    this->encoders.push_back(pEncoder);
}

MulNX::IVEncoder* AVCodecManager::FindEncoder(std::string_view name) const {
    auto it = std::find_if(this->encoders.begin(), this->encoders.end(), [&name](const auto* encoder) {
        return name == encoder->GetImplModuleName();
        });
    return it == this->encoders.end() ? nullptr : *it;
}

void AVCodecManager::ResolveStartupSelection() {
    auto preferredNameValue = this->currentEncoderName.load(std::memory_order_acquire);
    const std::string_view preferredName = std::string_view(*preferredNameValue);

    if (auto* preferred = this->FindEncoder(preferredName); preferred && preferred->IsAvailable()) {
        this->selectionStatus.clear();
        return;
    }

    if (preferredName.empty()) {
        this->selectionStatus = "未设置首选编码器，正在尝试回退链。";
    }
    else if (auto* preferred = this->FindEncoder(preferredName)) {
        this->selectionStatus = std::format(
            "首选编码器 {} 不可用：{}；正在尝试回退链。",
            preferred->GetImplModuleName(), preferred->GetNotAvailableReason());
    }
    else {
        this->selectionStatus = std::format(
            "首选编码器模块名“{}”未注册；正在尝试回退链。", preferredName);
    }

    std::vector<std::string> fallbackIds;
    std::string fallbackError;
    auto fallbackJsonValue = this->encoderFallbackModuleNames.load(std::memory_order_acquire);
    if (!fallbackJsonValue) {
        fallbackError = "回退链为空";
    }
    else try {
        const auto fallback = nlohmann::json::parse(*fallbackJsonValue);
        if (!fallback.is_array()) {
            fallbackError = "回退链必须是 JSON 字符串数组";
        }
        else {
            for (const auto& entry : fallback) {
                if (!entry.is_string()) {
                    fallbackError = "回退链中的每一项都必须是字符串";
                    break;
                }
                fallbackIds.push_back(entry.get<std::string>());
            }
        }
    }
    catch (const nlohmann::json::exception& e) {
        fallbackError = std::format("回退链 JSON 无效：{}", e.what());
    }
    if (!fallbackError.empty()) {
        this->selectionStatus += std::format(" 回退链配置无效：{}。", fallbackError);
        this->LogError(std::string(this->selectionStatus));
        this->currentEncoderName.store(std::make_shared<std::string>(), std::memory_order_release);
        return;
    }

    std::vector<std::string> rejectedFallbacks;
    for (const auto& fallbackId : fallbackIds) {
        auto* fallback = this->FindEncoder(fallbackId);
        if (!fallback) {
            rejectedFallbacks.push_back(std::format("{}：未注册", fallbackId));
            continue;
        }
        if (!fallback->IsAvailable()) {
            rejectedFallbacks.push_back(std::format(
                "{}：{}", fallback->GetImplModuleName(), fallback->GetNotAvailableReason()));
            continue;
        }
        this->currentEncoderName.store(std::make_shared<std::string>(fallbackId), std::memory_order_release);
        if (!rejectedFallbacks.empty()) {
            this->selectionStatus += " 已跳过不可用回退项：";
            for (size_t i = 0; i < rejectedFallbacks.size(); ++i) {
                if (i > 0) {
                    this->selectionStatus += "；";
                }
                this->selectionStatus += rejectedFallbacks[i];
            }
            this->selectionStatus += "。";
        }
        this->selectionStatus += std::format(
            " 已回退到 {}。", fallback->GetImplModuleName());
        this->LogWarning(std::string(this->selectionStatus));
        return;
    }

    this->currentEncoderName.store(std::make_shared<std::string>(), std::memory_order_release);
    if (!rejectedFallbacks.empty()) {
        this->selectionStatus += " 回退项均不可用：";
        for (size_t i = 0; i < rejectedFallbacks.size(); ++i) {
            if (i > 0) {
                this->selectionStatus += "；";
            }
            this->selectionStatus += rejectedFallbacks[i];
        }
        this->selectionStatus += "。";
    }
    this->selectionStatus += " 回退链中没有可用的已注册编码器。";
    this->LogError(std::string(this->selectionStatus));
}

MulNX::IVEncoder* AVCodecManager::GetCurrentVEncoder() {
    auto currentName = this->currentEncoderName.load(std::memory_order_acquire);
    const std::string_view name = currentName ? std::string_view(*currentName) : std::string_view{};
    auto* encoder = this->FindEncoder(name);
    return encoder && encoder->IsAvailable() ? encoder : nullptr;
}