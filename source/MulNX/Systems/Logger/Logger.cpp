#include "Logger.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/PathManager/PathManager.hpp>
#include <MulNX/Systems/I18nManager/I18nManager.hpp>
#include <format>

bool MulNX::Logger::Init() {
    this->logPath = this->ISys().Path()->PathGetForShared("Log") / ("Log_" + this->Core->GetName() + ".txt");
    this->target = std::ofstream(this->logPath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!this->target) {
        MulNX::ErrorTerminate("Cannot Wirte Log!");
    }
    this->target << I18n("log.new") << std::endl;
    this->ISys().SendTask("Log", "Loging", [this]()->bool {
        this->Log();
        return true;
        });

    this->lineFmt = I18n("log.fmt");
    
    this->kInfo = I18n("log.info");
    this->kSucc = I18n("log.succ");
    this->kWarning = I18n("log.warning");
    this->kError = I18n("log.error");

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

void MulNX::Logger::Log() {
    MulNX::Log meta;
    while (this->logs.try_dequeue(meta)) {
        std::string moduleName = meta.pModule->GetName();
        std::string rawMsg = std::move(meta.raw);
        auto eventTime = MulNX::FromUnixUs(meta.timestamp_us);
        std::string levelStr = this->PraseLevel(meta.level);

        std::istringstream iss(rawMsg);
        std::string line;
        bool first = true;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) continue;
            
            std::string formatted = std::vformat(this->lineFmt, std::make_format_args(
                eventTime,
                levelStr,
                moduleName,
                line
            ));

            this->target << formatted << std::endl;   // 应该是 formatted 而不是 line
        }
    }
}