#pragma once
#include "FileRedirector.hpp"

class IFileListenModule {
public:
    virtual std::optional<MulNX::Hook::Then> OnCreateFileW(MulNX::Hook* hk, RegContext* ctx) = 0;
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