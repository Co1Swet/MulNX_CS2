#include "Logger.hpp"
#include <MulNX/Systems/PathManager/PathManager.hpp>
#include <MulNX/Systems/I18nManager/I18nManager.hpp>
#include <MulNX/Systems/TaskSystem/TaskSystem.hpp>
#include <MulNX/Systems/MessageManager/MessageManager.hpp>
#include <format>

bool MulNX::Logger::Init() {
    this->logPath = this->Path()->PathGetForShared("Log") / ("Log_" + this->Core->GetName() + ".txt");
    this->target = std::ofstream(this->logPath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!this->target) {
        MulNX::ErrorTerminate("Cannot Wirte Log!");
    }
    this->target << I18n("log.new") << std::endl;

    this->lineFmt = I18n("log.fmt");

    this->kInfo = I18n("log.info");
    this->kSucc = I18n("log.succ");
    this->kWarning = I18n("log.warning");
    this->kError = I18n("log.error");

    this->ImCreateTask("Log", "Loging", [this]()->bool {
        this->Log();
        return true;
        });

    return true;
}

std::string MulNX::Logger::PraseLevel(Log::Level level) {
    switch (level) {
    case Log::Level::Info: {
        return this->kInfo;
    }
    case Log::Level::Succ: {
        return this->kSucc;
    }
    case Log::Level::Warning: {
        return this->kWarning;
    }
    case Log::Level::Error: {
        return this->kError;
    }
    default: {
        MulNX::ErrorTerminate("错误日志等级！");
    }
    }
}
MulNX::MsgType MulNX::Logger::MakeMsgType(Log::Level level) {
    switch (level) {
    case Log::Level::Info: {
        return "Log/Info"_hash;
    }
    case Log::Level::Succ: {
        return "Log/Succ"_hash;
    }
    case Log::Level::Warning: {
        return "Log/Warning"_hash;
    }
    case Log::Level::Error: {
        return "Log/Error"_hash;
    }
    default: {
        return 0;
    }
    }
}

void MulNX::Logger::Log() {
    // 批量取出当前队列中所有日志
    std::vector<MulNX::Log> batch;
    batch.reserve(256); // 预分配空间，减少扩容开销

    MulNX::Log log;
    while (this->logs.wait_dequeue_timed(log, 200)) {
        batch.push_back(std::move(log));
    }

    if (batch.empty()) return;

    // 按时间戳升序排序，还原真实事件顺序
    std::sort(batch.begin(), batch.end(),
        [](const MulNX::Log& a, const MulNX::Log& b) {
            return a.timestamp_us < b.timestamp_us;
        });

    // 依次格式化并写入文件
    for (auto& entry : batch) {
        std::string moduleName = entry.pModule->GetName();
        std::string rawMsg = std::move(entry.raw);
        auto eventTime = MulNX::FromUnixUs(entry.timestamp_us);
        std::string levelStr = this->PraseLevel(entry.level);

        std::istringstream iss(rawMsg);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) continue;

            std::string formatted = std::vformat(
                this->lineFmt,
                std::make_format_args(eventTime, levelStr, moduleName, line));

            this->target << formatted << '\n';

            MulNX::MsgType type = this->MakeMsgType(entry.level);
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>(type);
            rp->str1 = std::move(formatted);
            this->PublishAsync(std::move(msg));
        }
    }

    // 可选的定期刷盘（根据你的需求调整频率）
    // this->target.flush();
}