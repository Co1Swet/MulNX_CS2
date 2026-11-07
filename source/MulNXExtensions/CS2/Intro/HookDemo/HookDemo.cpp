#include "HookDemo.hpp"
#include <Support/TimeController/TimeController.hpp>

bool HookDemo::Init() {
    this->SubscribeSync("Hook/RegisterConCommand", [this](MulNX::Message& msg) {
        auto&& [pCmd] = msg.Access<CS2::Console::ConConmand*>();
        std::string_view name = pCmd->m_pszName;
        if (name == "playdemo") {
            this->hkPlaydemo = MulNX::Hook::Create((uint8_t*)pCmd->pConCmdCallback, [this](MulNX::Hook* hk, RegContext* ctx) {
                return this->OnCmdPlayDemo(hk, ctx);
                }
            ).value();
            this->RegisterAttachHook(this->hkPlaydemo, "PlayDemo");
        }
        if (name == "demo_gototick") {
            this->HookDemoGotoTick(pCmd);
        }
        if (name == "demo_timescale") {
            this->hkDemeTimescale = MulNX::Hook::Create((uint8_t*)pCmd->pConCmdCallback, [this](MulNX::Hook* hk, RegContext* ctx) {
                auto cmd = (CS2::Console::CCommandLine*)ctx->rdx;
                if (std::string_view(cmd->pRawString) != "demo_timescale 1.000000") {
                    this->AsyncCommand("host_timescale 1.000000");
                }
                return MulNX::Hook::Then::Continue;
                }).value();
            this->RegisterAttachHook(this->hkDemeTimescale, "DemeTimescale");
        }
        });

    return true;
}
MulNX::Hook::Then HookDemo::OnCmdPlayDemo(MulNX::Hook* hk, RegContext* ctx) {
    auto cmd = (CS2::Console::CCommandLine*)ctx->rdx;

    if (this->CS2Time->IsPlayingDemo()) {
        this->AsyncCommand("disconnect");
        std::string cmdCopy = cmd->pRawString;
        this->SendTask("AsyncPlayDemo", "CSControl", [this, asyncCmd = std::move(cmdCopy)]()mutable {
            if (this->CS2Time->IsPlayingDemo())return true;
            this->AsyncCommand(std::move(asyncCmd));
            return false;
            });
        return MulNX::Hook::Then::Return;
    }
    using PlayDemo_t = void(*)(uint64_t a1, CS2::Console::CCommandLine* a2);
    hk->CallMaybeAs<PlayDemo_t>(ctx->rcx, cmd);
    std::string_view raw(cmd->pRawString);
    raw = raw.substr(raw.find(' ') + 1);
    this->BeforePlay(raw);
    return MulNX::Hook::Then::Return;
}
void HookDemo::HookDemoGotoTick(CS2::Console::ConConmand* pCmd) {
    auto pf = pCmd->pConCmdCallback;
    this->hkDemoGotoTick = MulNX::Hook::Create((uint8_t*)pf, [this](MulNX::Hook* hk, RegContext* ctx) {
        using DemoGotoTick_t = uint64_t(*)(uint64_t a1, CS2::Console::CCommandLine* a2);
        auto cmd = (CS2::Console::CCommandLine*)ctx->rdx;
        hk->CallMaybeAs<DemoGotoTick_t>(ctx->rcx, cmd);
        auto msg = MulNX::Message("Demo/GotoTick/Complete"_hash);
        auto&& [jumpTick] = msg.Access<int>();
        sscanf_s(cmd->pRawString, "demo_gototick %d", &jumpTick);
        this->LogInfo(std::format("跳转到tick: {}", jumpTick));
        this->PublishAsync(std::move(msg));
        return MulNX::Hook::Then::Return;
        }).value();
    this->RegisterAttachHook(this->hkDemoGotoTick, "DemoGotoTick");
}
void HookDemo::BeforePlay(std::string_view rawArg) {
    // ---------- 1. 去首尾空白 ----------
    std::string rawPath(rawArg);
    size_t start = rawPath.find_first_not_of(" \t");
    if (start == std::string::npos) {
        this->LogError("解析Demo名失败");
        return;
    }
    size_t end = rawPath.find_last_not_of(" \t");
    rawPath = rawPath.substr(start, end - start + 1);

    // ---------- 2. 判断成对引号 ----------
    bool hasQuote = false;
    if (rawPath.size() >= 2) {
        if ((rawPath.front() == '"' && rawPath.back() == '"') ||
            (rawPath.front() == '\'' && rawPath.back() == '\'')) {
            rawPath = rawPath.substr(1, rawPath.size() - 2);
            hasQuote = true;
        }
    }
    if (rawPath.empty()) {
        this->LogError("Demo 名为空");
        return;
    }

    // ---------- 3. 构建路径（按 hasQuote 分流） ----------
    auto& dirDemos = this->CS2Paths->demo;
    fs::path demoPath(rawPath);

    if (!hasQuote) {
        // 无引号：语义上只能是 CS2 默认 demo 目录下的相对名
        if (demoPath.is_absolute()) {
            this->LogError(std::format(
                "无引号的 demo 路径不会被当作绝对路径（会被当作 {} 下的相对名）。"
                "如需使用绝对路径，请用双引号包裹: {}",
                dirDemos.string(), rawPath));
            return;
        }
        if (rawPath.find(' ') != std::string::npos) {
            this->LogError("含空格的 demo 名/路径必须用引号包裹");
            return;
        }
        demoPath = dirDemos / demoPath;
    }
    else {
        // 有引号：绝对路径原样使用，相对路径拼到 demo 目录
        if (!demoPath.is_absolute()) {
            demoPath = dirDemos / demoPath;
        }
    }

    // ---------- 4. .dem 补全 + 存在性检查 ----------
    bool fileFound = false;
    if (fs::exists(demoPath) && fs::is_regular_file(demoPath)) {
        fileFound = true;
    }
    else if (demoPath.extension() != ".dem") {
        fs::path tryPath = demoPath;
        tryPath += ".dem";
        if (fs::exists(tryPath) && fs::is_regular_file(tryPath)) {
            demoPath = tryPath;
            fileFound = true;
            this->LogInfo(std::format("已自动补充 .dem 后缀: {}", demoPath.string()));
        }
    }
    if (!fileFound) {
        this->LogError(std::format("找不到Demo文件: {}", demoPath.string()));
        return;
    }

    // ---------- 5. 确定 key ----------
    std::string key;
    if (!hasQuote) {
        // 无引号：固定标记为 kGAMEDEMOS
        key = "kGAMEDEMOS";
    }
    else {
        // 有引号：反查所在目录对应的静态 key
        auto* pPath = this->Path();
        if (!pPath) {
            this->LogError("PathManager 不可用");
            return;
        }
        const fs::path parentPath = demoPath.parent_path();
        auto keys = pPath->PathFindStaticKeys(parentPath);
        if (keys.empty()) {
            this->LogError(std::format(
                "Demo 所在目录未注册到 PathManager: {}", parentPath.string()));
            return;
        }
        key = std::move(keys.front());
    }

    // ---------- 6. demoName（无扩展名） ----------
    const std::string demoName = demoPath.stem().string();

    this->LogInfo(std::format("播放Demo: {} (key={}, name={})",
        demoPath.string(), key, demoName));

    // ---------- 7. 发消息 ----------
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/SetPlaying"_hash);
    rp->str1 = key;
    rp->str2 = demoName;
    this->PublishAsync(std::move(msg));

    auto [msg2, rp2] = MulNX::Message::Create<MulNX::NetExt>("Demo/SetOperating"_hash);
    rp2->str1 = std::move(key);
    rp2->str2 = std::move(demoName);
    this->PublishAsync(std::move(msg2));
}