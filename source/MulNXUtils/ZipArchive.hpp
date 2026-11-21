#pragma once
#include <miniz.h>
#include <MulNX/Base/fs.hpp>
#include <cerrno>
#include <cstdio>
#include <memory>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace MulNX::Archive {
    inline std::expected<std::vector<fs::path>, std::error_code> ExtractZipFiles(
        const fs::path& archivePath,
        const fs::path& destination,
        std::string_view extension);
}

namespace MulNX::Archive::Detail {
    class ZipErrorCategory final : public std::error_category {
    public:
        const char* name() const noexcept override {
            return "MulNX.ZipArchive";
        }

        std::string message(int value) const override {
            const auto* message = mz_zip_get_error_string(static_cast<mz_zip_error>(value));
            return message ? message : "Unknown ZIP archive error";
        }
    };

    inline const std::error_category& GetZipErrorCategory() {
        static const ZipErrorCategory category;
        return category;
    }

    inline std::error_code GetZipError(mz_zip_archive& archive) {
        const auto error = mz_zip_get_last_error(&archive);
        if (error == MZ_ZIP_NO_ERROR) {
            return std::make_error_code(std::errc::io_error);
        }
        return { static_cast<int>(error), GetZipErrorCategory() };
    }

    inline std::error_code GetFileError() {
        return { errno ? errno : EIO, std::generic_category() };
    }

    struct ArchiveGuard {
        mz_zip_archive* archive;
        ~ArchiveGuard() {
            mz_zip_reader_end(this->archive);
        }
    };

    using FileHandle = std::unique_ptr<FILE, decltype(&std::fclose)>;

    inline std::error_code RemoveTemporaryFile(const fs::path& path) {
        std::error_code error;
        fs::remove(path, error);
        return error;
    }
}

inline std::expected<std::vector<fs::path>, std::error_code> MulNX::Archive::ExtractZipFiles(
    const fs::path& archivePath, const fs::path& destination, std::string_view extension) {
    
    std::error_code directoryError;
    fs::create_directories(destination, directoryError);
    if (directoryError) {
        return std::unexpected(directoryError);
    }

    Detail::FileHandle archiveFile(_wfopen(archivePath.wstring().c_str(), L"rb"), &std::fclose);
    if (!archiveFile) {
        return std::unexpected(Detail::GetFileError());
    }

    mz_zip_archive archive{};
    if (!mz_zip_reader_init_cfile(&archive, archiveFile.get(), 0, 0)) {
        return std::unexpected(Detail::GetZipError(archive));
    }
    const Detail::ArchiveGuard archiveGuard{ &archive };

    std::vector<fs::path> extractedFiles;
    for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&archive); ++i) {
        if (mz_zip_reader_is_file_a_directory(&archive, i)) {
            continue;
        }

        const mz_uint filenameSize = mz_zip_reader_get_filename(&archive, i, nullptr, 0);
        if (filenameSize <= 1) {
            continue;
        }
        std::vector<char> filenameBuffer(filenameSize);
        if (mz_zip_reader_get_filename(&archive, i, filenameBuffer.data(), filenameSize) != filenameSize) {
            return std::unexpected(Detail::GetZipError(archive));
        }

        const fs::path entryPath(filenameBuffer.data());
        if (entryPath.extension().string() != extension) {
            continue;
        }

        const auto outputPath = destination / entryPath.filename();
        auto temporaryPath = outputPath;
        temporaryPath += L".tmp";
        Detail::FileHandle outputFile(_wfopen(temporaryPath.wstring().c_str(), L"wb"), &std::fclose);
        if (!outputFile) {
            return std::unexpected(Detail::GetFileError());
        }

        if (!mz_zip_reader_extract_to_cfile(&archive, i, outputFile.get(), 0)) {
            const auto error = Detail::GetZipError(archive);
            outputFile.reset();
            if (const auto cleanupError = Detail::RemoveTemporaryFile(temporaryPath)) {
                return std::unexpected(cleanupError);
            }
            return std::unexpected(error);
        }
        if (std::fflush(outputFile.get()) != 0) {
            const auto error = Detail::GetFileError();
            outputFile.reset();
            if (const auto cleanupError = Detail::RemoveTemporaryFile(temporaryPath)) {
                return std::unexpected(cleanupError);
            }
            return std::unexpected(error);
        }
        if (std::fclose(outputFile.release()) != 0) {
            const auto error = Detail::GetFileError();
            if (const auto cleanupError = Detail::RemoveTemporaryFile(temporaryPath)) {
                return std::unexpected(cleanupError);
            }
            return std::unexpected(error);
        }

        std::error_code renameError;
        fs::rename(temporaryPath, outputPath, renameError);
        if (renameError) {
            if (const auto cleanupError = Detail::RemoveTemporaryFile(temporaryPath)) {
                return std::unexpected(cleanupError);
            }
            return std::unexpected(renameError);
        }
        extractedFiles.push_back(outputPath);
    }

    return extractedFiles;
}