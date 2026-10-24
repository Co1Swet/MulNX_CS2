#pragma once
#define GHC_WIN_DISABLE_WSTRING_STORAGE_TYPE
#define GHC_FILESYSTEM_ENFORCE_CPP17_API
#include <MulNXThirdParty/ghc/filesystem.hpp>
#include <expected>
#include <ranges>
#include <unordered_set>

namespace ghc::filesystem {

    namespace fs = ghc::filesystem;

    inline void OpenFolderWindow(const fs::path& path) {
        ShellExecuteW(
            nullptr,          // 父窗口 HWND
            L"open",          // 动词：open / explore
            path.wstring().c_str(),    // 文件夹路径
            nullptr,          // 参数（文件夹不需要）
            nullptr,          // 工作目录（默认）
            SW_SHOWNORMAL     // 显示方式
        );
    }

    inline std::expected<std::vector<std::string>, std::error_code> SubDirectoryNames(const fs::path& path) {
        try {
            return fs::directory_iterator(path)
                | std::views::filter([](const fs::directory_entry& e) {return e.is_directory();})
                | std::views::transform([](const fs::directory_entry& e) {return e.path().filename().string();})
                | std::ranges::to<std::vector>();
        }
        catch (const fs::filesystem_error& e) {
            return std::unexpected(e.code());
        }
    }

    inline std::expected<std::vector<std::string>, std::error_code> SubFileNames(const fs::path& path,
        const std::unordered_set<std::string>& filter = {}) {
        try {
            return fs::directory_iterator(path)
                | std::views::filter([](const fs::directory_entry& e) {
                return e.is_regular_file();}
                )
                | std::views::filter([&](const fs::directory_entry& e) {
                if (!filter.empty())return true;
                return filter.contains(e.path().extension());
                    })
                | std::views::transform([](const fs::directory_entry& e) {return e.path().filename().string();})
                | std::ranges::to<std::vector>();
        }
        catch (const fs::filesystem_error& e) {
            return std::unexpected(e.code());
        }
    }

    inline std::expected<void, std::error_code> FileMove(const fs::path& name, const fs::path& resource, const fs::path& target) {
        if (name.empty() || name.has_parent_path() || name.is_absolute())
            return std::unexpected(std::make_error_code(std::errc::invalid_argument));

        const fs::path src = resource / name;
        const fs::path dst = target / name;

        std::error_code ec;
        fs::rename(src, dst, ec);
        if (!ec) return {};

        fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
        if (ec) return std::unexpected(ec);

        fs::remove(src, ec);
        if (ec) return std::unexpected(ec);
        return {};
    }
}
namespace fs = ghc::filesystem;