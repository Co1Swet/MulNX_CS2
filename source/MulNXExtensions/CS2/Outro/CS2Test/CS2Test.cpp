#include "CS2Test.hpp"
#include <Mirror/ResourceSystem/ResourceSystem.hpp>

ResourceSystem* pResourceSystem = nullptr;

void CS2Test::UI() {
    auto pLocalPawn = this->CS2Entitys->GetLocalPlayerPawnEx();
    if (!pLocalPawn)return;
    auto observerService = MulNX::MRead(pLocalPawn->pObserverServices());
    auto pMode = observerService->iObserverMode();

    uint64_t address = (uint64_t)pMode;
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%016llX", address);

    ImGui::InputText("Address", buf, sizeof(buf),
        ImGuiInputTextFlags_ReadOnly);
}

bool CS2Test::Init() {
    std::thread([]() {
        MessageBoxW(NULL, L"MulNX 注入成功！", L"MulNX", MB_OK | MB_ICONINFORMATION);
        }).detach();

    pResourceSystem = this->FindModule<ResourceSystem>("ResourceSystem");

    this->AsyncCommand("playdemo 111");
    this->AsyncCommand("tv_listen_voice_indices -1");

    this->SendUIRoot("MyCS2Test", [this](auto&&...) {
        try {
            this->UI();
        }
        catch (MulNX::Exception& e) {

        }
        });

    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](auto&&...) {
        auto base = this->CS2->engine2.GetBaseAddress();

        static auto hookFormat = MulNX::Hook::Create(
            (uint8_t*)base + 0x85522,
            [this](MulNX::Hook* hk, RegContext* ctx) {
                auto mapName = (char*)ctx->r8;
                auto thisPtr = (void*)ctx->rcx;
                if (mapName) {
                    this->LogInfo(std::format(
                        "[Format] this={:#x} map={}",
                        (uintptr_t)thisPtr, mapName));
                }
                return MulNX::Hook::Then::Continue;
            }, true).value();
        hookFormat->Attach();

        static auto hookSet = MulNX::Hook::Create(
            (uint8_t*)base + 0x85541,
            [this](MulNX::Hook* hk, RegContext* ctx) {
                auto mapName = (char*)ctx->rdx;
                auto thisPtr = (void*)ctx->rcx;
                if (mapName) {
                    this->LogInfo(std::format(
                        "[Set]    this={:#x} map={}",
                        (uintptr_t)thisPtr, mapName));
                }
                return MulNX::Hook::Then::Continue;
            }, true).value();
        hookSet->Attach();
        });

    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](auto&&...) {
        auto base = this->CS2->engine2.GetBaseAddress();

        static auto hookManifestPath = MulNX::Hook::Create(
            (uint8_t*)(base + 0x3F425D),
            [this](MulNX::Hook* hk, RegContext* ctx) {
                auto path = (const char*)ctx->r8;
                this->LogInfo(std::format("[Manifest/Path] {}", path));
                return MulNX::Hook::Then::Continue;
            }, true).value();
        hookManifestPath->Attach();
        });

    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {
        static auto hook = MulNX::Hook::Create(
            (uint8_t*)this->CS2->engine2.GetBaseAddress() + 0x83346,
            [this](MulNX::Hook* hk, RegContext* ctx) {
                uintptr_t v51 = ctx->r8;                      
                const char* rawName = *(const char**)v51;      
                const char* xformName = *(const char**)(v51 + 8);

                this->LogWarning(std::format("游戏尝试加载：raw={} xform={}",
                    rawName ? rawName : "<null>",
                    xformName ? xformName : "<null>"));

                auto isPureMap = [](const char* s) {
                    if (!s || !*s) return false;
                    if (strchr(s, '/') || strchr(s, '\\')) return false;
                    if (strstr(s, "prefabs")) return false;
                    return true;
                    };
                auto isSkybox = [](const char* s) {
                    if (!s) return false;
                    return std::string_view(s).find("skybox") != std::string_view::npos;
                    };

                bool rawMap = isPureMap(rawName);
                bool xformMap = isPureMap(xformName);
                bool rawSky = isSkybox(rawName);
                bool xformSky = isSkybox(xformName);

                if (rawMap || xformMap) {
                    this->LogInfo(std::format("地图：raw={} xform={}",
                        rawName ? rawName : "<null>",
                        xformName ? xformName : "<null>"));
                    return MulNX::Hook::Then::Continue;
                }

                if (rawSky || xformSky) {
                    this->LogInfo(std::format("天空：raw={} xform={}",
                        rawName ? rawName : "<null>",
                        xformName ? xformName : "<null>"));
                    return MulNX::Hook::Then::Continue;
                }

                this->LogInfo(std::format("拦截：raw={} xform={}",
                    rawName ? rawName : "<null>",
                    xformName ? xformName : "<null>"));
                return MulNX::Hook::Then::SkipAllAndContinue;
            }, true).value();
        hook->Attach();
        });

    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](auto&&...) {
        auto base = this->CS2->engine2.GetBaseAddress();

        static auto hookMapReq = MulNX::Hook::Create(
            (uint8_t*)(base + 0x21B500),
            [this](MulNX::Hook* hk, RegContext* ctx) {
                auto mapName = (const char*)ctx->rdx;
                auto addonName = (const char*)ctx->r8;
                auto isChangelevel = (uint8_t)ctx->r9;
                auto optionsPtr = hk->GetStackParam<void*>(ctx, 4);
                this->LogInfo(std::format(
                    "[HostState/Map] map={} addon={} isChangelevel={} options={:#x}",
                    mapName ? mapName : "<null>",
                    addonName ? addonName : "<null>",
                    isChangelevel,
                    optionsPtr ? (uintptr_t)*optionsPtr : 0));
                return MulNX::Hook::Then::Continue;
            }).value();
        hookMapReq->Attach();

        static auto hookPlayDemo = MulNX::Hook::Create(
            (uint8_t*)(base + 0x21AB20),
            [this](MulNX::Hook* hk, RegContext* ctx) {
                auto demoName = (const char*)ctx->rdx;
                auto addonName = (const char*)ctx->r8;
                auto flag = (uint8_t)ctx->r9;
                auto optionsPtr = hk->GetStackParam<void*>(ctx, 4);
                this->LogInfo(std::format(
                    "[HostState/Demo] demo={} addon={} flag={} options={:#x}",
                    demoName ? demoName : "<null>",
                    addonName ? addonName : "<null>",
                    flag,
                    optionsPtr ? (uintptr_t)*optionsPtr : 0));
                return MulNX::Hook::Then::Continue;
            }).value();
        hookPlayDemo->Attach();

        static auto hookAddonDL = MulNX::Hook::Create(
            (uint8_t*)(base + 0x21AE60),
            [this](MulNX::Hook* hk, RegContext* ctx) {
                auto optionsPtr = (void*)ctx->rdx;
                this->LogInfo(std::format(
                    "[HostState/AddonDL] options={:#x}",
                    (uintptr_t)optionsPtr));
                return MulNX::Hook::Then::Continue;
            }).value();
        hookAddonDL->Attach();
        });

    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](auto&&...) {
        auto base = this->CS2->engine2.GetBaseAddress();

        static auto hook = MulNX::Hook::Create(
            (uint8_t*)(base + 0x17916C),
            [this, base](MulNX::Hook* hk, RegContext* ctx) {
                auto a1 = (uint8_t*)ctx->rsi;
                if (!a1) return MulNX::Hook::Then::Continue;
                if (!ctx->rax) return MulNX::Hook::Then::Continue;

                uint32_t hasBits = *(uint32_t*)(a1 + 0x10);
                if ((hasBits & 0x01) == 0) return MulNX::Hook::Then::Continue;

                uint64_t tagged = *(uint64_t*)(a1 + 0x18);
                uint64_t pStd = tagged & ~3ull;
                if (!pStd || pStd == (base + 0x62ED10)) return MulNX::Hook::Then::Continue;

                uint64_t size = *(uint64_t*)(pStd + 0x10);
                uint64_t capacity = *(uint64_t*)(pStd + 0x18);
                if (size == 0) return MulNX::Hook::Then::Continue;

                char* data = (capacity > 0xF) ? *(char**)pStd : (char*)pStd;
                std::string_view name(data, (size_t)size);
                this->LogInfo(std::format("[SpawnGroup/Name] {}", name));

                if (name == "de_inferno") {
                    constexpr std::string_view newName = "de_overpass";

                    char* dst = (capacity > 0xF) ? *(char**)pStd : (char*)pStd;
                    memcpy(dst, newName.data(), newName.size());
                    dst[newName.size()] = 0;
                    *(uint64_t*)(pStd + 0x10) = newName.size();

                    this->LogInfo("[SpawnGroup/Name] → de_overpass");
                }

                return MulNX::Hook::Then::Continue;
            },true).value();
        hook->Attach();
        });

    

    return true;
}