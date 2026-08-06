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

        CreateFileWControl fc(raw, prefixEnd);

        return this->OnCreateFileW(&fc, ctx, lpFileName);
        }).value();

    this->SubscribeSync("System/Init/End", [this](auto&&...) {
        this->RegisterAttachHook(this->hkCreateFileW, "CreateFileW");
        });

    this->hkGetFileAttributesExW = MulNX::Hook::Create((uint8_t*)&GetFileAttributesExW, [this](MulNX::Hook* hk, RegContext* ctx) {

        return MulNX::Hook::Then::Continue;
        }).value();
    this->RegisterAttachHook(this->hkGetFileAttributesExW, "GetFileAttributesExW");

    return true;
}

MulNX::Hook::Then FileRedirector::OnCreateFileW(CreateFileWControl* pfc, RegContext* ctx, LPCWSTR lpFileName) {
    for (auto* listener : this->listeners) {
        auto res = listener->OnCreateFileW(pfc);
        if (pfc->redirected.has_value()) {
            if (pfc->redirected.value() == nullptr) {
                ctx->rcx = 0;
            }
            else {
                ctx->rcx = std::bit_cast<uint64_t>(pfc->redirected.value()->c_str());
            }
        }
        else {
            ctx->rcx = std::bit_cast<uint64_t>(lpFileName);
        }
        if (pfc->retFileHandle.has_value()) {
            ctx->rax = std::bit_cast<uint64_t>(pfc->retFileHandle.value());
        }
        if (res.has_value())return res.value();
    }
    return MulNX::Hook::Then::Continue;
}