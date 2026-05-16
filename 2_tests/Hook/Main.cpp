#include <MulNXExtensions/WinExt/WinExt.hpp>
int main() {
    auto hkMessageBoxW = MulNX::Hook::Create((uint8_t*)&MessageBoxW, 0, false, [](MulNX::Hook* hk, RegContext* ctx) {
        if (strstr((char*)(ctx->rdx), (char*)L"no show"))return MulNX::Hook::Then::Return;
        ctx->rdx = (uint64_t)(L"hacked");
        reinterpret_cast<decltype(MessageBoxW)*>(hk->pMaybeRawFunc)(NULL, L"callback", L"example", MB_OK);
        return MulNX::Hook::Then::Continue;
        }).value();
    hkMessageBoxW->Attach();
    MessageBoxW(NULL, L"cant see", L"example", MB_OK);
    reinterpret_cast<decltype(MessageBoxW)*>(hkMessageBoxW->pMaybeRawFunc)(NULL, L"can see", L"example", MB_OK);
    hkMessageBoxW->Detach();
    MessageBoxW(NULL, L"can see", L"example", MB_OK);
    hkMessageBoxW->Attach();
    MessageBoxW(NULL, L"no show", L"example", MB_OK);
    MessageBoxW(NULL, L"show hacked", L"example", MB_OK);
}