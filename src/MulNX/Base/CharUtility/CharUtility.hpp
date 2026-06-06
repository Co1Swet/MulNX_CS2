#pragma once

#include <filesystem>
#include <string>

namespace MulNX {
    namespace CharUtility {
        // 从标准库的filesystem::path转换为std::string（utf-8编码）
        std::string FilePathToString(const std::filesystem::path& path);

        std::wstring U8ToW(const std::string& u8String);
        std::string WToU8(const std::wstring& wString);
    }
}