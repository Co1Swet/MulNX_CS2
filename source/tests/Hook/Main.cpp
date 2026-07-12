#include <MulNXUtils/WinExt/WinExt.hpp>
int Test(int rcx, int rdx, int r8, int r9, int stack4, int stack5) {
    auto str = std::format("{} {} {}", rcx, stack4, stack5);
    MessageBoxA(NULL, str.c_str(), str.c_str(), MB_OK);
    return 0;
}
class Hooker {
    std::unique_ptr<MulNX::Hook> hook = nullptr;
    MulNX::Hook::Then OnTest(MulNX::Hook* hk, RegContext* ctx) {
        if (!this->enable)return MulNX::Hook::Then::Continue;
        ctx->rcx = std::bit_cast<uint32_t>(-1);
        *hk->GetStackParam<int>(ctx, 4) = 42;
        hk->CallMaybeOrigin(2, ctx); // 栈上两个参数
        return MulNX::Hook::Then::Return;
    }
public:
    void DoHook() {
        this->hook = MulNX::Hook::Create((uint8_t*)&Test, [this](MulNX::Hook* hk, RegContext* ctx) {
            return this->OnTest(hk, ctx);
            }).value();
        this->hook->Attach();
    }
    bool enable = true;
};
int main() {
    Hooker hooker{};
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
    hooker.DoHook();
    Test(0, 1, 2, 3, 4, 5);
    hooker.enable = false;
    Test(0, 1, 2, 3, 4, 5);
}