#pragma once
#include <string>
#include <format>

std::string I18n(const std::string& key);

template<typename... Args>
std::string I18n(const std::string& key, Args&&... args) {
    return std::vformat(I18n(key), std::make_format_args((args)...));
}