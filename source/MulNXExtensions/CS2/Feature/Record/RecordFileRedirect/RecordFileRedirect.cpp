#include "RecordFileRedirect.hpp"
#include <MulNX/Base/CharUtility/CharUtility.hpp>

bool RecordFileRedirect::Init() {
    this->dirVideos = this->Path()->PathGetForShared("Videos");

    this->SubscribeSync("MediaSync/SetOn", [this](MulNX::Message&) {
        auto current = this->pMediaState->pCurrentOutputDir.load(std::memory_order_acquire);
        std::filesystem::path snapshotDir = current ? *current : this->dirVideos;

        this->redirectBaseSnapshot.store(
            std::make_shared<std::filesystem::path>(snapshotDir),
            std::memory_order_release
        );

        this->LogInfo(std::string("重定向文件夹快照已更新: ") + snapshotDir.string());
    });

    return true;
}

std::optional<MulNX::Hook::Then> RecordFileRedirect::OnCreateFileW(CreateFileWControl* pfc) {
    std::wstring_view clean = pfc->GetCleanSrc();

    const std::wstring_view kMovieDir = L"game\\csgo\\movie\\";

    size_t pos = std::wstring_view::npos;
    if (clean.size() >= kMovieDir.size()) {
        for (size_t i = 0; i <= clean.size() - kMovieDir.size(); ++i) {
            if (_wcsnicmp(clean.data() + i, kMovieDir.data(), kMovieDir.size()) == 0) {
                pos = i;
                break;
            }
        }
    }

    if (pos == std::wstring_view::npos)
        return std::nullopt;

    std::wstring_view afterMovie = clean.substr(pos + kMovieDir.size());
    if (afterMovie.empty())
        return std::nullopt;

    auto lastSlash = afterMovie.rfind(L'\\');
    std::wstring_view filename = (lastSlash == std::wstring_view::npos)
        ? afterMovie
        : afterMovie.substr(lastSlash + 1);

    if (filename.empty())
        return std::nullopt;

    auto current = this->redirectBaseSnapshot.load(std::memory_order_acquire);
    std::filesystem::path targetRoot = current ? *current : this->dirVideos;
    std::filesystem::path targetPath = targetRoot / filename;

    std::error_code ec;
    std::filesystem::create_directories(targetPath.parent_path(), ec);

    std::wstring newFullPath = L"\\\\?\\" + targetPath.wstring();

    thread_local std::wstring tls_redirectPath;
    tls_redirectPath = std::move(newFullPath);

    pfc->redirected = &tls_redirectPath;

    // 使用 std::format 统一日志格式，WToU8 统一使用全限定名
    this->LogInfo(std::format("重定向文件创建: {} -> {}",
        MulNX::CharUtility::WToU8(std::wstring(pfc->GetRaw())),
        MulNX::CharUtility::WToU8(tls_redirectPath)));

    return MulNX::Hook::Then::Continue;
}

std::optional<MulNX::Hook::Then> RecordFileRedirect::OnGetFileAttributesExW(GetFileAttributesExWControl* pac) {
    std::wstring_view clean = pac->GetCleanSrc();

    const std::wstring_view kMovieDir = L"game\\csgo\\movie\\";

    size_t pos = std::wstring_view::npos;
    if (clean.size() >= kMovieDir.size()) {
        for (size_t i = 0; i <= clean.size() - kMovieDir.size(); ++i) {
            if (_wcsnicmp(clean.data() + i, kMovieDir.data(), kMovieDir.size()) == 0) {
                pos = i;
                break;
            }
        }
    }

    if (pos == std::wstring_view::npos)
        return std::nullopt;

    std::wstring_view afterMovie = clean.substr(pos + kMovieDir.size());
    if (afterMovie.empty())
        return std::nullopt;

    auto lastSlash = afterMovie.rfind(L'\\');
    std::wstring_view filename = (lastSlash == std::wstring_view::npos)
        ? afterMovie
        : afterMovie.substr(lastSlash + 1);

    if (filename.empty())
        return std::nullopt;

    auto current = this->redirectBaseSnapshot.load(std::memory_order_acquire);
    std::filesystem::path targetRoot = current ? *current : this->dirVideos;
    std::filesystem::path targetPath = targetRoot / filename;
    std::wstring newPath = L"\\\\?\\" + targetPath.wstring();

    BOOL result = pac->WrapGetFileAttributesExW(newPath.c_str());
    pac->retResult = result;

    this->LogInfo(std::format("重定向文件属性查询: {} -> {} (结果: {})",
        MulNX::CharUtility::WToU8(std::wstring(pac->GetRaw())),
        MulNX::CharUtility::WToU8(newPath),
        (result != INVALID_FILE_ATTRIBUTES ? "成功" : "失败")));

    return MulNX::Hook::Then::Return;
}

std::optional<MulNX::Hook::Then> RecordFileRedirect::OnCreateDirectoryW(CreateDirectoryWControl* pdc) {
    std::wstring_view clean = pdc->GetCleanSrc();

    const std::wstring_view kMovieDir = L"game\\csgo\\movie\\";

    size_t pos = std::wstring_view::npos;
    if (clean.size() >= kMovieDir.size()) {
        for (size_t i = 0; i <= clean.size() - kMovieDir.size(); ++i) {
            if (_wcsnicmp(clean.data() + i, kMovieDir.data(), kMovieDir.size()) == 0) {
                pos = i;
                break;
            }
        }
    }

    if (pos == std::wstring_view::npos)
        return std::nullopt;

    pdc->retResult = TRUE;

    this->LogInfo(std::format("拦截目录创建，直接返回成功: {}",
        MulNX::CharUtility::WToU8(std::wstring(pdc->GetRaw()))));

    return MulNX::Hook::Then::Return;
}