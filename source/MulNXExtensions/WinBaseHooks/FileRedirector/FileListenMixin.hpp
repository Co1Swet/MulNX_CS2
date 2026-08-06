#pragma once
#include "FileRedirector.hpp"

class BaseFileControl {
    std::wstring_view raw{};
    size_t prefixEnd{};
    LPCWSTR lpFileName = nullptr;
public:
    BaseFileControl(LPCWSTR lpFileName) {
        std::wstring_view raw(lpFileName);
        this->lpFileName = lpFileName;

        size_t prefixEnd = 0;
        if (raw.starts_with(L"\\\\?\\"))
            prefixEnd = 4;
        else if (raw.starts_with(L"\\??\\"))
            prefixEnd = 4;

        this->raw = raw;
        this->prefixEnd = prefixEnd;
    }

    const std::wstring_view& GetRaw()const { return this->raw; }
    const size_t& GetPrefixEnd()const { return this->prefixEnd; }
    const LPCWSTR& GetLpFileName()const { return this->lpFileName; }
    std::wstring_view GetCleanSrc()const { return this->raw.substr(this->prefixEnd); }
};

class CreateFileWControl final :public BaseFileControl {
public:
    using BaseFileControl::BaseFileControl;
    // 可以通过不返回std::nullopt的方式来传递修改
    std::optional<std::wstring*> redirected = std::nullopt;
    std::optional<HANDLE> retFileHandle = std::nullopt;
    // 调用这个函数之后，应当填充retFileHandle，并返回MulNX::Hook::Then::Return，以防出现内存泄露
    std::function<HANDLE(LPCWSTR)>WrapCreateFileW = nullptr;
};

class GetFileAttributesExWControl final :public BaseFileControl {
public:
    using BaseFileControl::BaseFileControl;

    std::optional<std::wstring*> redirected = std::nullopt;
    std::optional<BOOL>retResult = std::nullopt;
    std::function<BOOL(LPCWSTR)>WrapGetFileAttributesExW = nullptr;
};

class IFileListenModule {
public:
    virtual std::optional<MulNX::Hook::Then> OnCreateFileW(CreateFileWControl* pfc) { return std::nullopt; };
    virtual std::optional<MulNX::Hook::Then> OnGetFileAttributesExW(GetFileAttributesExWControl* pac) { return std::nullopt; }
};

template<typename T>
class FileListenMixin : public IFileListenModule {
    T* This() { return static_cast<T*>(this); }
public:
    FileListenMixin() {
        This()->preInits.push_back([this]() {
            auto pRedirector = This()->FindModule<FileRedirector>("FileRedirector");
            pRedirector->listeners.push_back(this);
            return true;
            });
    }
};