#include "FileRedirector.hpp"
#include "FileListenMixin.hpp"

bool FileRedirector::Init() {
    this->hkCreateFileW = MulNX::Hook::Create((uint8_t*)&CreateFileW, [this](MulNX::Hook* hook, RegContext* ctx) {
        LPCWSTR lpFileName = reinterpret_cast<LPCWSTR>(ctx->rcx);
        if (!lpFileName)return MulNX::Hook::Then::Continue;

        std::wstring_view raw(lpFileName);

        size_t prefixEnd = 0;
        if (raw.starts_with(L"\\\\?\\"))
            prefixEnd = 4;
        else if (raw.starts_with(L"\\??\\"))
            prefixEnd = 4;

        FileListenControl fc(raw, prefixEnd);

        for (auto* listener : this->listeners) {
            auto res = listener->OnCreateFileW(&fc);
            if (fc.redirected.has_value()) {
                if (fc.redirected.value() == nullptr) {
                    ctx->rcx = 0;
                }
                else {
                    ctx->rcx = std::bit_cast<uint64_t>(fc.redirected.value()->c_str());
                }
            }
            else {
                ctx->rcx = std::bit_cast<uint64_t>(lpFileName);
            }
            if (fc.retFileHandle.has_value()) {
                ctx->rax = std::bit_cast<uint64_t>(fc.retFileHandle.value());
            }
            if (res.has_value())return res.value();
        }

        return MulNX::Hook::Then::Continue;
        }).value();

    this->SubscribeSync("System/Init/End", [this](auto&&...) {
        this->RegisterAttachHook(this->hkCreateFileW, "CreateFileW");
        });

    return true;
}