#pragma once
#include "FileRedirector.hpp"

class FileListenControl {
    std::wstring_view raw{};
    size_t prefixEnd{};
public:
    // 可以通过不返回std::nullopt的方式来传递修改
    std::optional<std::wstring*> redirected = std::nullopt;
    std::optional<HANDLE> retFileHandle = std::nullopt;
    // 调用这个函数之后，应当填充retFileHandle，并返回MulNX::Hook::Then::Return，以防出现内存泄露
    std::function<HANDLE(LPCWSTR)>WrapCreateFile = nullptr;

    FileListenControl(std::wstring_view raw, size_t prefixEnd) {
        this->raw = raw;
        this->prefixEnd = prefixEnd;
    }

    const std::wstring_view& GetRaw()const { return this->raw; }
    const size_t& GetPrefixEnd()const { return this->prefixEnd; }
    std::wstring_view GetCleanSrc()const { return this->raw.substr(this->prefixEnd); }
};

class IFileListenModule {
public:
    virtual std::optional<MulNX::Hook::Then> OnCreateFileW(FileListenControl* pfc) = 0;
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