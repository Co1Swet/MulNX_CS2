#include "HookDemo.hpp"

bool HookDemo::Init() {
    this->SubscribeSync("Hook/RegisterConCommand", [this](MulNX::Message& msg) {
        auto&& [pCmd] = msg.Access<CCmd*>();
        std::string_view name = pCmd->m_pszName;
        if (name == "playdemo") this->HookPlayDemo(pCmd);
        if (name == "demo_gototick")  this->HookDemoGotoTick(pCmd);
        });

    return true;
}
void HookDemo::HookPlayDemo(CCmd* pCmd) {
    this->hkPlaydemo = MulNX::Hook::Create((uint8_t*)pCmd->m_pCommandCallback, [this](MulNX::Hook* hk, RegContext* ctx) {
        auto cmd = (CCommand*)ctx->rdx;
        hk->CallMaybeOrigin(0, ctx);
        std::string_view raw(cmd->pRawString);
        raw = raw.substr(raw.find(' ') + 1);
        this->BeforePlay(raw);
        return MulNX::Hook::Then::Return;
        }
    ).value();
    this->RegisterAttachHook(this->hkPlaydemo, "PlayDemo");
}
void HookDemo::HookDemoGotoTick(CCmd* pCmd) {
    auto pf = pCmd->m_pCommandCallback;
    this->hkDemoGotoTick = MulNX::Hook::Create((uint8_t*)pCmd->m_pCommandCallback, [this](MulNX::Hook* hk, RegContext* ctx) {
        auto cmd = (CCommand*)ctx->rdx;
        hk->CallMaybeOrigin(0, ctx);
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
    // 1. 去除首尾空白
    std::string rawPath(rawArg);
    size_t start = rawPath.find_first_not_of(" \t");
    if (start == std::string::npos) {
        this->LogError("解析Demo名失败");
        return;
    }
    size_t end = rawPath.find_last_not_of(" \t");
    rawPath = rawPath.substr(start, end - start + 1);

    // 2. 去除可能存在的成对引号
    if (rawPath.size() >= 2) {
        if ((rawPath.front() == '"' && rawPath.back() == '"') ||
            (rawPath.front() == '\'' && rawPath.back() == '\'')) {
            rawPath = rawPath.substr(1, rawPath.size() - 2);
        }
    }

    auto& dirDemos = this->CS2Paths->demo;
    std::filesystem::path demoPath(rawPath);
    if (!demoPath.is_absolute()) {
        demoPath = dirDemos / demoPath;
    }

    // 3. 检查文件是否存在，若不存在且缺少 .dem 后缀则自动补全
    bool fileFound = false;
    if (std::filesystem::exists(demoPath) && std::filesystem::is_regular_file(demoPath)) {
        fileFound = true;   // 原路径直接有效
    }
    else if (demoPath.extension() != ".dem") {
        // 尝试追加 .dem
        std::filesystem::path tryPath = demoPath;
        tryPath += ".dem";
        if (std::filesystem::exists(tryPath) && std::filesystem::is_regular_file(tryPath)) {
            demoPath = tryPath;
            fileFound = true;
            this->LogWarning(std::format("已自动补充 .dem 后缀: {}", demoPath.string()));
        }
    }

    if (!fileFound) {
        this->LogError(std::format("找不到Demo文件: {}", demoPath.string()));
        return;
    }

    // 4. 最终确定，输出播放日志
    this->LogInfo(std::format("播放Demo: {}", demoPath.string()));

    // 发送分析请求（包含完整路径）
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Analyze"_hash);
    rp->str1 = demoPath.string();
    this->PublishAsync(std::move(msg));

    // 发送设置当前操作的 Demo（使用不带扩展名的文件名）
    auto [msg2, rp2] = MulNX::Message::Create<MulNX::NetExt>("Demo/SetOperating"_hash);
    rp2->str1 = demoPath.stem().string();  // 例如 "111"
    this->PublishAsync(std::move(msg2));
}