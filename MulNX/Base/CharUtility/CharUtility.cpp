#include "CharUtility.hpp"

#include <Windows.h>

std::string MulNX::CharUtility::FilePathToString(const std::filesystem::path& path) {
	std::u8string u8path = path.u8string();
	std::string utf8Path(u8path.begin(), u8path.end());
	std::replace(utf8Path.begin(), utf8Path.end(), '\\', '/');
	return utf8Path;
}

std::wstring MulNX::CharUtility::U8ToW(const std::string& u8String) {
    if (u8String.empty()) return {};

    int wStringLen = MultiByteToWideChar(CP_UTF8, 0,
        u8String.data(), static_cast<int>(u8String.length()), nullptr, 0);
    if (wStringLen <= 0) return {};

    std::wstring wString(wStringLen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0,
        u8String.data(), static_cast<int>(u8String.length()),
        wString.data(), wStringLen);
    return wString;
}
std::string MulNX::CharUtility::WToU8(const std::wstring& wString) {
    if (wString.empty()) return {};

    int u8StringLen = WideCharToMultiByte(CP_UTF8, 0,
        wString.data(), static_cast<int>(wString.length()), nullptr, 0, nullptr, nullptr);
    if (u8StringLen <= 0) return {};

    std::string u8String(u8StringLen, '\0');
    WideCharToMultiByte(CP_UTF8, 0,
        wString.data(), static_cast<int>(wString.length()),
        u8String.data(), u8StringLen, nullptr, nullptr);
    return u8String;
}