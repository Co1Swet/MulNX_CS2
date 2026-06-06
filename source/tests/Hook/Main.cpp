#include <MulNXExtensions/WinExt/WinExt.hpp>
int Test(int rcx, int rdx, int r8, int r9, int stack4, int stack5) {
    auto str1 = std::to_string(stack4);
    auto str2 = std::to_string(stack5);
    MessageBoxA(NULL, str1.c_str(), str2.c_str(), MB_OK);
    return 0;
}
int main() {
    {
        auto hkMessageBoxW = MulNX::Hook::Create((uint8_t*)&MessageBoxW, [](MulNX::Hook* hk, RegContext* ctx) {
            if (wcscmp((wchar_t*)(ctx->rdx), L"no show"))return MulNX::Hook::Then::Return;
            ctx->rdx = (uint64_t)(L"hacked");
            reinterpret_cast<decltype(MessageBoxW)*>(hk->pMaybeRawFunc)(NULL, L"callback", L"example", MB_OK);
            ctx->rdx = (uint64_t)(L"RegCall");
            hk->CallMaybeOrigin(0, ctx);
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
    Test(0, 1, 2, 3, 4, 5);
    auto hkTest = MulNX::Hook::Create((uint8_t*)&Test, [](MulNX::Hook* hk, RegContext* ctx) {
        hk->CallMaybeOrigin(2, ctx); // 栈上两个参数
        return MulNX::Hook::Then::Return;
        }).value();
    hkTest->Attach();
    Test(0, 1, 2, 3, 4, 5);
}