#pragma once
#define GHC_WIN_DISABLE_WSTRING_STORAGE_TYPE
#define GHC_FILESYSTEM_ENFORCE_CPP17_API
#include <MulNXThirdParty/ghc/filesystem.hpp>

namespace ghc::filesystem {
    inline void OpenFolderWindow(const ghc::filesystem::path& path) {
        ShellExecuteW(
            nullptr,          // 父窗口 HWND
            L"open",          // 动词：open / explore
            path.wstring().c_str(),    // 文件夹路径
            nullptr,          // 参数（文件夹不需要）
            nullptr,          // 工作目录（默认）
            SW_SHOWNORMAL     // 显示方式
        );
    }
}
namespace fs = ghc::filesystem;