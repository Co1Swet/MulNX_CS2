#include "FileRedirector.hpp"
#include "FileListenMixin.hpp"

bool FileRedirector::Init() {
    this->hkCreateFileW = MulNX::Hook::Create((uint8_t*)&CreateFileW, [this](MulNX::Hook* hk, RegContext* ctx) {
        LPCWSTR lpFileName = reinterpret_cast<LPCWSTR>(ctx->rcx);
        if (!lpFileName)return MulNX::Hook::Then::Continue;

        CreateFileWControl fc(lpFileName);

        return this->OnCreateFileW(&fc, ctx);
        }).value();

    this->hkGetFileAttributesExW = MulNX::Hook::Create((uint8_t*)&GetFileAttributesExW, [this](MulNX::Hook* hk, RegContext* ctx) {
        LPCWSTR lpFileName = reinterpret_cast<LPCWSTR>(ctx->rcx);
        if (!lpFileName)return MulNX::Hook::Then::Continue;

        GetFileAttributesExWControl ac(lpFileName);
        ac.WrapGetFileAttributesExW = [&](LPCWSTR lpFileName)->BOOL {
            auto ret = hk->CallMaybeAs<decltype(&GetFileAttributesExW)>(lpFileName,
                *reinterpret_cast<GET_FILEEX_INFO_LEVELS*>(&ctx->rdx),
                std::bit_cast<LPVOID>(ctx->r8));
            return ret;
            };

        return this->OnGetFileAttributesExW(&ac, ctx);
        }).value();

    this->SubscribeSync("System/Init/End", [this](auto&&...) {
        this->RegisterAttachHook(this->hkCreateFileW, "CreateFileW");
        this->RegisterAttachHook(this->hkGetFileAttributesExW, "GetFileAttributesExW");
        });

    return true;
}

MulNX::Hook::Then FileRedirector::OnCreateFileW(CreateFileWControl* pfc, RegContext* ctx) {
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
            ctx->rcx = std::bit_cast<uint64_t>(pfc->GetLpFileName());
        }
        if (pfc->retFileHandle.has_value()) {
            ctx->rax = std::bit_cast<uint64_t>(pfc->retFileHandle.value());
        }
        if (res.has_value())return res.value();
    }
    return MulNX::Hook::Then::Continue;
}

MulNX::Hook::Then FileRedirector::OnGetFileAttributesExW(GetFileAttributesExWControl* pac, RegContext* ctx) {
    for (auto* listener : this->listeners) {
        auto res = listener->OnGetFileAttributesExW(pac);
        if (pac->redirected.has_value()) {
            if (pac->redirected.value() == nullptr) {
                ctx->rcx = 0;
            }
            else {
                ctx->rcx = std::bit_cast<uint64_t>(pac->redirected.value()->c_str());
            }
        }
        else {
            ctx->rcx = std::bit_cast<uint64_t>(pac->GetLpFileName());
        }
        if (pac->retResult.has_value()) {
            ctx->rax = *reinterpret_cast<uint64_t*>(&pac->retResult.value());
        }
        if (res.has_value())return res.value();
    }
    return MulNX::Hook::Then::Continue;
}